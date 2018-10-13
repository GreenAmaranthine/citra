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

class Screens::TouchState : public Input::Factory<Input::TouchDevice>,
                            public std::enable_shared_from_this<TouchState> {
public:
    std::unique_ptr<Input::TouchDevice> Create(const Common::ParamPackage&) override {
        return std::make_unique<Device>(shared_from_this());
    }

    std::mutex mutex;

    bool touch_pressed{}; ///< True if touchpad area is currently pressed, otherwise false

    float touch_x{}; ///< Touchpad X-position
    float touch_y{}; ///< Touchpad Y-position

private:
    class Device : public Input::TouchDevice {
    public:
        explicit Device(std::weak_ptr<TouchState>&& touch_state) : touch_state{touch_state} {}
        std::tuple<float, float, bool> GetStatus() const override {
            if (auto state{touch_state.lock()}) {
                std::lock_guard<std::mutex> guard{state->mutex};
                return std::make_tuple(state->touch_x, state->touch_y, state->touch_pressed);
            }
            return std::make_tuple(0.0f, 0.0f, false);
        }

    private:
        std::weak_ptr<TouchState> touch_state;
    };
};

Screens::Screens(QWidget* parent, EmuThread* emu_thread) : QWidget{parent}, emu_thread{emu_thread} {

    touch_state = std::make_shared<TouchState>();
    Input::RegisterFactory<Input::TouchDevice>("emu_window", touch_state);

    setAttribute(Qt::WA_AcceptTouchEvents);

    InputCommon::Init();

    Frontend::Set_GetFramebufferLayout(
        [&]() -> const Layout::FramebufferLayout& { return GetFramebufferLayout(); });

    Frontend::Set_SwapBuffers([&]() { SwapBuffers(); });

    Frontend::Set_MakeCurrent([&]() { MakeCurrent(); });

    Frontend::Set_UpdateCurrentFramebufferLayout([&](unsigned int width, unsigned int height) {
        UpdateCurrentFramebufferLayout(width, height);
    });
}

Screens::~Screens() {
    Input::UnregisterFactory<Input::TouchDevice>("emu_window");
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
        return; // touch input is handled in TouchBeginEvent

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
        return; // touch input is handled in TouchUpdateEvent

    auto pos{event->pos()};
    const auto [x, y]{ScaleTouch(pos)};
    TouchMoved(x, y);
    InputCommon::GetMotionEmu()->Tilt(pos.x(), pos.y());
}

void Screens::mouseReleaseEvent(QMouseEvent* event) {
    if (event->source() == Qt::MouseEventSynthesizedBySystem)
        return; // touch input is handled in TouchEndEvent

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

    // average all active touch points
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

    setMinimumSize(400, 480);

    OnFramebufferSizeChanged();

    BackupGeometry();
}

