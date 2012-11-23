// Copyright (c) 2012 Oliver Lau <ola@ct.de>
// All rights reserved.

#include <QSettings>
#include <QtCore/QDebug>
#include <QtConcurrentRun>
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
    setCursor(Qt::WaitCursor);

    mDepthWidget = new DepthWidget;
    ui->gridLayout->addWidget(mDepthWidget, 0, 0);
    m3DWidget = new ThreeDWidget;
    ui->gridLayout->addWidget(m3DWidget, 0, 1);

    ui->frameLagSpinBox->setMaximum(ThreeDWidget::MaxVideoFrameLag);
    ui->mergedDepthFramesSpinBox->setMaximum(ThreeDWidget::MaxMergedDepthFrames);

    QObject::connect(ui->tiltSpinBox, SIGNAL(valueChanged(int)), &mSensorMotor, SLOT(setTilt(int)));

    QObject::connect(ui->gammaSpinBox, SIGNAL(valueChanged(double)), m3DWidget, SLOT(setGamma(double)));
    QObject::connect(ui->filterComboBox, SIGNAL(currentIndexChanged(int)), m3DWidget, SLOT(setFilter(int)));
    QObject::connect(ui->sharpeningSlider, SIGNAL(valueChanged(int)), m3DWidget, SLOT(setSharpening(int)));
    QObject::connect(ui->haloRadiusSpinBox, SIGNAL(valueChanged(int)), m3DWidget, SLOT(setHaloRadius(int)));
    QObject::connect(ui->frameLagSpinBox, SIGNAL(valueChanged(int)), m3DWidget, SLOT(setVideoFrameLag(int)));
    QObject::connect(ui->mergedDepthFramesSpinBox, SIGNAL(valueChanged(int)), m3DWidget, SLOT(setMergedDepthFrames(int)));

    QObject::connect(ui->saturationSlider, SIGNAL(valueChanged(int)), SLOT(saturationChanged(int)));
    QObject::connect(ui->contrastSlider, SIGNAL(valueChanged(int)), SLOT(contrastChanged(int)));

    QObject::connect(m3DWidget, SIGNAL(depthFrameReady(const QImage&)), mDepthWidget, SLOT(setDepthFrame(const QImage&)));

    QObject::connect(this, SIGNAL(sensorInitialized()), SLOT(postInitSensor()));

    mSensorMotor.open();
    mInitFuture = QtConcurrent::run(this, &MainWindow::initSensor);

    restoreSettings();
}


MainWindow::~MainWindow()
{
    delete mDepthWidget;
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
    ui->saturationSlider->setValue(settings.value("Options/saturation", 100).toInt());
    ui->contrastSlider->setValue(settings.value("Options/contrast", 100).toInt());
    ui->sharpeningSlider->setValue(settings.value("Options/sharpening", 0).toInt());
    ui->haloRadiusSpinBox->setValue(settings.value("Options/haloRadius", 5).toInt());
    ui->frameLagSpinBox->setValue(settings.value("Options/videoFrameLag", 3).toInt());
    ui->mergedDepthFramesSpinBox->setValue(settings.value("Options/mergedDepthFrames", 3).toInt());
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
    settings.setValue("Options/saturation", ui->saturationSlider->value());
    settings.setValue("Options/contrast", ui->contrastSlider->value());
    settings.setValue("Options/sharpening", ui->sharpeningSlider->value());
    settings.setValue("Options/haloRadius", ui->haloRadiusSpinBox->value());
    settings.setValue("Options/videoFrameLag", ui->frameLagSpinBox->value());
    settings.setValue("Options/mergedDepthFrames", ui->mergedDepthFramesSpinBox->value());
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

    emit sensorInitialized();
}


void MainWindow::postInitSensor(void)
{
    XnFieldOfView fov;
    mDepthGenerator.GetFieldOfView(fov);
    mDepthWidget->setFOV(fov.fHFOV, fov.fVFOV);
    mHoriFOV = fov.fHFOV / M_PI * 180;
    mVertFOV = fov.fVFOV / M_PI * 180;
    m3DWidget->setFOV(mHoriFOV, mVertFOV);
    startSensor();
    setCursor(Qt::ArrowCursor);
    statusBar()->showMessage(tr("Sensor initialized."), 3000);
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
        mDepthGenerator.GetAlternativeViewPointCap().SetViewPoint(mVideoGenerator);
        XnStatus rc = mContext.WaitAndUpdateAll();
        if (rc != XN_STATUS_OK) {
            qWarning() << "Failed updating data:" << xnGetStatusString(rc);
            return;
        }
        m3DWidget->setVideoFrame(mVideoGenerator.GetImageMap(), mVideoMetaData.XRes(), mVideoMetaData.YRes());
        m3DWidget->setDepthFrame(mDepthGenerator.GetDepthMap(), mDepthMetaData.XRes(), mDepthMetaData.YRes());
        mDepthWidget->setDepthFrame(mDepthGenerator.GetDepthMap(), mDepthMetaData.XRes(), mDepthMetaData.YRes());
        m3DWidget->setThresholds(ui->nearClippingSpinBox->value(), ui->farClippingSpinBox->value());
        if (++mFrameCount > 10) {
            mDepthWidget->setFPS(1e3 * mFrameCount / mT0.elapsed());
            mT0.start();
            mFrameCount = 0;
        }
    }
    else if (e->timerId() == mRegressTimerId) {
        regressH();
    }
}


void MainWindow::keyPressEvent(QKeyEvent* e)
{
    switch (e->key()) {
    case Qt::Key_Escape:
        m3DWidget->resetTransformations();
        break;
    default:
        e->ignore();
        return;
    }
    e->accept();
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
        mFrameTimerId = startTimer(2 * 1000 / 3 / mVideoMetaData.FPS());
        mRegressTimerId = startTimer(2000);
    }
}


void MainWindow::saturationChanged(int saturation)
{
    ui->saturationLabel->setText(QString("%1%").arg(saturation));
    m3DWidget->setSaturation(1e-2f * GLfloat(saturation));
}


void MainWindow::contrastChanged(int contrast)
{
    ui->contrastLabel->setText(QString("%1%").arg(contrast));
    m3DWidget->setContrast(1e-2f * GLfloat(contrast));
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
//    ui->z_LineEdit->setText(QString("%1 mm").arg(z_));
    float x_ = sumX / n - 0.5f * mDepthMetaData.XRes();
//    ui->x_LineEdit->setText(QString("%1").arg(x_));
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
//    ui->aLineEdit->setText(QString("%1").arg(hA));
//    ui->bLineEdit->setText(QString("%1").arg(hB));
//    ui->zCenterLineEdit->setText(QString("%1 mm").arg((int)midLine[mDepthMetaData.XRes() / 2]));
}
