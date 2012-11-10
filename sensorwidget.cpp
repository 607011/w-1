// Copyright (c) 2012 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.

#include <QPainter>
#include "sensorwidget.h"
#include "ui_sensorwidget.h"

SensorWidget::SensorWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SensorWidget)
{
    ui->setupUi(this);
}

SensorWidget::~SensorWidget()
{
    delete ui;
}


void SensorWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.drawImage(rect(), mDepthFrame);
    p.setOpacity(0.3);
    p.drawImage(rect(), mVideoFrame);
}


void SensorWidget::depthFrameReady(const QImage& frame)
{
    mDepthFrame = frame;
    update();
}


void SensorWidget::videoFrameReady(const QImage& frame)
{
    mVideoFrame = frame;
    update();
}
