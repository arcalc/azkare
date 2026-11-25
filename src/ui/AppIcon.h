#pragma once
#include <QColor>
#include <QFont>
#include <QIcon>
#include <QPainter>
#include <QPixmap>

/** Draws the app icon in code (no SVG plugin needed): navy disc, blue ring, Arabic letter dhal. */
inline QIcon appIcon()
{
    QIcon icon;
    const int sizes[] = {16, 24, 32, 48, 64, 128, 256};
    for (int s : sizes) {
        QPixmap pm(s, s);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::TextAntialiasing);
        const qreal ring = qMax<qreal>(1.0, s / 16.0);
        p.setPen(QPen(QColor(0x52, 0x89, 0xAD), ring));
        p.setBrush(QColor(0x24, 0x3C, 0x4C));
        p.drawEllipse(QRectF(ring, ring, s - 2 * ring, s - 2 * ring));
        QFont f(QStringLiteral("Amiri"));
        f.setPixelSize(int(s * 0.60));
        p.setFont(f);
        p.setPen(QColor(0xF4, 0xFC, 0xFB));
        p.drawText(QRectF(0, -s * 0.04, s, s), Qt::AlignCenter, QString::fromUtf8("ذ"));
        icon.addPixmap(pm);
    }
    return icon;
}
