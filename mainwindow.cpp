// Copyright (c) 2012 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.

#include <QtCore/QDebug>
#include "mainwindow.h"
#include "ui_mainwindow.h"


MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    mThreeDWidget = new ThreeDWidget;
    ui->gridLayout->addWidget(mThreeDWidget, 0, 0);
    mSensorWidget = new SensorWidget;
    ui->gridLayout->addWidget(mSensorWidget, 0, 1);

    initSensor();
}


MainWindow::~MainWindow()
{
    delete mSensorWidget;
    delete mThreeDWidget;
    delete ui;
}


void MainWindow::initSensor(void)
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

    // mDepthGenerator.GetAlternativeViewPointCap().SetViewPoint(mImageGenerator);
    mDepthGenerator.GetMetaData(mDepthMetaData);
    mImageGenerator.GetMetaData(mImageMetaData);

    qDebug() << "depth image resolution: " << mDepthMetaData.XRes() << "x" << mDepthMetaData.YRes() << "@" << mDepthMetaData.FPS() << "fps";
    qDebug() << "video image resolution: " << mImageMetaData.XRes() << "x" << mImageMetaData.YRes() << "@" << mImageMetaData.FPS() << "fps";

    startSensor();
}


void MainWindow::closeEvent(QCloseEvent*)
{
    stopSensor();
    mImageGenerator.Release();
    mDepthGenerator.Release();
    mContext.Release();
}


void MainWindow::timerEvent(QTimerEvent*)
{
    XnStatus rc;
    if (!mImageGenerator.IsNewDataAvailable() && !mDepthGenerator.IsNewDataAvailable())
        return;

    rc = mContext.WaitAndUpdateAll();
    if (rc != XN_STATUS_OK) {
        qDebug() << "Failed updating data:" << xnGetStatusString(rc);
        return;
    }

    const XnDepthPixel* depthPixels = mDepthMetaData.Data();
    const XnDepthPixel* const depthPixelsEnd = depthPixels + (mDepthMetaData.XRes() * mDepthMetaData.YRes());
    QImage depthImage(mDepthMetaData.XRes(), mDepthMetaData.YRes(), QImage::Format_ARGB32);
    QRgb* dst = reinterpret_cast<QRgb*>(depthImage.bits());
    const int clipDelta = ui->farClippingHorizontalSlider->value() - ui->nearClippingHorizontalSlider->value();
    while (depthPixels < depthPixelsEnd) {
        const int depth = int(*depthPixels++);
        QRgb c;
        if (depth == 0) {
            c = qRgb(0, 251, 190);
        }
        else if (depth < ui->nearClippingHorizontalSlider->value()) {
            c = qRgb(251, 85, 5);
        }
        else if (depth > ui->farClippingHorizontalSlider->value()) {
            c = qRgb(8, 98, 250);
        }
        else {
            const quint8 k = quint8(255 * (depth - ui->nearClippingHorizontalSlider->value()) / clipDelta);
            c = qRgb(k, k, k);
        }
        *dst++ = c;
    }
    mSensorWidget->depthFrameReady(depthImage);
    QImage videoImage(mImageMetaData.Data(), mImageMetaData.XRes(), mDepthMetaData.YRes(), QImage::Format_RGB888);
    mThreeDWidget->videoFrameReady(videoImage);
}


void MainWindow::stopSensor(void)
{
    killTimer(mTimerId);
    mImageGenerator.StopGenerating();
    mDepthGenerator.StopGenerating();
}


void MainWindow::startSensor(void)
{
    mContext.StartGeneratingAll();
    mTimerId = startTimer(1000 / mImageMetaData.FPS());
}
