/* ======================================================================================
 * Library       : vhlibcamera
 * Description   : Qt6 camera capture widget with live preview and format selection
 * Revision      : 0.0.3
 * Source        : https://github.com/vigatron/vhlibcamera
 * Disclaimer    : Provided "AS IS", without warranty.
 * License       : MIT
 * File          : src/vhwdgcam.hpp
 * Content size  : 8678
 * Date / Time   : 07-11-2026 05:36:28
 * MD5           : 283ea33ff7e42e8c4745ca76d62c5f0b
 * Notes         : MD5 = file content without header/footer
 * Encoding      : UTF-8
 * Author        : Viktor Glebov / V01G04A81
 * Copyright     : © 2026 Viktor Glebov
 * ========================[ BEGIN FILE CONTENT ]====================================== */

#pragma once

#include <QCamera>
#include <QCameraFormat>
#include <QCameraDevice>
#include <QMediaDevices>
#include <QMediaCaptureSession>
#include <QVideoSink>
#include <QVideoFrame>
#include <QVideoFrameFormat>

#include <QComboBox>
#include <QPushButton>

#include "vhwdgcamimg.hpp"

class QCloseEvent;

/**
 * @brief Camera widget
 *
 * Combines the camera capture layer (device enumeration, capture session,
 * video sink and frame processing) with the user interface used to preview
 * incoming frames and to control the capture process.
 */
class VHWidgetCamera : public QWidget
{

    Q_OBJECT

public:

    /**
     * @brief Constructor
     * @param parent Parent widget (usually nullptr)
     */
    explicit VHWidgetCamera(QWidget *parent = nullptr);

    /**
     * @brief Destructor
     * Closes the camera and releases all captured resources
     */
    ~VHWidgetCamera();

    /**
     * @brief Open a camera by its index
     * Closes the previously opened camera, creates QCamera and QVideoSink,
     * connects frame processing and applies the selected video format
     * @param id Index of the camera in the available cameras list
     * @return true on success, false on error
     */
    bool CameraOpen(int id);

    /**
     * @brief Close the camera
     * Stops the frame stream and releases the capture pipeline. The actual
     * destruction of QCamera and QVideoSink is deferred to the event loop,
     * so the caller (usually a button handler) never blocks the GUI thread
     * @return true always
     */
    bool CameraClose();

    /**
     * @brief Start the camera
     * Resumes the frame stream of an already opened camera
     */
    void CameraStart();

    /**
     * @brief Stop the camera
     * Stops the frame stream but keeps the capture pipeline alive, so the
     * next CameraStart() resumes instantly. The pipeline is destroyed only
     * by CameraClose() or by the widget destructor
     */
    void CameraStop();

    /**
     * @brief Get the last captured frame
     * Returns the image of the last frame delivered by the capture session
     * and cached by ProcessFrame(). The cached image is kept when the stream
     * is paused or stopped, so the caller can capture the last known frame
     * @return Image of the last frame, a null image when no frame was received yet
     */
    const QImage &currentImage() const { return imageCurrent; }

signals:

    /**
     * @brief Frame ready signal
     * Emitted from ProcessFrame() when a new video frame has been decoded
     * @param img Image of the captured frame
     */
    void frameReady(const QImage &img);

    /**
     * @brief Error notification
     * Emitted when the camera cannot be opened, started or reports a device error
     * @param msg Human readable error description
     */
    void errorOccurred(const QString &msg);

protected:

    /**
     * @brief Widget close handler
     * Deinitializes the camera when the window is closed, even if the user
     * never pressed Stop: the frame stream is stopped and the capture
     * pipeline is released, so the device is never left busy
     * @param event Close event
     */
    void closeEvent(QCloseEvent *event) override;

private slots:

    // Handler of the "Camera Start" button
    void OnClickCameraStart();

    // Handler of the "Camera Stop" button
    void OnClickCameraStop();

    /**
     * @brief Handler of the "Camera Pause" button
     * Toggles frame update suppression, the QVideoSink stream keeps running
     */
    void OnClickCameraPause();

    /**
     * @brief Handler of the camera selection change
     * Refreshes the video formats list and reopens the capture session
     * on the newly selected camera
     * @param idx Index of the selected camera
     */
    void OnCameraSelected(int idx);

    /**
     * @brief Handler of the video format selection change
     * Applies the selected format to the opened camera
     * @param idx Index in the video formats list
     */
    void OnFormatSelected(int idx);

