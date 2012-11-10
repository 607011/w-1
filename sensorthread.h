// Copyright (c) 2012 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.

#ifndef __SENSORTHREAD_H_
#define __SENSORTHREAD_H_

#include <QThread>
#include <QImage>
#include <XnCppWrapper.h>

class SensorThread : public QThread
{
    Q_OBJECT

public:
    explicit SensorThread(QObject* parent = NULL);
    ~SensorThread();
    void stop(void);
    void resume(void);

protected:
    void run(void);
    
signals:
    void depthFrameReady(const QImage&);
    void videoFrameReady(const QImage&);

private:
    bool mPrestart;
    bool mStopped;
    int mNearClipping;
    int mFarClipping;
    xn::ScriptNode mScriptNode;
    xn::Context mContext;
    xn::DepthGenerator mDepthGenerator;
    xn::ImageGenerator mImageGenerator;
    xn::DepthMetaData mDepthMetaData;
    xn::ImageMetaData mImageMetaData;

private: // methods
};

#endif // SENSORTHREAD_H
