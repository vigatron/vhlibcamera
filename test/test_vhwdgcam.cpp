/**
 * @file test_vhwdgcam.cpp
 * @brief Simple smoke-test application for the VHWidgetCamera widget
 */

#include "vhwdgcam.hpp"

#include <QApplication>
#include <QDebug>
#include <QImage>

/**
 * @brief Test application entry point
 * Creates the VHWidgetCamera widget, opens the first available camera
 * and starts the frame stream.
 * @param argc Number of command line arguments
 * @param argv Command line arguments
 * @return Exit code of the application
 */
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Create the camera widget
    auto *cam = new VHWidgetCamera(nullptr);
    cam->show();

    // Frame consumption: forward to the debug output, errors are logged as well
    QObject::connect(cam, &VHWidgetCamera::frameReady, [](const QImage &img) {
        qDebug() << "Frame received:" << img.size();
    });

    QObject::connect(cam, &VHWidgetCamera::errorOccurred, [](const QString &msg) {
        qWarning() << "VHWidgetCamera error:" << msg;
    });

    // Open the first available camera and start streaming
    if (!cam->CameraOpen(0))
        return 1;

    cam->CameraStart();

    return app.exec();
}
