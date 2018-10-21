// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cmath>
#include <mutex>
#include <QApplication>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QScreen>
#include <QWindow>
#include "citra/bootmanager.h"
#include "common/scm_rev.h"
#include "core/3ds.h"
#include "core/core.h"
#include "core/input.h"
#include "core/settings.h"
#include "input_common/keyboard.h"
#include "input_common/main.h"
#include "input_common/motion_emu.h"
#include "video_core/video_core.h"

EmuThread::EmuThread(Screens* screens) : screens{screens} {}

void EmuThread::run() {
    screens->MakeCurrent();
    stop_run = false;
    auto& system{Core::System::GetInstance()};
    while (!stop_run) {
        Core::System::ResultStatus result{system.RunLoop()};
        if (result == Core::System::ResultStatus::ShutdownRequested) {
            // Notify frontend we shutdown
            emit ErrorThrown(result, "");
            // End emulation execution
            break;
        }
        if (result != Core::System::ResultStatus::Success) {
            system.SetRunning(false);
            emit ErrorThrown(result, system.GetStatusDetails());
        }
    }

    // Shutdown the core emulation
    system.Shutdown();
    screens->moveContext();
}

// This class overrides paintEvent and resizeEvent to prevent the GUI thread from stealing GL
// context.
// The corresponding functionality is handled in EmuThread instead
class GGLWidgetInternal : public QGLWidget {
public:
    GGLWidgetInternal(QGLFormat fmt, Screens* parent) : QGLWidget{fmt, parent}, parent{parent} {}

    void paintEvent(QPaintEvent* ev) override {}

    void resizeEvent(QResizeEvent* ev) override {
        parent->UpdateCurrentFramebufferLayout(ev->size().width(), ev->size().height());
        parent->OnFramebufferSizeChanged();
    }

private:
    Screens* parent;
};

Screens::Screens(QWidget* parent, EmuThread* emu_thread) : QWidget{parent}, emu_thread{emu_thread} {
    setAttribute(Qt::WA_AcceptTouchEvents);

    InputCommon::Init();
}

Screens::~Screens() {
    InputCommon::Shutdown();
}

void Screens::moveContext() {
    DoneCurrent();

    // If the thread started running, move the GL Context to the new thread. Otherwise, move it
    // back.
    auto thread{(QThread::currentThread() == qApp->thread() && emu_thread != nullptr)
                    ? emu_thread
                    : qApp->thread()};
    child->context()->moveToThread(thread);
}

void Screens::SwapBuffers() {
    // In our multi-threaded QGLWidget use case we shouldn't need to call `makeCurrent`,
    // since we never call `doneCurrent` in this thread.
    // However:
    // - The Qt debug runtime prints a bogus warning on the console if `makeCurrent` wasn't called
    // since the last time `swapBuffers` was executed;
    // - On macOS, if `makeCurrent` isn't called explicitely, resizing the buffer breaks.
    child->makeCurrent();
    child->swapBuffers();
}

void Screens::MakeCurrent() {
    child->makeCurrent();
}

void Screens::DoneCurrent() {
    child->doneCurrent();
}

// On Qt 5.0+, this correctly gets the size of the framebuffer (pixels).
//
// Older versions get the window size (density independent pixels),
// and hence, do not support DPI scaling ("retina" displays).
// The result will be a viewport that is smaller than the extent of the window.
void Screens::OnFramebufferSizeChanged() {
    // Screen changes potentially incur a change in screen DPI, hence we should update the
    // framebuffer size
    qreal pixel_ratio{windowPixelRatio()};
    unsigned width{static_cast<unsigned>(child->QPaintDevice::width() * pixel_ratio)};
    unsigned height{static_cast<unsigned>(child->QPaintDevice::height() * pixel_ratio)};
    UpdateCurrentFramebufferLayout(width, height);
}

void Screens::BackupGeometry() {
    geometry = ((QGLWidget*)this)->saveGeometry();
}

void Screens::RestoreGeometry() {
    // We don't want to back up the geometry here (obviously)
    QWidget::restoreGeometry(geometry);
}

void Screens::restoreGeometry(const QByteArray& geometry) {
    // Make sure users of this class don't need to deal with backing up the geometry themselves
    QWidget::restoreGeometry(geometry);
    BackupGeometry();
}

QByteArray Screens::saveGeometry() {
    // If we are a top-level widget, store the current geometry
    // otherwise, store the last backup
    if (parent() == nullptr)
        return ((QGLWidget*)this)->saveGeometry();
    else
        return geometry;
}

qreal Screens::windowPixelRatio() const {
    // windowHandle() might not be accessible until the window is displayed to screen.
    return windowHandle() ? windowHandle()->screen()->devicePixelRatio() : 1.0f;
}

std::pair<unsigned, unsigned> Screens::ScaleTouch(const QPointF pos) const {
    const qreal pixel_ratio{windowPixelRatio()};
    return {static_cast<unsigned>(std::max(std::round(pos.x() * pixel_ratio), qreal{0.0})),
            static_cast<unsigned>(std::max(std::round(pos.y() * pixel_ratio), qreal{0.0}))};
}

void Screens::closeEvent(QCloseEvent* event) {
    emit Closed();
    QWidget::closeEvent(event);
}

