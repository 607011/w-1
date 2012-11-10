// Copyright (c) 2012 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.

#include <limits>
#include <QtCore/QDebug>
#include <XnOS.h>
#include "sensorthread.h"

SensorThread::SensorThread(QObject *parent)
    : QThread(parent)
    , mPrestart(true)
    , mStopped(false)
    , mNearClipping(0)
    , mFarClipping(0xffff - 0x8000)
{
    XnStatus rc = XN_STATUS_OK;
    xn::EnumerationErrors errors;
    rc = mContext.InitFromXmlFile("../w-1/config/sample.xml", mScriptNode, &errors);

    if (rc == XN_STATUS_NO_NODE_PRESENT) {
        qWarning() << "XN_STATUS_NO_NODE_PRESENT";
        XnChar strError[1024];
        errors.ToString(strError, 1024);
        qWarning() << strError;
        return;
    }
    else if (rc != XN_STATUS_OK) {
        qWarning() << "mContext.InitFromXmlFile() failed:" << xnGetStatusString(rc);
        return;
    }

    rc = mContext.FindExistingNode(XN_NODE_TYPE_DEPTH, mDepthGenerator);
    if (rc != XN_STATUS_OK) {
        qWarning() << "FindExistingNode(XN_NODE_TYPE_DEPTH) failed";
        return;
    }

    rc = mContext.FindExistingNode(XN_NODE_TYPE_IMAGE, mImageGenerator);
    if (rc != XN_STATUS_OK) {
        qWarning() << "FindExistingNode(XN_NODE_TYPE_IMAGE) failed";
        return;
    }

    rc = mDepthGenerator.Create(mContext);
    if (rc != XN_STATUS_OK) {
        qWarning() << "mDepthGenerator.Create() failed";
        return;
    }

    rc = mImageGenerator.Create(mContext);
    if (rc != XN_STATUS_OK) {
        qWarning() << "mImageGenerator.Create() failed";
        return;
    }

    mDepthGenerator.GetAlternativeViewPointCap().SetViewPoint(mImageGenerator);
    mDepthGenerator.GetMetaData(mDepthMetaData);
    mImageGenerator.GetMetaData(mImageMetaData);

    qDebug() << "depth image resolution: " << mDepthMetaData.XRes() << "x" << mDepthMetaData.YRes() << "@" << mDepthMetaData.FPS() << "fps";
    qDebug() << "video image resolution: " << mImageMetaData.XRes() << "x" << mImageMetaData.YRes() << "@" << mImageMetaData.FPS() << "fps";

    // make sure that Qt's threading system is up and running to prevent lagged thread execution after pressing "run" the first time
    start();
}


SensorThread::~SensorThread()
{
    mImageGenerator.Release();
    mDepthGenerator.Release();
    mContext.Release();
}


void SensorThread::stop(void)
{
    mStopped = true;
}


void SensorThread::resume(void)
{
    mStopped = false;
    start();
}


void SensorThread::regress(const XnDepthPixel* depthPixels, int size, qreal& A, qreal& B, qreal& C)
{
    Q_UNUSED(depthPixels);
    Q_UNUSED(size);
    Q_UNUSED(A);
    Q_UNUSED(B);
    Q_UNUSED(C);
}


void SensorThread::run(void)
{
    if (mPrestart) {
        mPrestart = false;
        return;
    }
    mContext.StartGeneratingAll();
    while (!mStopped) {
        XnStatus rc = mContext.WaitAndUpdateAll();
        if (rc != XN_STATUS_OK) {
            qDebug() << "Failed updating data:" << xnGetStatusString(rc);
            continue;
        }
        const XnDepthPixel* depthPixels = mDepthMetaData.Data();
        const XnDepthPixel* const depthPixelsEnd = depthPixels + (mDepthMetaData.XRes() * mDepthMetaData.YRes());
        QImage depthImage(mDepthMetaData.XRes(), mDepthMetaData.YRes(), QImage::Format_ARGB32);
        QRgb* dst = reinterpret_cast<QRgb*>(depthImage.bits());
        while (depthPixels < depthPixelsEnd) {
            const int depth = *depthPixels++;
            QRgb c;
            if (depth == 0) {
                c = qRgb(0, 251, 190);
            }
            else if (depth < mNearClipping) {
                c = qRgb(251, 85, 5);
            }
            else if (depth > mFarClipping) {
                c = qRgb(8, 98, 250);
            }
            else {
                const quint8 k = quint8((depth - mNearClipping) * 255 / (mFarClipping - mNearClipping));
                c = qRgb(k, k, k);
            }
            *dst++ = c;
        }
        QImage videoImage(mImageMetaData.Data(), mImageMetaData.XRes(), mDepthMetaData.YRes(), QImage::Format_RGB888);
        emit videoFrameReady(videoImage);
        emit depthFrameReady(depthImage);
    }
}
