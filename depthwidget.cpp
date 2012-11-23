// Copyright (c) 2012 Oliver Lau <ola@ct.de>
// All rights reserved.

#include <QPainter>
#include <QtCore/QDebug>
#include <qmath.h>
#include "depthwidget.h"
#include "string.h"

DepthWidget::DepthWidget(QWidget* parent)
    : QWidget(parent)
    , mWindowAspectRatio(1.0)
    , mImageAspectRatio(1.0)
    , mFPS(0)
    , mHFOV(45.0)
    , mVFOV(45.0)
    , mU(0.707107)
    , mV(0.707107)
    , mDepthUnderCursor(-1)
    , mStartupTimerId(0)
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    mStartupTime.start();
    mStartupTimerId = startTimer(1000/25);
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
        p.drawText(mDestRect.translated(-3, -3), Qt::AlignRight | Qt::AlignBottom, tr("%1 fps").arg(mFPS, 0, 'f', 1));
        if (!mCursorPos.isNull()) {
            p.setBrush(QColor(100, 100, 250, 170));
            p.setPen(QColor(130, 130, 255, 220));
            const QRect box(mCursorPos - QPoint(85, 44), QSize(85, 44));
            const QRect textBox(box.topLeft() + QPoint(3, 3), box.bottomRight() - QPoint(3, 3));
            p.setRenderHint(QPainter::Antialiasing, false);
            p.drawRect(box);
            const QPoint posInImage = QPointF((mCursorPos - mDestRect.topLeft()) * mDepthFrame.width() / mDestRect.width()).toPoint();
            const qreal z = mDepth.at(posInImage.x() + posInImage.y() * mDepthFrame.width());
            const qreal u = qreal(posInImage.x()) / mDepthFrame.width() - 0.5;
            const qreal v = qreal(posInImage.y()) / mDepthFrame.height() - 0.5;
            const qreal x = z * u / mU;
            const qreal y = z * v / mV;
            p.setPen(QColor(10, 10, 40));
            p.setRenderHint(QPainter::Antialiasing);
            p.drawText(textBox, Qt::AlignRight | Qt::AlignVCenter,
                       QString("x = %1 cm\n"
                               "y = %2 cm\n"
                               "z = %3 cm")
                       .arg(1e-1 * x, 0, 'f', 1)
                       .arg(1e-1 * y, 0, 'f', 1)
                       .arg(1e-1 * z, 0, 'f', 1));
            p.setPen(Qt::white);
            p.drawPoint(mCursorPos);
            p.drawText(QRect(3, 3, 60, 30), Qt::AlignLeft | Qt::AlignVCenter, QString("u = %1\nv = %2").arg(u, 0, 'f', 3).arg(v, 0, 'f', 3));
        }
    }
    else {
        p.fillRect(rect(), QColor(44, 8, 7));
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QColor(150 + int(105 * sin(6e-3 * mStartupTime.elapsed())), 8, 7));
        p.drawText(rect(), Qt::AlignHCenter | Qt::AlignVCenter, tr("Initializing sensor ... Please wait ..."));
    }
}


void DepthWidget::mousePressEvent(QMouseEvent* e)
{
    mCursorPos = e->pos();
}


void DepthWidget::mouseMoveEvent(QMouseEvent* e)
{
    mCursorPos = e->pos();
}


void DepthWidget::timerEvent(QTimerEvent* e)
{
    if (e->timerId() == mStartupTimerId) {
        if (mDepthFrame.isNull()) {
            update();
            e->accept();
        }
        else {
            killTimer(mStartupTimerId);
            e->ignore();
        }
    }
}


void DepthWidget::setFPS(qreal fps)
{
    mFPS = fps;
}


void DepthWidget::setFOV(qreal hfov, qreal vfov)
{
    mHFOV = hfov;
    mVFOV = vfov;
    mU = cos(mHFOV);
    mV = cos(mVFOV);
    qDebug() << "cos(" << (mHFOV/M_PI*180) << "deg) =" << mU << ", cos(" << (mVFOV/M_PI*180) << "deg) =" << mV;
}


void DepthWidget::setDepthFrame(const QImage& frame)
{
    mDepthFrame = frame;
    mImageAspectRatio = (qreal)mDepthFrame.width() / mDepthFrame.height();
    update();
}


void DepthWidget::setDepthFrame(const XnDepthPixel* const pixels, int width, int height)
{
    mDepth.resize(width * height);
    const size_t sz =  width * height * sizeof(XnDepthPixel);
    memcpy_s(mDepth.data(), sz, pixels, sz);
}
