// Copyright (c) 2012 Oliver Lau <ola@ct.de>
// All rights reserved.

#ifndef __SENSORWIDGET_H_
#define __SENSORWIDGET_H_

#include <QWidget>
#include <QImage>
#include <QPoint>
#include <QRectF>
#include <QTimer>
#include <QTime>
#include <QVector>
#include <QResizeEvent>
#include <XnCppWrapper.h>


class DepthWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DepthWidget(QWidget* parent = NULL);
    QSize minimumSizeHint(void) const { return QSize(320, 240); }
    QSize sizeHint(void) { return QSize(640, 480); }

    void setDepthFrame(const XnDepthPixel* const pixels, int width, int height);
    void setFPS(qreal);
    void setFOV(qreal hfov, qreal vfov);

public slots:
    void setDepthFrame(const QImage&);

protected:
    void paintEvent(QPaintEvent*);
    void resizeEvent(QResizeEvent*);
    void mouseMoveEvent(QMouseEvent*);
    void mousePressEvent(QMouseEvent*);
    void timerEvent(QTimerEvent*);

private:
    QImage mDepthFrame;
    qreal mWindowAspectRatio;
    qreal mImageAspectRatio;
    QRectF mDestRect;
    qreal mFPS;
    qreal mHFOV;
    qreal mVFOV;
    qreal mU;
    qreal mV;
    int mDepthUnderCursor;
    QPoint mCursorPos;
    QVector<XnDepthPixel> mDepth;
    QTime mStartupTime;
    int mStartupTimerId;
};

#endif // __SENSORWIDGET_H_
