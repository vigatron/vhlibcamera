/* ======================================================================================
 * Library       : vhlibcamera
 * Description   : Qt6 camera capture widget with live preview and format selection
 * Revision      : 0.0.2
 * Source        : https://github.com/vigatron/vhlibcamera
 * Disclaimer    : Provided "AS IS", without warranty.
 * License       : MIT
 * File          : src/vhwdgcamimg.hpp
 * Content size  : 411
 * Date / Time   : 06-11-2026 23:33:19
 * MD5           : f2a4f25eb756099968a22c620521ffde
 * Notes         : MD5 = file content without header/footer
 * Encoding      : UTF-8
 * Author        : Viktor Glebov / V01G04A81
 * Copyright     : © 2026 Viktor Glebov
 * ========================[ BEGIN FILE CONTENT ]====================================== */

#pragma once

#include <QImage>
#include <QPainter>
#include <QWidget>

class VHWidgetCamImage : public QWidget
{

public:
    explicit VHWidgetCamImage(QWidget *parent = nullptr);

    bool LoadImage(const QString &fname);
    bool SetImage(const QImage &newImage);
    const QImage &GetQImage() const { return image; }

protected:
    void paintEvent(QPaintEvent *e) override;

private:
    QImage image;
};

/* ========================[  END FILE CONTENT  ]========================
 * Library          : vhlibcamera
 * File             : src/vhwdgcamimg.hpp
 * Revision         : 0.0.2
 * Content size     : 411
 * Date / Time      : 06-11-2026 23:33:19
 * MD5              : f2a4f25eb756099968a22c620521ffde
 * Copyright        : © 2026 Viktor Glebov
 * ====================================================================== */