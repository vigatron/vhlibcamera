/* ======================================================================================
 * Library       : vhlibcamera
 * Description   : Qt6 camera capture widget with live preview and format selection
 * Revision      : 0.0.1
 * Source        : https://github.com/vigatron/vhlibcamera
 * Disclaimer    : Provided "AS IS", without warranty.
 * License       : MIT
 * File          : src/vhwdgcam.cpp
 * Content size  : 11555
 * Date / Time   : 02-11-2026 21:09:42
 * MD5           : e4524afbb7a9ad726401112ce78e784f
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
 * Closes the camera and releases all captured resources
 */
VHWidgetCamera::~VHWidgetCamera()
{
    CameraClose();
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
 * Closes the previously opened camera, creates QCamera and QVideoSink,
 * connects frame processing and applies the selected video format
 * @param id Index of the camera in the available cameras list
 * @return true on success, false on error
 */
bool VHWidgetCamera::CameraOpen(int id)
{
    // Release previous resources before opening a new camera
    CameraClose();

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
 * Stops the camera, disconnects all signals and releases allocated memory
 * @return true always
 */
bool VHWidgetCamera::CameraClose()
{
    cameraRunning = false;

    if (!camera)
    {
        UpdateControls();
        return true;
    }

    // Stop the camera
    camera->stop();

    // Disconnect sink -> ProcessFrame
    if (sink)
    {
        QObject::disconnect(
            sink,
            &QVideoSink::videoFrameChanged,
            this,
            &VHWidgetCamera::ProcessFrame);

        // The session must not keep a pointer to the sink being deleted
        session.setVideoOutput(nullptr);

        delete sink;
        sink = nullptr;
    }

    // Destroy the camera
    delete camera;
    camera = nullptr;

    // Clear the capture session
    session.setCamera(nullptr);

    UpdateControls();

    return true;
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

    camera->start();
    cameraRunning = true;

    UpdateControls();
}

/**
 * @brief Stop the camera
 * Suspends the frame stream, camera resources are kept alive
 */
void VHWidgetCamera::CameraStop()
{
    cameraRunning = false;

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
 * Converts the frame to an image, updates the preview area and
 * emits frameReady() for external listeners
 * @param frame Video frame received from the capture session
 */
void VHWidgetCamera::ProcessFrame(const QVideoFrame &frame)
{

    if (!frame.isValid())
        return;

    // The frame is always rendered by the internal preview area,
    // frameReady() is emitted additionally for external listeners
    QImage img = frame.toImage();
    if (img.isNull())
        return;

    wdgImage->SetImage(img);
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
 * Enables the start or the stop button according to the current state
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
 * Suspends the frame stream, camera resources are kept alive
 */
void VHWidgetCamera::OnClickCameraStop()
{
    CameraStop();
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
 * Revision         : 0.0.1
 * Content size     : 11555
 * Date / Time      : 02-11-2026 21:09:42
 * MD5              : e4524afbb7a9ad726401112ce78e784f
 * Copyright        : © 2026 Viktor Glebov
 * ====================================================================== */