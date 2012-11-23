// Copyright (c) 2012 Oliver Lau <ola@ct.de>
// All rights reserved.

#include <QPainter>
#include <QtCore/QDebug>
#include "depthwidget.h"


DepthWidget::DepthWidget(QWidget* parent)
    : QWidget(parent)
    , mWindowAspectRatio(1.0)
    , mImageAspectRatio(1.0)
    , mFPS(0)
    , mDepthUnderCursor(-1)
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
}


void DepthWidget::resizeEvent(QResizeEvent* e)
{
    mWindowAspectRatio = (qreal)e->size().width() / e->size().height();
}


void DepthWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    if (mWindowAspectRatio < mImageAspectRatio) {
        const int h = int(width() / mImageAspectRatio);
        mDestRect = QRect(0, (height()-h)/2, width(), h);
    }
    else {
        const int w = int(height() * mImageAspectRatio);
        mDestRect = QRect((width()-w)/2, 0, w, height());
    }
    if (!mDepthFrame.isNull()) {
        p.fillRect(rect(), QColor(20, 20, 20));
        p.drawImage(mDestRect, mDepthFrame);
        p.setOpacity(0.6);
        p.setPen(Qt::red);
        p.setBrush(Qt::transparent);
        p.drawLine(0, height()/2, width(), height()/2);
        p.drawLine(width()/2, 0, width()/2, height());
        p.setOpacity(1.0);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(Qt::white);
        p.drawText(mDestRect, Qt::AlignRight | Qt::AlignBottom, tr("%1 fps").arg(mFPS, 0, 'f', 1));
        if (!mCursorPos.isNull()) {
            p.setBrush(QColor(100, 100, 250, 170));
            p.setPen(QColor(130, 130, 255, 220));
            QPointF posInImage = mDepthFrame.width() * (mCursorPos - mDestRect.topLeft()) / mDestRect.width();
            QRect box(mCursorPos - QPoint(30, 14), QSize(30, 14));
            p.setRenderHint(QPainter::Antialiasing, false);
            p.drawRect(box);
            const int depth = qGray(mDepthFrame.pixel(posInImage.toPoint()));
            p.setPen(QColor(10, 10, 40));
            p.setRenderHint(QPainter::Antialiasing);
            p.drawText(box, Qt::AlignHCenter | Qt::AlignVCenter, QString("%1").arg(depth));
            p.setPen(Qt::white);
            p.drawPoint(mCursorPos);
        }
    }
    else {
        p.fillRect(rect(), QColor(44, 8, 7));
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QColor(220, 40, 30));
        p.drawText(rect(), Qt::AlignHCenter | Qt::AlignVCenter, tr("Initializing sensor ... Please wait ..."));
    }
}


void DepthWidget::mouseMoveEvent(QMouseEvent* e)
{
    mCursorPos = e->pos();
}



void DepthWidget::setDepthFrame(const QImage& frame)
{
    mDepthFrame = frame;
    mImageAspectRatio = (qreal)mDepthFrame.width() / mDepthFrame.height();
    update();
}
