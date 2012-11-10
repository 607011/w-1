// Copyright (c) 2012 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.

#ifndef __SENSORWIDGET_H_
#define __SENSORWIDGET_H_

#include <QWidget>
#include <QImage>
#include <QRectF>
#include <QResizeEvent>

namespace Ui {
class SensorWidget;
}

class SensorWidget : public QWidget
{
    Q_OBJECT
    
public:
    explicit SensorWidget(QWidget* parent = NULL);
    ~SensorWidget();
    QSize minimumSizeHint(void) const { return QSize(320, 240); }
    QSize sizeHint(void) { return QSize(640, 480); }

public slots:
    void videoFrameReady(const QImage&);
    void depthFrameReady(const QImage&);

protected:
    void paintEvent(QPaintEvent*);
    void resizeEvent(QResizeEvent*);

private:
    Ui::SensorWidget *ui;
    QImage mDepthFrame;
    QImage mVideoFrame;
    qreal mWindowAspectRatio;
    qreal mImageAspectRatio;
    QRectF mDestRect;
};

#endif // __SENSORWIDGET_H_
