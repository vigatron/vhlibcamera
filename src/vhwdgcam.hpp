/* ======================================================================================
 * Library       : vhlibcamera
 * Description   : Qt6 camera capture widget with live preview and format selection
 * Revision      : 0.0.1
 * Source        : https://github.com/vigatron/vhlibcamera
 * Disclaimer    : Provided "AS IS", without warranty.
 * License       : MIT
 * File          : src/vhwdgcam.hpp
 * Content size  : 6235
 * Date / Time   : 02-11-2026 21:09:42
 * MD5           : 4e82fc02968cc369deff241e6d62f565
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
     * Stops the camera, disconnects all signals and releases allocated memory
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
     * Suspends the frame stream, camera resources are kept alive
     */
    void CameraStop();

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

private slots:

    // Handler of the "Camera Start" button
    void OnClickCameraStart();

    // Handler of the "Camera Stop" button
    void OnClickCameraStop();

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
     * Converts the frame to an image, updates the preview area and
     * emits frameReady() for external listeners
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
     * @brief Update the control buttons state
     * Enables the start or the stop button according to the current state
     */
    void UpdateControls();

    // ------------------------------------------------------------------
    // Widget user interface
    // ------------------------------------------------------------------

    QPushButton *btnCameraStart = nullptr;
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
 * Revision         : 0.0.1
 * Content size     : 6235
 * Date / Time      : 02-11-2026 21:09:42
 * MD5              : 4e82fc02968cc369deff241e6d62f565
 * Copyright        : © 2026 Viktor Glebov
 * ====================================================================== */