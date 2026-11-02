# vhlibcamera

**A Qt6 / C++23 camera selector widget with live preview and capture controls.**

`vhlibcamera` is a small, self-contained library that wraps the Qt6 Multimedia camera stack into a ready-to-use widget. Drop `VHWidgetCamera` into any Qt Widgets application and you get camera enumeration, video-format selection, a live frame preview and start/stop control buttons without writing the capture plumbing yourself. Internally the widget owns a `QMediaCaptureSession`, a `QCamera` and a `QVideoSink`; incoming `QVideoFrame` objects are converted to `QImage` and pushed to the preview area, while the same image is re-emitted through the `frameReady()` signal so the rest of your application can consume it.

The widget is built from two classes. `VHWidgetCamera` is the public entry point: it enumerates available devices through `QMediaDevices::videoInputs()`, fills the camera and format combo boxes, and exposes a compact API — `CameraOpen(int)`, `CameraClose()`, `CameraStart()` and `CameraStop()` — for programmatic control. `WidgetImage` is the lightweight preview surface: a `QWidget` that paints either a centered "No Image" placeholder or the most recent frame, scaled to fit, with a soft drop shadow. Formats are listed as `WIDTHxHEIGHT PIXELFORMAT`, the last supported format is selected by default, and the combo box is kept in sync with whatever format the backend actually applied, since camera drivers are free to adjust a requested format.

All capture state changes are reflected in the UI: the Start button is enabled only when a camera is available and no stream is running, the Stop button only while frames are flowing. Selecting a different camera reopens the capture session on the new device and resumes streaming automatically if it was running before; selecting a different format applies it immediately to the open camera. Hardware and configuration problems never fail silently — they are reported through the `errorOccurred(const QString &)` signal and mirrored to the Qt debug log.

## Features

- Camera enumeration via `QMediaDevices::videoInputs()`
- Video-format selection per camera, synced with the backend's actual choice
- Live preview with aspect-agnostic scaling and an empty-state placeholder
- `frameReady(const QImage &)` signal for downstream processing
- `errorOccurred(const QString &)` signal for device and configuration errors
- Automatic reopen and stream resume when the camera selection changes
- Clean resource release: the sink is detached from the session before deletion

## Requirements

- Qt 6 (Core, Widgets, Multimedia)
- A C++23-capable compiler
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

## License

MIT. Provided "AS IS", without warranty of any kind.

Copyright © 2026 Viktor Glebov (V01G04A81).
