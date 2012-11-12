// Copyright (c) 2012 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.

#include <QSettings>
#include <QtCore/QDebug>
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
    mThreeDWidget = new ThreeDWidget;
    ui->gridLayout->addWidget(mThreeDWidget, 0, 1);

    mSensorMotor.open();
//    qDebug() << "mSensorMotor.open() " << (mSensorMotor.isOpen()? "succeeded" : "failed");

    initSensor();
    startSensor();

    QObject::connect(ui->tiltSpinBox, SIGNAL(valueChanged(int)), &mSensorMotor, SLOT(setTilt(int)));

    restoreSettings();
}


MainWindow::~MainWindow()
{
    delete mSensorWidget;
    delete mThreeDWidget;
    delete ui;
}


void MainWindow::restoreSettings(void)
{
    QSettings settings(Company, AppName);
    restoreGeometry(settings.value("MainWindow/geometry").toByteArray());
    ui->nearClippingSpinBox->setValue(settings.value("Options/nearClipping", 0).toInt());
    ui->farClippingSpinBox->setValue(settings.value("Options/farClipping", 4096).toInt());
    ui->tiltSpinBox->setValue(settings.value("Sensor/tilt", 0).toInt());
//    mThreeDWidget->setXRotation(settings.value("Options/xRot", ThreeDWidget::DefaultXRot).toFloat());
//    mThreeDWidget->setYRotation(settings.value("Options/yRot", ThreeDWidget::DefaultYRot).toFloat());
//    mThreeDWidget->setZoom(settings.value("Options/zoom", ThreeDWidget::DefaultZoom).toFloat());
}


void MainWindow::saveSettings(void)
{
    QSettings settings(Company, AppName);
    settings.setValue("MainWindow/geometry", saveGeometry());
    settings.setValue("Options/xRot", mThreeDWidget->xRotation());
    settings.setValue("Options/yRot", mThreeDWidget->yRotation());
    settings.setValue("Options/zoom", mThreeDWidget->zoom());
    settings.setValue("Options/nearClipping", ui->nearClippingSpinBox->value());
    settings.setValue("Options/farClipping", ui->farClippingSpinBox->value());
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
}


void MainWindow::closeEvent(QCloseEvent* e)
{
    saveSettings();
    stopSensor();
    mImageGenerator.Release();
    mDepthGenerator.Release();
    mContext.Release();
    e->accept();
}


void MainWindow::timerEvent(QTimerEvent*)
{
    if (!mImageGenerator.IsNewDataAvailable() && !mDepthGenerator.IsNewDataAvailable())
        return;
    XnStatus rc = mContext.WaitAndUpdateAll();
    if (rc != XN_STATUS_OK) {
        qDebug() << "Failed updating data:" << xnGetStatusString(rc);
        return;
    }
    QImage depthImage(mDepthMetaData.XRes(), mDepthMetaData.YRes(), QImage::Format_ARGB32);
    QRgb* dst = reinterpret_cast<QRgb*>(depthImage.bits());
    const int nearThreshold = ui->nearClippingSpinBox->value();
    const int farThreshold = ui->farClippingSpinBox->value();
    const int clipDelta = farThreshold - nearThreshold;
    const XnDepthPixel* depthPixels = mDepthMetaData.Data();
    const XnDepthPixel* const depthPixelsEnd = depthPixels + (mDepthMetaData.XRes() * mDepthMetaData.YRes());
    while (depthPixels < depthPixelsEnd) {
        const int depth = int(*depthPixels++);
        QRgb c;
        if (depth == 0) {
            c = qRgb(0, 251, 190);
        }
        else if (depth < nearThreshold) {
            c = qRgb(251, 85, 5);
        }
        else if (depth > farThreshold) {
            c = qRgb(8, 98, 250);
        }
        else {
            const quint8 k = quint8(255 * (depth - nearThreshold) / clipDelta);
            c = qRgb(k, k, k);
        }
        *dst++ = c;
    }
    mSensorWidget->depthFrameReady(depthImage);
    regressH();
    QImage videoImage(mImageMetaData.Data(), mImageMetaData.XRes(), mDepthMetaData.YRes(), QImage::Format_RGB888);
    mThreeDWidget->videoFrameReady(videoImage);
}


void MainWindow::stopSensor(void)
{
    if (mFrameTimerId) {
        killTimer(mFrameTimerId);
        mFrameTimerId = 0;
        mImageGenerator.StopGenerating();
        mDepthGenerator.StopGenerating();
    }
}


void MainWindow::startSensor(void)
{
    if (mFrameTimerId == 0) {
        mContext.StartGeneratingAll();
        mFrameTimerId = startTimer(1000 / mImageMetaData.FPS());
    }
}


void MainWindow::regressH(void)
{
    const int nearThreshold = ui->nearClippingSpinBox->value();
    const int farThreshold = ui->farClippingSpinBox->value();
    const XnDepthPixel* const midLine = mDepthMetaData.Data() + mDepthMetaData.XRes() * mDepthMetaData.YRes() / 2;
    qreal sumZ = 0, sumX = 0;
    int n = 0;
    const XnDepthPixel* pZ;
    pZ = midLine;
    for (XnUInt32 x = 0; x < mDepthMetaData.XRes(); ++x) {
        int z = int(*pZ++);
        if (z > nearThreshold && z < farThreshold) {
            sumZ += z;
            sumX += x;
            ++n;
        }
    }
    qreal z_ = sumZ / n;
    ui->z_LineEdit->setText(QString("%1 mm").arg(z_));
    qreal x_ = sumX / n;
    ui->x_LineEdit->setText(QString("%1").arg(x_));
    pZ = midLine;
    qreal sumZ2 = 0, sumZX = 0;
    for (XnUInt32 x = 0; x < mDepthMetaData.XRes(); ++x) {
        int z = int(*pZ++);
        if (z > nearThreshold && z < farThreshold) {
            qreal zd = z - z_;
            qreal xd = x - x_;
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