void Screens::CaptureScreenshot(u16 res_scale, const QString& screenshot_path) {
    if (!res_scale) {
        res_scale = VideoCore::GetResolutionScaleFactor();
    }

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

/**
 * Check if the given x/y coordinates are within the touchpad specified by the framebuffer layout
 * @param layout FramebufferLayout object describing the framebuffer size and screen positions
 * @param framebuffer_x Framebuffer x-coordinate to check
 * @param framebuffer_y Framebuffer y-coordinate to check
 * @return True if the coordinates are within the touchpad, otherwise false
 */
static bool IsWithinTouchscreen(const Layout::FramebufferLayout& layout, unsigned framebuffer_x,
                                unsigned framebuffer_y) {
    if (Settings::values.factor_3d != 0) {
        return (framebuffer_y >= layout.bottom_screen.top &&
                framebuffer_y < layout.bottom_screen.bottom &&
                framebuffer_x >= layout.bottom_screen.left / 2 &&
                framebuffer_x < layout.bottom_screen.right / 2);
    } else {
        return (framebuffer_y >= layout.bottom_screen.top &&
                framebuffer_y < layout.bottom_screen.bottom &&
                framebuffer_x >= layout.bottom_screen.left &&
                framebuffer_x < layout.bottom_screen.right);
    }
}

std::tuple<unsigned, unsigned> Screens::ClipToTouchScreen(unsigned new_x, unsigned new_y) {
    new_x = std::max(new_x, framebuffer_layout.bottom_screen.left);
    new_x = std::min(new_x, framebuffer_layout.bottom_screen.right - 1);

    new_y = std::max(new_y, framebuffer_layout.bottom_screen.top);
    new_y = std::min(new_y, framebuffer_layout.bottom_screen.bottom - 1);

    return std::make_tuple(new_x, new_y);
}

void Screens::TouchPressed(unsigned framebuffer_x, unsigned framebuffer_y) {
    if (!IsWithinTouchscreen(framebuffer_layout, framebuffer_x, framebuffer_y)) {
        return;
    }

    std::lock_guard<std::mutex> guard{touch_state->mutex};
    if (Settings::values.factor_3d != 0) {
        touch_state->touch_x =
            static_cast<float>(framebuffer_x - framebuffer_layout.bottom_screen.left / 2) /
            (framebuffer_layout.bottom_screen.right / 2 -
             framebuffer_layout.bottom_screen.left / 2);
    } else {
        touch_state->touch_x =
            static_cast<float>(framebuffer_x - framebuffer_layout.bottom_screen.left) /
            (framebuffer_layout.bottom_screen.right - framebuffer_layout.bottom_screen.left);
    }
    touch_state->touch_y =
        static_cast<float>(framebuffer_y - framebuffer_layout.bottom_screen.top) /
        (framebuffer_layout.bottom_screen.bottom - framebuffer_layout.bottom_screen.top);

    touch_state->touch_pressed = true;
}

void Screens::TouchReleased() {
    std::lock_guard<std::mutex> guard{touch_state->mutex};
    touch_state->touch_pressed = false;
    touch_state->touch_x = 0;
    touch_state->touch_y = 0;
}

void Screens::TouchMoved(unsigned framebuffer_x, unsigned framebuffer_y) {
    if (!touch_state->touch_pressed) {
        return;
    }

    if (!IsWithinTouchscreen(framebuffer_layout, framebuffer_x, framebuffer_y)) {
        std::tie(framebuffer_x, framebuffer_y) = ClipToTouchScreen(framebuffer_x, framebuffer_y);
    }

    TouchPressed(framebuffer_x, framebuffer_y);
}

void Screens::UpdateCurrentFramebufferLayout(unsigned width, unsigned height) {
    Layout::FramebufferLayout layout;
    if (Settings::values.custom_layout) {
        layout = Layout::CustomFrameLayout(width, height, Settings::values.swap_screen);
    } else {
        switch (Settings::values.layout_option) {
        case Settings::LayoutOption::SingleScreen:
            layout = Layout::SingleFrameLayout(width, height, Settings::values.swap_screen);
            break;
        case Settings::LayoutOption::MediumScreen:
            layout = Layout::MediumFrameLayout(width, height, Settings::values.swap_screen);
            break;
        case Settings::LayoutOption::LargeScreen:
            layout = Layout::LargeFrameLayout(width, height, Settings::values.swap_screen);
            break;
        case Settings::LayoutOption::SideScreen:
            layout = Layout::SideFrameLayout(width, height, Settings::values.swap_screen);
            break;
        case Settings::LayoutOption::Default:
        default:
            layout = Layout::DefaultFrameLayout(width, height, Settings::values.swap_screen);
            break;
        }
    }
    resize(width, height);
    framebuffer_layout = std::move(layout);
}

void Screens::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);

    // windowHandle() is not initialized until the Window is shown, so we connect it here.
    connect(windowHandle(), &QWindow::screenChanged, this, &Screens::OnFramebufferSizeChanged,
            Qt::UniqueConnection);
}
