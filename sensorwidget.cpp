// Copyright (c) 2012 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.

#include <QPainter>
#include "sensorwidget.h"
#include "ui_sensorwidget.h"

SensorWidget::SensorWidget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::SensorWidget)
    , mWindowAspectRatio(640/480)
    , mImageAspectRatio(640/480)
{
    ui->setupUi(this);
}


SensorWidget::~SensorWidget()
{
    delete ui;
}


void SensorWidget::resizeEvent(QResizeEvent* e)
{
    mWindowAspectRatio = (qreal) e->size().width() / e->size().height();
}


void SensorWidget::paintEvent(QPaintEvent*)
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
    if (!mDepthFrame.isNull())
        p.drawImage(mDestRect, mDepthFrame);
    if (!mVideoFrame.isNull()) {
        p.setOpacity(0.3);
        p.drawImage(mDestRect, mVideoFrame);
    }
    p.setOpacity(0.8);
    p.setPen(Qt::red);
    p.setBrush(Qt::transparent);
    p.drawLine(0, height()/2, width(), height()/2);
    p.drawLine(width()/2, 0, width()/2, height());
}


void SensorWidget::depthFrameReady(const QImage& frame)
{
    mDepthFrame = frame;
    mImageAspectRatio = (qreal)mDepthFrame.width() / mDepthFrame.height();
    update();
}


void SensorWidget::videoFrameReady(const QImage& frame)
{
    mVideoFrame = frame;
    update();
}