    /**
     * @brief Handler of the camera active state change
     * Keeps the widget state and the control buttons in sync with the
     * state actually reported by the backend
     * @param active true when the camera is producing frames
     */
    void OnCameraActiveChanged(bool active);

private:

    // ------------------------------------------------------------------
    // Camera capture layer
    // ------------------------------------------------------------------

    QCamera *camera = nullptr;
    QList<QCameraDevice> camerasList;
    QMediaCaptureSession session;
    QVideoSink *sink = nullptr;

    // True while the camera is producing frames
    bool cameraRunning = false;

    // True while frame updates to the preview are suppressed (paused)
    bool cameraPaused = false;

    // Cache of the last frame processed by ProcessFrame()
    QImage imageCurrent;

    // Formats supported by the camera currently listed in the UI
    QList<QCameraFormat> formatsList;

    // Index of the camera the formatsList belongs to, -1 when unknown
    int uiCameraIndex = -1;

    /**
     * @brief Enumerate available cameras
     * Fills the internal camera list and prints device descriptions to debug output
     * @return Number of available cameras
     */
    int Enumerate();

    /**
     * @brief Get the number of available cameras
     * @return Number of cameras in the internal list
     */
    int CameraCount() const { return camerasList.size(); }

    /**
     * @brief Get a reference to the camera device by its index
     * @param n Index of the camera in the available cameras list
     * @return Reference to the camera device
     */
    const QCameraDevice &GetCamera(int n) const { return camerasList[n]; }

    /**
     * @brief Fill the video formats list for the given camera
     * Repopulates the formats combo, the combo signals are blocked meanwhile.
     * The last supported format is selected by default
     * @param camId Index of the camera in the available cameras list
     */
    void UiFillFormats(int camId);

    /**
     * @brief Apply a video format to the opened camera
     * Syncs the formats combo with the format actually applied by the backend
     * @param fmtId Index in the video formats list
     * @return true on success, false when the camera is closed or the index is invalid
     */
    bool ApplyFormat(int fmtId);

    /**
     * @brief Process a video frame
     * Converts the frame to an image, caches it for currentImage(),
     * updates the preview area and emits frameReady() for external listeners
     * @param frame Video frame received from the capture session
     */
    void ProcessFrame(const QVideoFrame &frame);

    /**
     * @brief Report an error
     * Prints the message to the log and emits errorOccurred()
     * @param msg Human readable error description
     */
    void ReportError(const QString &msg);

    /**
     * @brief Release the capture pipeline
     * Disconnects the frame handler, stops the camera, detaches the video
     * output and the camera from the capture session and destroys
     * QVideoSink / QCamera. Idempotent: calling it twice is safe
     * @param deferred true - destroy the objects through deleteLater(), so
     *        the call never blocks the GUI thread; false - destroy them
     *        immediately (the destructor path, where there is no event loop
     *        left to process a deferred deletion)
     */
    void ReleasePipeline(bool deferred = true);

    /**
     * @brief Update the control buttons state
     * Enables the start, the pause and the stop buttons according to the current state
     */
    void UpdateControls();

    // ------------------------------------------------------------------
    // Widget user interface
    // ------------------------------------------------------------------

    QPushButton *btnCameraStart = nullptr;
    QPushButton *btnCameraPause = nullptr;
    QPushButton *btnCameraStop = nullptr;

    // Available cameras list
    QComboBox *cmbCameraList = nullptr;

    // Supported video formats list
    QComboBox *cmbFormatList = nullptr;

    // Frame preview area
    VHWidgetCamImage *wdgImage = nullptr;

    /**
     * @brief Initialize the widget
     * Creates the child widgets and places them into the layout
     */
    void UiInit();

    /**
     * @brief Create the child widgets
     * Creates the camera and formats lists, the frame preview area
     * and the control buttons
     */
    void UiInitObjects();

    /**
     * @brief Arrange the child widgets
     * Builds the widget layout
     */
    void UiInitLayout();
};

/* ========================[  END FILE CONTENT  ]========================
 * Library          : vhlibcamera
 * File             : src/vhwdgcam.hpp
 * Revision         : 0.0.3
 * Content size     : 8678
 * Date / Time      : 07-11-2026 05:36:28
 * MD5              : 283ea33ff7e42e8c4745ca76d62c5f0b
 * Copyright        : © 2026 Viktor Glebov
 * ====================================================================== */