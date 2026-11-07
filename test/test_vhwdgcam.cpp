/**
 * @file test_vhwdgcam.cpp
 * @brief Simple smoke-test application for the VHWidgetCamera widget
 */

#include "vhwdgcam.hpp"

#include <QApplication>
#include <QDebug>
#include <QImage>
#include <QTimer>

/**
 * @brief Test application entry point
 *
 * Creates the VHWidgetCamera widget, opens the first available camera and
 * runs a scripted start/stop/close scenario. The scenario is a regression
 * test for the "GUI freezes on the Stop button" defect: every CameraStop()
 * call must return immediately and the event loop must stay responsive.
 *
 * Timeline:
 * - 0 ms    - camera opened and started
 * - 3000 ms - CameraStop()   (must not block the GUI thread)
 * - 4000 ms - CameraStart()  (resumes the stream without reopening)
 * - 6000 ms - CameraStop()
 * - 7000 ms - close() while the stream is running (Stop was "forgotten",
 *             the camera must still be deinitialized)
 * - 7500 ms - CameraOpen()   (the widget must be usable again)
 * - 11000 ms - widget deleted while the stream is running (the destructor
 *              must deinitialize the camera)
 * - 11500 ms - application quits
 *
 * @param argc Number of command line arguments
 * @param argv Command line arguments
 * @return Exit code of the application
 */
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // The test drives the application lifetime explicitly:
    // closing the last window must not stop the scenario in the middle
    QGuiApplication::setQuitOnLastWindowClosed(false);

    // Create the camera widget
    auto *cam = new VHWidgetCamera(nullptr);
    cam->setWindowTitle("VHWidgetCamera smoke test");
    cam->show();

    // Number of frames received from the capture stream
    qint64 frameCount = 0;

    // Frame consumption: forward to the debug output, errors are logged as well
    QObject::connect(cam, &VHWidgetCamera::frameReady, [&frameCount](const QImage &img) {
        ++frameCount;
        qDebug() << "Frame received:" << img.size();
    });

    QObject::connect(cam, &VHWidgetCamera::errorOccurred, [](const QString &msg) {
        qWarning() << "VHWidgetCamera error:" << msg;
    });

    // Open the first available camera and start streaming
    if (!cam->CameraOpen(0))
        return 1;

    cam->CameraStart();

    // --- Stop / resume scenario -------------------------------------------
    QTimer::singleShot(3000, cam, [cam]() {
        qDebug() << "SmokeTest: CameraStop() #1 -> begin";
        cam->CameraStop();
        qDebug() << "SmokeTest: CameraStop() #1 -> returned";
    });

    QTimer::singleShot(4000, cam, [cam]() {
        qDebug() << "SmokeTest: CameraStart() #2 -> begin";
        cam->CameraStart();
        qDebug() << "SmokeTest: CameraStart() #2 -> returned";
    });

    QTimer::singleShot(6000, cam, [cam]() {
        qDebug() << "SmokeTest: CameraStop() #2 -> begin";
        cam->CameraStop();
        qDebug() << "SmokeTest: CameraStop() #2 -> returned";
    });

    // Closing the window MUST deinitialize the camera even though the stream
    // is running: the user "forgot" to press Stop in this scenario
    QTimer::singleShot(7000, cam, [cam, &frameCount]() {
        qDebug() << "SmokeTest: close() while RUNNING -> begin";
        cam->close();
        qDebug() << "SmokeTest: close() while RUNNING -> returned (frames:" << frameCount << ")";
    });

    // After the deferred release the widget must be usable again:
    // open the device from scratch and resume the stream
    QTimer::singleShot(7500, cam, [cam, &frameCount]() {
        qDebug() << "SmokeTest: reopen + start -> begin";
        cam->show();

        if (!cam->CameraOpen(0))
        {
            qWarning() << "SmokeTest: reopen FAILED";
            return;
        }

        cam->CameraStart();
        qDebug() << "SmokeTest: reopen + start -> returned (frames:" << frameCount << ")";
    });

    // The destructor must deinitialize the camera even though the stream is
    // running: the user "forgot" to press Stop and the widget is destroyed
    QTimer::singleShot(11000, cam, [cam, &frameCount]() {
        qDebug() << "SmokeTest: delete widget while RUNNING -> begin (frames:" << frameCount << ")";
        delete cam;
        qDebug() << "SmokeTest: delete widget while RUNNING -> returned";
    });

    QTimer::singleShot(11500, &app, []() {
        qDebug() << "SmokeTest: quit";
        QCoreApplication::quit();
    });

    const int rc = app.exec();

    qDebug() << "SmokeTest: frames received =" << frameCount;
    qDebug() << "SmokeTest: exit code =" << rc;

    return rc;
}
