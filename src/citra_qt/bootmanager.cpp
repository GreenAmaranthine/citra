#include <QApplication>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QScreen>
#include <QWindow>
#include "citra_qt/bootmanager.h"
#include "common/scm_rev.h"
#include "core/3ds.h"
#include "core/core.h"
#include "core/settings.h"
#include "input_common/keyboard.h"
#include "input_common/main.h"
#include "input_common/motion_emu.h"
#include "video_core/video_core.h"

EmuThread::EmuThread(GRenderWindow* render_window) : render_window{render_window} {}

void EmuThread::run() {
    render_window->MakeCurrent();
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
    render_window->moveContext();
}

// This class overrides paintEvent and resizeEvent to prevent the GUI thread from stealing GL
// context.
// The corresponding functionality is handled in EmuThread instead
class GGLWidgetInternal : public QGLWidget {
public:
    GGLWidgetInternal(QGLFormat fmt, GRenderWindow* parent)
        : QGLWidget{fmt, parent}, parent{parent} {}

    void paintEvent(QPaintEvent* ev) override {
        if (do_painting) {
            QPainter painter{this};
        }
    }

    void resizeEvent(QResizeEvent* ev) override {
        parent->OnClientAreaResized(ev->size().width(), ev->size().height());
        parent->OnFramebufferSizeChanged();
    }

    void DisablePainting() {
        do_painting = false;
    }

    void EnablePainting() {
        do_painting = true;
    }

private:
    GRenderWindow* parent{};
    bool do_painting{};
};

GRenderWindow::GRenderWindow(QWidget* parent, EmuThread* emu_thread)
    : QWidget{parent}, child{nullptr}, emu_thread{emu_thread} {

    setWindowTitle("Render Window");
    setAttribute(Qt::WA_AcceptTouchEvents);

    InputCommon::Init();
}

GRenderWindow::~GRenderWindow() {
    InputCommon::Shutdown();
}

void GRenderWindow::moveContext() {
    DoneCurrent();

    // If the thread started running, move the GL Context to the new thread. Otherwise, move it
    // back.
    auto thread{(QThread::currentThread() == qApp->thread() && emu_thread != nullptr)
                    ? emu_thread
                    : qApp->thread()};
    child->context()->moveToThread(thread);
}

void GRenderWindow::SwapBuffers() {
    // In our multi-threaded QGLWidget use case we shouldn't need to call `makeCurrent`,
    // since we never call `doneCurrent` in this thread.
    // However:
    // - The Qt debug runtime prints a bogus warning on the console if `makeCurrent` wasn't called
    // since the last time `swapBuffers` was executed;
    // - On macOS, if `makeCurrent` isn't called explicitely, resizing the buffer breaks.
    child->makeCurrent();
    child->swapBuffers();
}

void GRenderWindow::MakeCurrent() {
    child->makeCurrent();
}

void GRenderWindow::DoneCurrent() {
    child->doneCurrent();
}

void GRenderWindow::PollEvents() {}

// On Qt 5.0+, this correctly gets the size of the framebuffer (pixels).
//
// Older versions get the window size (density independent pixels),
// and hence, do not support DPI scaling ("retina" displays).
// The result will be a viewport that is smaller than the extent of the window.
void GRenderWindow::OnFramebufferSizeChanged() {
    // Screen changes potentially incur a change in screen DPI, hence we should update the
    // framebuffer size
    qreal pixel_ratio{windowPixelRatio()};
    unsigned width{static_cast<unsigned>(child->QPaintDevice::width() * pixel_ratio)};
    unsigned height{static_cast<unsigned>(child->QPaintDevice::height() * pixel_ratio)};
    UpdateCurrentFramebufferLayout(width, height);
}

void GRenderWindow::BackupGeometry() {
    geometry = ((QGLWidget*)this)->saveGeometry();
}

void GRenderWindow::RestoreGeometry() {
    // We don't want to back up the geometry here (obviously)
    QWidget::restoreGeometry(geometry);
}

void GRenderWindow::restoreGeometry(const QByteArray& geometry) {
    // Make sure users of this class don't need to deal with backing up the geometry themselves
    QWidget::restoreGeometry(geometry);
    BackupGeometry();
}

QByteArray GRenderWindow::saveGeometry() {
    // If we are a top-level widget, store the current geometry
    // otherwise, store the last backup
    if (parent() == nullptr)
        return ((QGLWidget*)this)->saveGeometry();
    else
        return geometry;
}

qreal GRenderWindow::windowPixelRatio() {
    // windowHandle() might not be accessible until the window is displayed to screen.
    return windowHandle() ? windowHandle()->screen()->devicePixelRatio() : 1.0f;
}

void GRenderWindow::closeEvent(QCloseEvent* event) {
    emit Closed();
    QWidget::closeEvent(event);
}

void GRenderWindow::keyPressEvent(QKeyEvent* event) {
    InputCommon::GetKeyboard()->PressKey(event->key());
}

void GRenderWindow::keyReleaseEvent(QKeyEvent* event) {
    InputCommon::GetKeyboard()->ReleaseKey(event->key());
}

void GRenderWindow::mousePressEvent(QMouseEvent* event) {
    if (event->source() == Qt::MouseEventSynthesizedBySystem) {
        return; // touch input is handled in touchBeginEvent
    }

    auto pos{event->pos()};
    if (event->button() == Qt::LeftButton) {
        qreal pixel_ratio{windowPixelRatio()};
        TouchPressed(static_cast<unsigned>(pos.x() * pixel_ratio),
                     static_cast<unsigned>(pos.y() * pixel_ratio));
    } else if (event->button() == Qt::RightButton) {
        InputCommon::GetMotionEmu()->BeginTilt(pos.x(), pos.y());
    }
}

