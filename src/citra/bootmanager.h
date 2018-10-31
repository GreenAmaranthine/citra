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
#include "core/frontend.h"

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
    std::atomic_bool stop_run{};

    Screens* screens;

signals:
    void ErrorThrown(Core::System::ResultStatus, const std::string&);
};

class Screens : public QWidget, public Frontend {
    Q_OBJECT

public:
    explicit Screens(QWidget* parent, EmuThread* emu_thread);
    ~Screens();

    void SwapBuffers() override;
    void MakeCurrent() override;
    void DoneCurrent() override;

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

    void TouchChanged(unsigned, unsigned);

private:
    std::pair<unsigned, unsigned> ScaleTouch(const QPointF pos) const;
    void TouchBeginEvent(const QTouchEvent* event);
    void TouchUpdateEvent(const QTouchEvent* event);
    void TouchEndEvent();

    GGLWidgetInternal* child{};

    QByteArray geometry;

    EmuThread* emu_thread;

    /// Temporary storage of the screenshot taken
    QImage screenshot_image;

protected:
    void showEvent(QShowEvent* event) override;
};
