// Copyright (c) 2012 Oliver Lau <ola@ct.de>
// All rights reserved.

#ifndef __MAINWINDOW_H_
#define __MAINWINDOW_H_

#include <QMainWindow>
#include <QCloseEvent>
#include <QShowEvent>
#include <QTimerEvent>
#include <QKeyEvent>
#include <QTime>
#include <QImage>
#include <QMessageBox>

#include <XnCppWrapper.h>

#include "3dwidget.h"
#include "sensorwidget.h"
#include "sensormotor.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = NULL);
    ~MainWindow();

signals:
    void sensorInitialized(void);

protected:
    void closeEvent(QCloseEvent*);
    void timerEvent(QTimerEvent*);
    void keyPressEvent(QKeyEvent*);

private:
    QFuture<void> mInitFuture;
    Ui::MainWindow *ui;
    SensorWidget* mSensorWidget;
    ThreeDWidget* m3DWidget;
    SensorMotor mSensorMotor;
    int mFrameTimerId;
    int mRegressTimerId;
    xn::ScriptNode mScriptNode;
    xn::Context mContext;
    xn::DepthGenerator mDepthGenerator;
    xn::ImageGenerator mVideoGenerator;
    xn::DepthMetaData mDepthMetaData;
    xn::ImageMetaData mVideoMetaData;
    qreal hA;
    qreal hB;
    QTime mT0;
    int mFrameCount;

private: // methods
    void initSensor(void);
    void startSensor(void);
    void stopSensor(void);
    void regressH(void);

    void saveSettings(void);
    void restoreSettings(void);

private slots:
    void contrastChanged(int);
    void saturationChanged(int);
    void postInitSensor(void);
};

#endif // __MAINWINDOW_H_
