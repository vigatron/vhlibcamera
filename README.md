# vhlibcamera

**A Qt6 / C++17 camera selector widget with live preview and capture controls.**

![Language](https://img.shields.io/badge/C%2B%2B-17-blue.svg)
![Platform](https://img.shields.io/badge/platform-Desktop-lightgrey.svg)
![License](https://img.shields.io/badge/License-MIT-yellow.svg)
![Qt](https://img.shields.io/badge/Qt-6-00599C.svg)
![Version](https://img.shields.io/badge/version-0.0.3-orange.svg)
![Author](https://img.shields.io/badge/Author-Viktor%20Glebov-red.svg)

| Property | Value |
|---|---|
| **Library** | `vhlibcamera` |
| **Revision** | `0.0.3` |
| **Language** | C++17 |
| **Author** | Viktor Glebov / V01G04A81 |
| **License** | MIT |
| **Dependency** | Qt 6 (Core, Widgets, Multimedia) |

`vhlibcamera` is a small, self-contained library that wraps the Qt6 Multimedia camera stack into a ready-to-use widget. Drop `VHWidgetCamera` into any Qt Widgets application and you get camera enumeration, video-format selection, a live frame preview and start/stop control buttons without writing the capture plumbing yourself. Internally the widget owns a `QMediaCaptureSession`, a `QCamera` and a `QVideoSink`; incoming `QVideoFrame` objects are converted to `QImage` and pushed to the preview area, while the same image is re-emitted through the `frameReady()` signal so the rest of your application can consume it.

The widget is built from two classes. `VHWidgetCamera` is the public entry point: it enumerates available devices through `QMediaDevices::videoInputs()`, fills the camera and format combo boxes, and exposes a compact API — `CameraOpen(int)`, `CameraClose()`, `CameraStart()`, `CameraStop()` and the pause toggle control — for programmatic control. `VHWidgetCamImage` is the lightweight preview surface: a `QWidget` that paints either a centered "No Image" placeholder or the most recent frame, scaled to fit, with a soft drop shadow. Formats are listed as `WIDTHxHEIGHT PIXELFORMAT`, the last supported format is selected by default, and the combo box is kept in sync with whatever format the backend actually applied, since camera drivers are free to adjust a requested format.

All capture state changes are reflected in the UI: the Start button is enabled only when a camera is available and no stream is running, the Stop button only while frames are flowing, the Pause button toggles frame update suppression while the underlying `QVideoSink` stream keeps running. The widget state follows the state actually reported by the backend (`QCamera::activeChanged`), so a failed start or a device error never leaves the controls in a wrong state. Selecting a different camera reopens the capture session on the new device and resumes streaming automatically if it was running before; selecting a different format applies it immediately to the open camera. Hardware and configuration problems never fail silently — they are reported through the `errorOccurred(const QString &)` signal and mirrored to the Qt debug log.

Pressing the Stop button stops the frame stream but keeps the capture pipeline alive: no backend teardown is performed, the call returns immediately and the next Start resumes the stream without reopening the device. The pipeline is released asynchronously (`CameraClose()`, the widget `closeEvent()` and the destructor): the `QCamera` / `QVideoSink` objects are detached from the capture session first and destroyed through `deleteLater()`, so the GUI thread is never blocked by the GStreamer teardown while the streaming thread is still running. Closing the window always deinitializes the camera — even if the user never pressed Stop — and the destructor releases the pipeline synchronously, so the device is never left busy.

## Features

- Camera enumeration via `QMediaDevices::videoInputs()`
- Video-format selection per camera, synced with the backend's actual choice
- Live preview with aspect-agnostic scaling and an empty-state placeholder
- `frameReady(const QImage &)` signal for downstream processing
- `errorOccurred(const QString &)` signal for device and configuration errors
- Pause/resume frame update suppression without stopping the underlying stream
- Automatic reopen and stream resume when the camera selection changes
- Non-blocking stop: the capture pipeline is kept alive, no backend teardown in the GUI thread
- Deferred, race-free pipeline release (`deleteLater()` after detaching the capture session)
- `closeEvent()` deinitializes the camera even if Stop was not pressed, closing the window never leaves the device busy
- Widget state synced with the backend through `QCamera::activeChanged`

## Requirements

- Qt 6 (Core, Widgets, Multimedia)
- A C++17-capable compiler
- CMake or qmake project linking against `Qt6::Multimedia` and `Qt6::Widgets`

## Minimal usage

```cpp
#include "vhwdgcam.hpp"

auto *cam = new VHWidgetCamera(parent);

QObject::connect(cam, &VHWidgetCamera::frameReady,
                 [](const QImage &img) { /* consume the frame */ });

QObject::connect(cam, &VHWidgetCamera::errorOccurred,
                 [](const QString &msg) { qWarning() << msg; });

cam->CameraOpen(0);   // open the first available camera
cam->CameraStart();   // begin streaming
```

## Tests

The `test/` directory contains a minimal test application:

- `test_vhwdgcam` — creates a `VHWidgetCamera`, opens the first available camera, starts the frame stream and runs a scripted start / stop / resume / close / reopen / destroy scenario. The scenario is a regression test for the "GUI freezes on the Stop button" defect: every `CameraStop()` call must return immediately and the event loop must stay responsive. The test finishes on its own after about 12 seconds.

Build and run it with the test target enabled (from the module root):

```sh
cd src/extmods/vhlibcamera
cmake -B build -DMYLIB_BUILD_TESTS=ON
cmake --build build --target test_vhwdgcam
```

## License

MIT. Provided "AS IS", without warranty of any kind.

Copyright © 2026 Viktor Glebov (V01G04A81).
