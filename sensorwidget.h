// Copyright (c) 2012 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.

#ifndef __SENSORWIDGET_H_
#define __SENSORWIDGET_H_

#include <QWidget>
#include <QImage>
#include <QRectF>
#include <QResizeEvent>


class SensorWidget : public QWidget
{
    Q_OBJECT
    
public:
    explicit SensorWidget(QWidget* parent = NULL);
    QSize minimumSizeHint(void) const { return QSize(320, 240); }
    QSize sizeHint(void) { return QSize(640, 480); }

public slots:
    void depthFrameReady(const QImage&);

protected:
    void paintEvent(QPaintEvent*);
    void resizeEvent(QResizeEvent*);

private:
    QImage mDepthFrame;
    qreal mWindowAspectRatio;
    qreal mImageAspectRatio;
    QRectF mDestRect;

};

#endif // __SENSORWIDGET_H_
