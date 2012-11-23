// Copyright (c) 2012 Oliver Lau <ola@ct.de>
// All rights reserved.

#ifndef __SENSORWIDGET_H_
#define __SENSORWIDGET_H_

#include <QWidget>
#include <QImage>
#include <QPoint>
#include <QRectF>
#include <QResizeEvent>

class DepthWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DepthWidget(QWidget* parent = NULL);
    QSize minimumSizeHint(void) const { return QSize(320, 240); }
    QSize sizeHint(void) { return QSize(640, 480); }

    inline void setFPS(qreal fps) { mFPS = fps; }

public slots:
    void setDepthFrame(const QImage&);

protected:
    void paintEvent(QPaintEvent*);
    void resizeEvent(QResizeEvent*);
    void mouseMoveEvent(QMouseEvent*);

private:
    QImage mDepthFrame;
    qreal mWindowAspectRatio;
    qreal mImageAspectRatio;
    QRectF mDestRect;
    qreal mFPS;
    int mDepthUnderCursor;
    QPoint mCursorPos;
};

#endif // __SENSORWIDGET_H_
