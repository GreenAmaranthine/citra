// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <atomic>
#include <QGLWidget>
#include <QImage>
#include <QThread>
#include "common/common_types.h"
#include "core/core.h"
#include "core/framebuffer_layout.h"

class QKeyEvent;
class QScreen;

class GGLWidgetInternal;
class GMainWindow;
class Screens;
class QTouchEvent;

class EmuThread : public QThread {
    Q_OBJECT

public:
    explicit EmuThread(Screens* screens);

    /**
     * Start emulation (on new thread)
     * @warning Only call when not running!
     */
    void run() override;

    /**
     * Requests for the emulation thread to stop running
     */
    void RequestStop() {
        stop_run = true;
        Core::System::GetInstance().SetRunning(false);
    };

private:
    bool running{};
    std::atomic<bool> stop_run{false};

    Screens* screens;

signals:
    void ErrorThrown(Core::System::ResultStatus, const std::string&);
};

class Screens : public QWidget {
    Q_OBJECT

public:
    Screens(QWidget* parent, EmuThread* emu_thread);
    ~Screens();

    /**
     * Clip the provided coordinates to be inside the touchscreen area.
     */
    std::tuple<unsigned, unsigned> ClipToTouchScreen(unsigned new_x, unsigned new_y);

    /**
     * Signal that a touch pressed event has occurred (e.g. mouse click pressed)
     * @param framebuffer_x Framebuffer x-coordinate that was pressed
     * @param framebuffer_y Framebuffer y-coordinate that was pressed
     */
    void TouchPressed(unsigned framebuffer_x, unsigned framebuffer_y);

    /// Signal that a touch released event has occurred (e.g. mouse click released)
    void TouchReleased();

    /**
     * Signal that a touch movement event has occurred (e.g. mouse was moved over the emu window)
     * @param framebuffer_x Framebuffer x-coordinate
     * @param framebuffer_y Framebuffer y-coordinate
     */
    void TouchMoved(unsigned framebuffer_x, unsigned framebuffer_y);

    void SwapBuffers();
    void MakeCurrent();
    void DoneCurrent();
    void UpdateCurrentFramebufferLayout(unsigned int, unsigned int);

    void BackupGeometry();
    void RestoreGeometry();
    void restoreGeometry(const QByteArray& geometry); // overridden
    QByteArray saveGeometry();                        // overridden

    qreal windowPixelRatio() const;

    void closeEvent(QCloseEvent* event) override;

    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

    bool event(QEvent* event) override;

    void focusOutEvent(QFocusEvent* event) override;

    void InitRenderTarget();

    void CaptureScreenshot(u16 res_scale, const QString& screenshot_path);

public slots:
    void moveContext(); // overridden

    void OnEmulationStarting(EmuThread* emu_thread);
    void OnEmulationStopping();
    void OnFramebufferSizeChanged();

signals:
    /// Emitted when the window is closed
    void Closed();

private:
    std::pair<unsigned, unsigned> ScaleTouch(const QPointF pos) const;
    void TouchBeginEvent(const QTouchEvent* event);
    void TouchUpdateEvent(const QTouchEvent* event);
    void TouchEndEvent();

    const Layout::FramebufferLayout& GetFramebufferLayout() {
        return framebuffer_layout;
    }

    GGLWidgetInternal* child{};

    QByteArray geometry;

    EmuThread* emu_thread{};

    /// Temporary storage of the screenshot taken
    QImage screenshot_image;

    Layout::FramebufferLayout framebuffer_layout; ///< Current framebuffer layout

    class TouchState;
    std::shared_ptr<TouchState> touch_state;

protected:
    void showEvent(QShowEvent* event) override;
};
