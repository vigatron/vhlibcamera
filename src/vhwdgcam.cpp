/* ======================================================================================
 * Library       : vhlibcamera
 * Description   : Qt6 camera capture widget with live preview and format selection
 * Revision      : 0.0.3
 * Source        : https://github.com/vigatron/vhlibcamera
 * Disclaimer    : Provided "AS IS", without warranty.
 * License       : MIT
 * File          : src/vhwdgcam.cpp
 * Content size  : 16560
 * Date / Time   : 07-11-2026 05:36:28
 * MD5           : 339f3ecd9b76769449daa0e3d6142066
 * Notes         : MD5 = file content without header/footer
 * Encoding      : UTF-8
 * Author        : Viktor Glebov / V01G04A81
 * Copyright     : © 2026 Viktor Glebov
 * ========================[ BEGIN FILE CONTENT ]====================================== */

#include "vhwdgcam.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include <QSignalBlocker>
#include <QVideoFrameFormat>
#include <QCloseEvent>
#include <QTimer>
#include <QDebug>

// ---------------------------------------------------------------------------
// Camera capture layer
// ---------------------------------------------------------------------------

/**
 * @brief Constructor
 * @param parent Parent widget (usually nullptr)
 */
VHWidgetCamera::VHWidgetCamera(QWidget *parent) : QWidget(parent)
{
    UiInit();
}

/**
 * @brief Destructor
 * Releases all captured resources synchronously: the widget is going away and
 * there is no event loop left to process a deferred deletion. The camera is
 * deinitialized even if the user never pressed Stop
 */
VHWidgetCamera::~VHWidgetCamera()
{
    ReleasePipeline(false);
}

/**
 * @brief Enumerate available cameras
 * Fills the internal camera list and prints device descriptions to debug output
 * @return Number of available cameras
 */
int VHWidgetCamera::Enumerate()
{
    camerasList = QMediaDevices::videoInputs();

    for (auto &item : camerasList)
    {
        qDebug() << "Camera: " << item.description();
    }

    return camerasList.size();
}

/**
 * @brief Open a camera by its index
 * Releases the previously opened camera, creates QCamera and QVideoSink,
 * connects frame processing and applies the selected video format
 * @param id Index of the camera in the available cameras list
 * @return true on success, false on error
 */
bool VHWidgetCamera::CameraOpen(int id)
{
    // Release previous resources before opening a new camera.
    // The release is done synchronously here: the device must be free
    // before a new QCamera is created on it
    ReleasePipeline(false);

    // The previous stream is gone, the real state is confirmed by the
    // activeChanged() signal of the new camera
    cameraRunning = false;
    cameraPaused = false;

    // There is nothing to open without a device
    if (camerasList.isEmpty())
    {
        ReportError("No cameras available");
        return false;
    }

    // Validate the camera index
    if (id < 0 || id >= camerasList.size())
    {
        ReportError(
            QString("Invalid camera index: %1 (max: %2)")
                .arg(id)
                .arg(camerasList.size() - 1));
        return false;
    }

    // Create the selected camera
    camera = new QCamera(camerasList[id]);
    session.setCamera(camera);

    sink = new QVideoSink;
    session.setVideoOutput(sink);

    QObject::connect(
        sink,
        &QVideoSink::videoFrameChanged,
        this,
        &VHWidgetCamera::ProcessFrame);

    // Forward hardware level errors to the listeners
    QObject::connect(
        camera,
        &QCamera::errorOccurred,
        this,
        [this](QCamera::Error err, const QString &txt)
        {
            if (err != QCamera::NoError)
                ReportError(txt);
        });

    // Keep the widget state in sync with the state of the backend:
    // the buttons must reflect what the camera really does
    QObject::connect(
        camera,
        &QCamera::activeChanged,
        this,
        &VHWidgetCamera::OnCameraActiveChanged);

    // Keep the formats list in sync with the camera being opened
    if (uiCameraIndex != id)
    {
        UiFillFormats(id);
    }

    // Apply the format chosen in the UI, fall back to the last supported one
    if (!ApplyFormat(cmbFormatList ? cmbFormatList->currentIndex() : -1))
    {
        ApplyFormat(formatsList.size() - 1);
    }

    UpdateControls();

    return true;
}