void Screens::keyPressEvent(QKeyEvent* event) {
    InputCommon::GetKeyboard()->PressKey(event->key());
}

void Screens::keyReleaseEvent(QKeyEvent* event) {
    InputCommon::GetKeyboard()->ReleaseKey(event->key());
}

void Screens::mousePressEvent(QMouseEvent* event) {
    if (event->source() == Qt::MouseEventSynthesizedBySystem)
        return; // Touch input is handled in TouchBeginEvent

    auto pos{event->pos()};
    if (event->button() == Qt::LeftButton) {
        const auto [x, y]{ScaleTouch(pos)};
        TouchPressed(x, y);
    } else if (event->button() == Qt::RightButton) {
        InputCommon::GetMotionEmu()->BeginTilt(pos.x(), pos.y());
    }
}

void Screens::mouseMoveEvent(QMouseEvent* event) {
    if (event->source() == Qt::MouseEventSynthesizedBySystem)
        return; // Touch input is handled in TouchUpdateEvent

    auto pos{event->pos()};
    const auto [x, y]{ScaleTouch(pos)};
    TouchMoved(x, y);
    InputCommon::GetMotionEmu()->Tilt(pos.x(), pos.y());
}

void Screens::mouseReleaseEvent(QMouseEvent* event) {
    if (event->source() == Qt::MouseEventSynthesizedBySystem)
        return; // Touch input is handled in TouchEndEvent
    if (event->button() == Qt::LeftButton)
        TouchReleased();
    else if (event->button() == Qt::RightButton)
        InputCommon::GetMotionEmu()->EndTilt();
}

void Screens::TouchBeginEvent(const QTouchEvent* event) {
    // TouchBegin always has exactly one touch point, so take the .first()
    const auto [x, y]{ScaleTouch(event->touchPoints().first().pos())};
    TouchPressed(x, y);
}

void Screens::TouchUpdateEvent(const QTouchEvent* event) {
    QPointF pos;
    int active_points{};
    // Average all active touch points
    for (const auto tp : event->touchPoints()) {
        if (tp.state() & (Qt::TouchPointPressed | Qt::TouchPointMoved | Qt::TouchPointStationary)) {
            active_points++;
            pos += tp.pos();
        }
    }
    pos /= active_points;
    const auto [x, y]{ScaleTouch(pos)};
    TouchMoved(x, y);
}

void Screens::TouchEndEvent() {
    TouchReleased();
}

bool Screens::event(QEvent* event) {
    if (event->type() == QEvent::TouchBegin) {
        TouchBeginEvent(static_cast<QTouchEvent*>(event));
        return true;
    } else if (event->type() == QEvent::TouchUpdate) {
        TouchUpdateEvent(static_cast<QTouchEvent*>(event));
        return true;
    } else if (event->type() == QEvent::TouchEnd || event->type() == QEvent::TouchCancel) {
        TouchEndEvent();
        return true;
    }
    return QWidget::event(event);
}

void Screens::focusOutEvent(QFocusEvent* event) {
    QWidget::focusOutEvent(event);
    InputCommon::GetKeyboard()->ReleaseAllKeys();
}

void Screens::InitRenderTarget() {
    if (child)
        delete child;
    if (layout())
        delete layout();
    // TODO: One of these flags might be interesting: WA_OpaquePaintEvent, WA_NoBackground,
    // WA_DontShowOnScreen, WA_DeleteOnClose
    QGLFormat fmt;
    fmt.setVersion(3, 3);
    fmt.setProfile(QGLFormat::CoreProfile);
    fmt.setSwapInterval(false);
    // Requests a forward-compatible context, which is required to get a 3.2+ context on macOS
    fmt.setOption(QGL::NoDeprecatedFunctions);
    child = new GGLWidgetInternal(fmt, this);
    QBoxLayout* layout{new QHBoxLayout(this)};
    resize(Core::kScreenTopWidth, Core::kScreenTopHeight + Core::kScreenBottomHeight);
    layout->addWidget(child);
    layout->setMargin(0);
    setLayout(layout);
    setMinimumSize(400, 480);
    OnFramebufferSizeChanged();
    BackupGeometry();
}

void Screens::CaptureScreenshot(u16 res_scale, const QString& screenshot_path) {
    if (!res_scale)
        res_scale = Settings::values.resolution_factor;
    const Layout::FramebufferLayout layout{Layout::FrameLayoutFromResolutionScale(res_scale)};
    screenshot_image = QImage(QSize(layout.width, layout.height), QImage::Format_RGB32);
    VideoCore::RequestScreenshot(screenshot_image.bits(),
                                 [=] {
                                     screenshot_image.mirrored(false, true).save(screenshot_path);
                                     LOG_INFO(Frontend, "The screenshot is saved.");
                                 },
                                 layout);
}

void Screens::OnEmulationStarting(EmuThread* emu_thread) {
    this->emu_thread = emu_thread;
}

void Screens::OnEmulationStopping() {
    emu_thread = nullptr;
}

void Screens::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);

    // windowHandle() is not initialized until the Window is shown, so we connect it here.
    connect(windowHandle(), &QWindow::screenChanged, this, &Screens::OnFramebufferSizeChanged,
            Qt::UniqueConnection);
}
