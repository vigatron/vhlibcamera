/* ======================================================================================
 * Library       : vhlibcamera
 * Description   : Qt6 camera capture widget with live preview and format selection
 * Revision      : 0.0.2
 * Source        : https://github.com/vigatron/vhlibcamera
 * Disclaimer    : Provided "AS IS", without warranty.
 * License       : MIT
 * File          : src/vhwdgcamimg.cpp
 * Content size  : 1994
 * Date / Time   : 06-11-2026 23:07:24
 * MD5           : 2de0eb5e59eca6a0d5fe26e6202287c2
 * Notes         : MD5 = file content without header/footer
 * Encoding      : UTF-8
 * Author        : Viktor Glebov / V01G04A81
 * Copyright     : © 2026 Viktor Glebov
 * ========================[ BEGIN FILE CONTENT ]====================================== */

#include "vhwdgcamimg.hpp"

#include <QPushButton>

#include <QGraphicsDropShadowEffect>

/**
 * 
 */
VHWidgetCamImage::VHWidgetCamImage(QWidget * parent)
    : QWidget(parent), image(nullptr) {

    setMinimumSize(640, 480);

    // DropShadowEffect для мягкой тени
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect;
    shadow->setBlurRadius(32);
    shadow->setOffset(4, 4);
    shadow->setColor(Qt::darkGray);
    setGraphicsEffect(shadow);

}

/**
 * 
 */
bool VHWidgetCamImage::LoadImage(const QString & fname) {
   
    bool r = image.load(fname);
    update();
    return r;
}

/**
 * 
 */
void VHWidgetCamImage::paintEvent(QPaintEvent *e) {

    QPainter p(this);

    if(!image.width()) {
        // Заливка всего виджета белым цветом
        // QColor(0x50, 0x50, 0x50)
        QColor defaultColor = palette().color(QPalette::Window);
        QColor textColor    = palette().color(QPalette::Text);
        p.fillRect(rect(), defaultColor);

        // Настройка шрифта и цвета текста
        QFont font = p.font();
        font.setPointSize(12);
        p.setFont(font);
        p.setPen(textColor);

        // Отрисовка текста по центру
        p.drawText(rect(), Qt::AlignCenter, "No Image");
        return;
    }

    QRect bufrect = QRect(0, 0, image.width() , image.height());
    QRect scrrect = rect();
    const int flag_resize = 1;

    QImage convImage = image.convertToFormat(QImage::Format_RGB32, Qt::ThresholdDither | Qt::AutoColor);
    p.drawImage(scrrect, convImage, bufrect);

}

/**
 * 
 */
bool VHWidgetCamImage::SetImage(const QImage & newImage) {

    // нельзя установить пустое изображение
    if (newImage.isNull()) {
        return false;
    }

    // создаём копию + проверка на случай нехватки памяти
    image = newImage.copy();

    update();
    return true;
}

/* ========================[  END FILE CONTENT  ]========================
 * Library          : vhlibcamera
 * File             : src/vhwdgcamimg.cpp
 * Revision         : 0.0.2
 * Content size     : 1994
 * Date / Time      : 06-11-2026 23:07:24
 * MD5              : 2de0eb5e59eca6a0d5fe26e6202287c2
 * Copyright        : © 2026 Viktor Glebov
 * ====================================================================== */