/**
 * @brief Close the camera
 * Stops the frame stream and defers the release of the capture pipeline to
 * the event loop. Destroying QCamera / QVideoSink directly inside a signal
 * handler blocks the GUI thread while the backend streaming thread is still
 * running, which looks exactly like a frozen widget
 * @return true always
 */
bool VHWidgetCamera::CameraClose()
{
    cameraRunning = false;
    cameraPaused = false;

    if (!camera && !sink)
    {
        UpdateControls();
        return true;
    }

    // Stop the stream now, release the resources later
    if (camera)
    {
        camera->stop();
    }

    // The pipeline destruction is deferred: the current call stack may be a
    // button handler and the backend needs the event loop to finish the
    // streaming thread. singleShot() is bound to this object and is dropped
    // automatically if the widget is destroyed
    QTimer::singleShot(0, this, [this]() { ReleasePipeline(true); });

    UpdateControls();

    return true;
}

/**
 * @brief Release the capture pipeline
 * Disconnects the frame handler, stops the camera, detaches the video output
 * and the camera from the capture session and destroys QVideoSink / QCamera.
 * Idempotent: calling it twice is safe
 * @param deferred true - destroy the objects through deleteLater(), so the
 *        call never blocks the GUI thread; false - destroy them immediately
 */
void VHWidgetCamera::ReleasePipeline(bool deferred)
{
    // No more frames may reach ProcessFrame()
    if (sink)
    {
        QObject::disconnect(
            sink,
            &QVideoSink::videoFrameChanged,
            this,
            &VHWidgetCamera::ProcessFrame);
    }

    if (camera)
    {
        camera->stop();
    }

    // The session must not keep pointers to the objects being destroyed
    session.setVideoOutput(nullptr);
    session.setCamera(nullptr);

    // Deferred destruction: the streaming thread of the backend may still
    // hold a reference to the sink or to the camera
    if (sink)
    {
        if (deferred)
        {
            sink->deleteLater();
        }
        else
        {
            delete sink;
        }

        sink = nullptr;
    }

    if (camera)
    {
        if (deferred)
        {
            camera->deleteLater();
        }
        else
        {
            delete camera;
        }

        camera = nullptr;
    }
}

/**
 * @brief Start the camera
 * Resumes the frame stream of an already opened camera
 */
void VHWidgetCamera::CameraStart()
{
    if (!camera)
    {
        ReportError("Camera is not opened");
        return;
    }

    cameraPaused = false;

    // The real state is confirmed by QCamera::activeChanged(),
    // cameraRunning is updated there
    camera->start();

    UpdateControls();
}

/**
 * @brief Stop the camera
 * Stops the frame stream but keeps the capture pipeline alive, so the next
 * CameraStart() resumes instantly and no backend teardown is triggered
 */
void VHWidgetCamera::CameraStop()
{
    // The capture pipeline must NOT be destroyed here: the GStreamer teardown
    // performed in the GUI thread waits for the streaming thread and blocks
    // the whole application. Resources are released by CameraClose()
    cameraRunning = false;
    cameraPaused = false;

    if (camera)
    {
        camera->stop();
    }

    UpdateControls();
}

/**
 * @brief Fill the video formats list for the given camera
 * Repopulates the formats combo, the combo signals are blocked meanwhile.
 * The last supported format is selected by default
 * @param camId Index of the camera in the available cameras list
 */
void VHWidgetCamera::UiFillFormats(int camId)
{
    if (!cmbFormatList)
    {
        return;
    }

    // The combo must not emit while it is being repopulated
    const QSignalBlocker blocker(cmbFormatList);

    cmbFormatList->clear();
    formatsList.clear();
    uiCameraIndex = -1;

    if (camId < 0 || camId >= camerasList.size())
    {
        return;
    }

    formatsList = camerasList[camId].videoFormats();
    if (formatsList.isEmpty())
    {
        ReportError("No video formats for the selected camera");
        return;
    }

    for (const QCameraFormat &fmt : formatsList)
    {
        const QSize res = fmt.resolution();
        cmbFormatList->addItem(
            QString("%1x%2 %3")
                .arg(res.width())
                .arg(res.height())
                .arg(QVideoFrameFormat::pixelFormatToString(fmt.pixelFormat())));
    }

    // The last supported format is applied by default
    cmbFormatList->setCurrentIndex(formatsList.size() - 1);

    uiCameraIndex = camId;
}

