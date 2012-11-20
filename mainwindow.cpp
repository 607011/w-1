// Copyright (c) 2012 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.

#include <QSettings>
#include <QtCore/QDebug>
#include <qmath.h>
#include "main.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"


MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , mFrameTimerId(0)
    , mRegressTimerId(0)
{
    ui->setupUi(this);
    setWindowTitle(tr("%1 %2").arg(AppName).arg(AppVersion));

    mSensorWidget = new SensorWidget;
    ui->gridLayout->addWidget(mSensorWidget, 0, 0);
    m3DWidget = new ThreeDWidget;
    ui->gridLayout->addWidget(m3DWidget, 0, 1);

    mSensorMotor.open();

    initSensor();
    startSensor();

    QObject::connect(ui->tiltSpinBox, SIGNAL(valueChanged(int)), &mSensorMotor, SLOT(setTilt(int)));
    QObject::connect(ui->gammaSpinBox, SIGNAL(valueChanged(double)), m3DWidget, SLOT(setGamma(double)));
    QObject::connect(ui->sharpeningSlider, SIGNAL(valueChanged(int)), m3DWidget, SLOT(setSharpening(int)));

    QObject::connect(m3DWidget, SIGNAL(depthFrameReady(const QImage&)), mSensorWidget, SLOT(setDepthFrame(const QImage&)));

    restoreSettings();
}


MainWindow::~MainWindow()
{
    delete mSensorWidget;
    delete m3DWidget;
    delete ui;
}


void MainWindow::restoreSettings(void)
{
    QSettings settings(Company, AppName);
    restoreGeometry(settings.value("MainWindow/geometry").toByteArray());
    ui->nearClippingSpinBox->setValue(settings.value("Options/nearClipping", 0).toInt());
    ui->farClippingSpinBox->setValue(settings.value("Options/farClipping", 4096).toInt());
    ui->gammaSpinBox->setValue(settings.value("Options/gamma", 2.2).toDouble());
    ui->sharpeningSlider->setValue(settings.value("Options/sharpening", 0).toInt());
    ui->tiltSpinBox->setValue(settings.value("Sensor/tilt", 0).toInt());
}


void MainWindow::saveSettings(void)
{
    QSettings settings(Company, AppName);
    settings.setValue("MainWindow/geometry", saveGeometry());
    settings.setValue("Options/zoom", m3DWidget->zoom());
    settings.setValue("Options/nearClipping", ui->nearClippingSpinBox->value());
    settings.setValue("Options/farClipping", ui->farClippingSpinBox->value());
    settings.setValue("Options/gamma", ui->gammaSpinBox->value());
    settings.setValue("Options/sharpening", ui->sharpeningSlider->value());
    settings.setValue("Sensor/tilt", ui->tiltSpinBox->value());
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

    rc = mContext.FindExistingNode(XN_NODE_TYPE_IMAGE, mVideoGenerator);
    if (rc != XN_STATUS_OK) {
        qWarning() << "FindExistingNode(XN_NODE_TYPE_IMAGE) failed";
        return;
    }

    rc = mDepthGenerator.Create(mContext);
    if (rc != XN_STATUS_OK) {
        qWarning() << "mDepthGenerator.Create() failed";
        return;
    }

    rc = mVideoGenerator.Create(mContext);
    if (rc != XN_STATUS_OK) {
        qWarning() << "mImageGenerator.Create() failed";
        return;
    }

    mContext.SetGlobalMirror(0);
    mDepthGenerator.GetMetaData(mDepthMetaData);
    mVideoGenerator.GetMetaData(mVideoMetaData);

    XnFieldOfView fov;
    mDepthGenerator.GetFieldOfView(fov);
    m3DWidget->setFOV(fov.fHFOV/M_PI*180, fov.fVFOV/M_PI*180);
}


void MainWindow::closeEvent(QCloseEvent* e)
{
    saveSettings();
    stopSensor();
    mVideoGenerator.Release();
    mDepthGenerator.Release();
    mContext.Release();
    e->accept();
}