void GRenderWindow::mouseMoveEvent(QMouseEvent* event) {
    if (event->source() == Qt::MouseEventSynthesizedBySystem) {
        return; // touch input is handled in touchUpdateEvent
    }

    auto pos{event->pos()};
    qreal pixel_ratio{windowPixelRatio()};
    TouchMoved(std::max(static_cast<unsigned>(pos.x() * pixel_ratio), 0u),
               std::max(static_cast<unsigned>(pos.y() * pixel_ratio), 0u));
    InputCommon::GetMotionEmu()->Tilt(pos.x(), pos.y());
}

void GRenderWindow::mouseReleaseEvent(QMouseEvent* event) {
    if (event->source() == Qt::MouseEventSynthesizedBySystem) {
        return; // touch input is handled in touchEndEvent
    }

    if (event->button() == Qt::LeftButton) {
        TouchReleased();
    } else if (event->button() == Qt::RightButton) {
        InputCommon::GetMotionEmu()->EndTilt();
    }
}

void GRenderWindow::touchBeginEvent(QTouchEvent* event) {
    auto points{event->touchPoints()};
    auto tp{points.first()}; // there should only be 1 point in TouchBegin
    auto pos{tp.pos()};

    // Copied from mousePressEvent:
    qreal pixel_ratio{windowPixelRatio()};
    TouchPressed(static_cast<unsigned>(pos.x() * pixel_ratio),
                 static_cast<unsigned>(pos.y() * pixel_ratio));
}

void GRenderWindow::touchUpdateEvent(QTouchEvent* event) {
    qreal x{}, y{};
    int activePoints{};
    auto points{event->touchPoints()};

    for (int i{}; i < points.count(); i++) {
        auto tp{points[i]};

        if (tp.state() & (Qt::TouchPointPressed | Qt::TouchPointMoved | Qt::TouchPointStationary)) {
            activePoints++;

            x += tp.pos().x();
            y += tp.pos().y();
        }
    }

    x /= activePoints;
    y /= activePoints;

    // Copied from mouseMoveEvent:
    qreal pixel_ratio{windowPixelRatio()};
    TouchMoved(std::max(static_cast<unsigned>(x * pixel_ratio), 0u),
               std::max(static_cast<unsigned>(y * pixel_ratio), 0u));
}

void GRenderWindow::touchEndEvent(QTouchEvent* event) {
    // Copied from mouseReleaseEvent:
    TouchReleased();
}

bool GRenderWindow::event(QEvent* event) {
    if (event->type() == QEvent::TouchBegin) {
        touchBeginEvent(static_cast<QTouchEvent*>(event));
        return true;
    } else if (event->type() == QEvent::TouchUpdate) {
        touchUpdateEvent(static_cast<QTouchEvent*>(event));
        return true;
    } else if (event->type() == QEvent::TouchEnd || event->type() == QEvent::TouchCancel) {
        touchEndEvent(static_cast<QTouchEvent*>(event));
        return true;
    }

    return QWidget::event(event);
}

void GRenderWindow::focusOutEvent(QFocusEvent* event) {
    QWidget::focusOutEvent(event);
    InputCommon::GetKeyboard()->ReleaseAllKeys();
}

void GRenderWindow::OnClientAreaResized(unsigned width, unsigned height) {
    NotifyClientAreaSizeChanged(std::make_pair(width, height));
}

void GRenderWindow::InitRenderTarget() {
    if (child) {
        delete child;
    }

    if (layout()) {
        delete layout();
    }

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

    OnMinimalClientAreaChangeRequest(GetActiveConfig().min_client_area_size);

    OnFramebufferSizeChanged();
    NotifyClientAreaSizeChanged(std::pair<unsigned, unsigned>(child->width(), child->height()));

    BackupGeometry();
}

void GRenderWindow::CaptureScreenshot(u16 res_scale, const QString& screenshot_path) {
    if (!res_scale)
        res_scale = VideoCore::GetResolutionScaleFactor();
    const Layout::FramebufferLayout layout{Layout::FrameLayoutFromResolutionScale(res_scale)};
    screenshot_image = QImage(QSize(layout.width, layout.height), QImage::Format_RGB32);
    VideoCore::RequestScreenshot(screenshot_image.bits(),
                                 [=] {
                                     screenshot_image.mirrored(false, true).save(screenshot_path);
                                     LOG_INFO(Frontend, "The screenshot is saved.");
                                 },
                                 layout);
}

void GRenderWindow::OnMinimalClientAreaChangeRequest(
    const std::pair<unsigned, unsigned>& minimal_size) {
    setMinimumSize(minimal_size.first, minimal_size.second);
}

void GRenderWindow::OnEmulationStarting(EmuThread* emu_thread) {
    this->emu_thread = emu_thread;
    child->DisablePainting();
}

void GRenderWindow::OnEmulationStopping() {
    emu_thread = nullptr;
    child->EnablePainting();
}

void GRenderWindow::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);

    // windowHandle() is not initialized until the Window is shown, so we connect it here.
    connect(windowHandle(), &QWindow::screenChanged, this, &GRenderWindow::OnFramebufferSizeChanged,
            Qt::UniqueConnection);
}