/**
 * @brief Apply a video format to the opened camera
 * Syncs the formats combo with the format actually applied by the backend
 * @param fmtId Index in the video formats list
 * @return true on success, false when the camera is closed or the index is invalid
 */
bool VHWidgetCamera::ApplyFormat(int fmtId)
{
    if (!camera || fmtId < 0 || fmtId >= formatsList.size())
    {
        return false;
    }

    camera->setCameraFormat(formatsList[fmtId]);

    // The backend may adjust the requested format, keep the UI in sync
    const int actualIdx = formatsList.indexOf(camera->cameraFormat());
    if (cmbFormatList && actualIdx >= 0 && actualIdx != cmbFormatList->currentIndex())
    {
        const QSignalBlocker blocker(cmbFormatList);
        cmbFormatList->setCurrentIndex(actualIdx);
    }

    return true;
}

/**
 * @brief Process a video frame
 * Converts the frame to an image, caches it for currentImage(),
 * updates the preview area and emits frameReady() for external listeners.
 * Frame updates are suppressed while the camera is stopped or paused
 * @param frame Video frame received from the capture session
 */
void VHWidgetCamera::ProcessFrame(const QVideoFrame &frame)
{

    if (!frame.isValid())
        return;

    // Frames that were already queued by the backend may arrive after the
    // stream has been stopped, and frame updates are suppressed while paused
    if (!cameraRunning || cameraPaused)
        return;

    // The frame is always rendered by the internal preview area,
    // frameReady() is emitted additionally for external listeners
    QImage img = frame.toImage();
    if (img.isNull())
        return;

    if (wdgImage)
    {
        wdgImage->SetImage(img);
    }

    // The last frame is cached for currentImage(): the cache survives
    // pause / stop, so a frame can be captured at any time
    imageCurrent = img;

    emit frameReady(img);
}

/**
 * @brief Report an error
 * Prints the message to the log and emits errorOccurred()
 * @param msg Human readable error description
 */
void VHWidgetCamera::ReportError(const QString &msg)
{
    qWarning() << "VHWidgetCamera:" << msg;
    emit errorOccurred(msg);
}

/**
 * @brief Update the control buttons state
 * Enables the start, the pause and the stop buttons according to the current state
 */
void VHWidgetCamera::UpdateControls()
{
    if (!btnCameraStart || !btnCameraStop)
    {
        return;
    }

    // Start is available while no frame stream is running
    btnCameraStart->setEnabled(!camerasList.isEmpty() && !cameraRunning);

    // Stop is available only while the frame stream is running
    btnCameraStop->setEnabled(cameraRunning);

    // Pause toggle is available only while the frame stream is running
    if (btnCameraPause)
    {
        btnCameraPause->setEnabled(cameraRunning);
        btnCameraPause->setText(cameraPaused ? "Camera Resume" : "Camera Pause");
    }
}

// ---------------------------------------------------------------------------
// Widget user interface
// ---------------------------------------------------------------------------

/**
 * @brief Initialize the widget
 * Creates the child widgets and places them into the layout
 */
void VHWidgetCamera::UiInit()
{
    UiInitObjects();

    UiInitLayout();

    UpdateControls();
}

/**
 * @brief Create the child widgets
 * Creates the camera and formats lists, the frame preview area
 * and the control buttons
 */
void VHWidgetCamera::UiInitObjects()
{

    // Available cameras list
    cmbCameraList = new QComboBox();

    Enumerate();

    for (const QCameraDevice &cam : camerasList)
    {
        cmbCameraList->addItem(cam.description());
    }

    // Supported video formats list
    cmbFormatList = new QComboBox();
    connect(
        cmbFormatList,
        &QComboBox::currentIndexChanged,
        this,
        &VHWidgetCamera::OnFormatSelected);

    // Formats of the initially selected camera
    UiFillFormats(cmbCameraList->currentIndex());

    // Frame preview area
    wdgImage = new VHWidgetCamImage();

    // Capture control buttons
    btnCameraStart = new QPushButton("Camera Start", this);
    connect(btnCameraStart, &QPushButton::clicked, this, &VHWidgetCamera::OnClickCameraStart);

    btnCameraPause = new QPushButton("Camera Pause", this);
    connect(btnCameraPause, &QPushButton::clicked, this, &VHWidgetCamera::OnClickCameraPause);

    btnCameraStop = new QPushButton("Camera Stop", this);
    connect(btnCameraStop, &QPushButton::clicked, this, &VHWidgetCamera::OnClickCameraStop);

    // Connect the camera selection handler after the list has been filled,
    // otherwise it would fire on every item insertion
    connect(
        cmbCameraList,
        &QComboBox::currentIndexChanged,
        this,
        &VHWidgetCamera::OnCameraSelected);
}