void MainWindow::timerEvent(QTimerEvent* e)
{
    if (e->timerId() == mFrameTimerId) {
        XnStatus rc;
        mDepthGenerator.GetAlternativeViewPointCap().SetViewPoint(mVideoGenerator);
        rc = mContext.WaitAndUpdateAll();
        if (rc != XN_STATUS_OK) {
            qWarning() << "Failed updating data:" << xnGetStatusString(rc);
            return;
        }
        m3DWidget->setThresholds(ui->nearClippingSpinBox->value(), ui->farClippingSpinBox->value());
        m3DWidget->setVideoFrame(mVideoGenerator.GetImageMap(), mVideoMetaData.XRes(), mVideoMetaData.YRes());
        m3DWidget->setDepthFrame(mDepthGenerator.GetDepthMap(), mDepthMetaData.XRes(), mDepthMetaData.YRes());
        if (++mFrameCount > 10) {
            ui->fpsLineEdit->setText(QString("%1").arg(1e3 * mFrameCount / mT0.elapsed(), 0, 'f', 3));
            mT0.start();
            mFrameCount = 0;
        }
    }
    else if (e->timerId() == mRegressTimerId) {
        regressH();
    }
}


void MainWindow::stopSensor(void)
{
    if (mFrameTimerId) {
        killTimer(mFrameTimerId);
        mFrameTimerId = 0;
    }
    if (mRegressTimerId) {
        killTimer(mRegressTimerId);
        mRegressTimerId = 0;
    }
    if (mVideoGenerator.IsGenerating())
        mVideoGenerator.StopGenerating();
    if (mDepthGenerator.IsGenerating())
        mDepthGenerator.StopGenerating();
}


void MainWindow::startSensor(void)
{
    if (mFrameTimerId == 0) {
        mFrameCount = 0;
        mT0.start();
        mDepthGenerator.StartGenerating();
        mVideoGenerator.StartGenerating();
        mFrameTimerId = startTimer(1000 / mVideoMetaData.FPS() / 2);
        mRegressTimerId = startTimer(2000);
    }
}


void MainWindow::regressH(void)
{
    const int nearThreshold = ui->nearClippingSpinBox->value();
    const int farThreshold = ui->farClippingSpinBox->value();
    const XnDepthPixel* const midLine = mDepthMetaData.Data() + mDepthMetaData.XRes() * mDepthMetaData.YRes() / 2;
    const XnDepthPixel* pZ;
    float sumZ = 0, sumX = 0;
    int n = 0;
    pZ = midLine;
    for (XnUInt32 x = 0; x < mDepthMetaData.XRes(); ++x) {
        int z = int(*pZ++);
        if (z > nearThreshold && z < farThreshold) {
            sumZ += z;
            sumX += x;
            ++n;
        }
    }
    float z_ = sumZ / n;
    ui->z_LineEdit->setText(QString("%1 mm").arg(z_));
    float x_ = sumX / n - 0.5f * mDepthMetaData.XRes();
    ui->x_LineEdit->setText(QString("%1").arg(x_));
    pZ = midLine;
    float sumZ2 = 0, sumZX = 0;
    for (XnUInt32 x = 0; x < mDepthMetaData.XRes(); ++x) {
        int z = int(*pZ++);
        if (z > nearThreshold && z < farThreshold) {
            const float zd = z - z_;
            const float xd = x - x_;
            sumZX += zd * xd;
            sumZ2 += zd * zd;
        }
    }
    hB = sumZX / sumZ2;
    hA = x_ - hB * z_;
    ui->aLineEdit->setText(QString("%1").arg(hA));
    ui->bLineEdit->setText(QString("%1").arg(hB));
    ui->zCenterLineEdit->setText(QString("%1 mm").arg((int)midLine[mDepthMetaData.XRes() / 2]));
}