/**
 * @brief Arrange the child widgets
 * Builds the widget layout
 */
void VHWidgetCamera::UiInitLayout()
{
    // Capture control buttons layout
    auto hlayButtons = new QHBoxLayout();
    hlayButtons->addWidget(btnCameraStart);
    hlayButtons->addWidget(btnCameraPause);
    hlayButtons->addWidget(btnCameraStop);

    // Main widget layout
    auto vlayMain = new QVBoxLayout(this);
    vlayMain->addWidget(cmbCameraList);
    vlayMain->addWidget(cmbFormatList);
    vlayMain->addWidget(wdgImage);
    vlayMain->addLayout(hlayButtons);
}

/**
 * @brief Handler of the "Camera Start" button
 * Opens the selected camera if needed and starts the frame stream
 */
void VHWidgetCamera::OnClickCameraStart()
{
    int idx = cmbCameraList->currentIndex();

    // Open the camera by the selected index if it is not opened yet
    if (!camera)
    {
        if (!CameraOpen(idx))
            return;
    }

    CameraStart();
}

/**
 * @brief Handler of the "Camera Stop" button
 * Releases the camera capture pipeline, the next start reopens the camera
 */
void VHWidgetCamera::OnClickCameraStop()
{
    CameraStop();
}

/**
 * @brief Handler of the "Camera Pause" button
 * Toggles frame update suppression, the QVideoSink stream keeps running
 */
void VHWidgetCamera::OnClickCameraPause()
{
    cameraPaused = !cameraPaused;

    UpdateControls();
}

/**
 * @brief Handler of the camera selection change
 * Refreshes the video formats list and reopens the capture session
 * on the newly selected camera
 * @param idx Index of the selected camera
 */
void VHWidgetCamera::OnCameraSelected(int idx)
{
    if (idx < 0)
        return;

    // Refresh the formats list even if the camera is not opened yet
    UiFillFormats(idx);

    const bool wasRunning = cameraRunning;

    // Nothing to switch if the camera is not opened
    if (!camera)
        return;

    if (!CameraOpen(idx))
        return;

    if (wasRunning)
        CameraStart();
}

/**
 * @brief Handler of the camera active state change
 * Keeps the widget state and the control buttons in sync with the
 * state actually reported by the backend
 * @param active true when the camera is producing frames
 */
void VHWidgetCamera::OnCameraActiveChanged(bool active)
{
    // Ignore notifications coming from a camera that is not the current one
    // (for example, a device that is being released while a new one is
    // already opened)
    if (!camera || QObject::sender() != camera)
    {
        return;
    }

    cameraRunning = active;

    // A stopped or failed stream is never paused
    if (!active)
    {
        cameraPaused = false;
    }

    UpdateControls();
}

/**
 * @brief Widget close handler
 * Deinitializes the camera when the window is closed, even if the user never
 * pressed Stop: the frame stream is stopped and the capture pipeline is
 * released, so the device is never left busy
 * @param event Close event
 */
void VHWidgetCamera::closeEvent(QCloseEvent *event)
{
    CameraClose();

    QWidget::closeEvent(event);
}

/**
 * @brief Handler of the video format selection change
 * Applies the selected format to the opened camera
 * @param idx Index in the video formats list
 */
void VHWidgetCamera::OnFormatSelected(int idx)
{
    // Nothing to apply while the camera is closed, the selection is kept
    if (!camera || idx < 0)
        return;

    if (!ApplyFormat(idx))
    {
        ReportError("Cannot apply the selected video format");
    }
}

/* ========================[  END FILE CONTENT  ]========================
 * Library          : vhlibcamera
 * File             : src/vhwdgcam.cpp
 * Revision         : 0.0.3
 * Content size     : 16560
 * Date / Time      : 07-11-2026 05:36:28
 * MD5              : 339f3ecd9b76769449daa0e3d6142066
 * Copyright        : © 2026 Viktor Glebov
 * ====================================================================== */