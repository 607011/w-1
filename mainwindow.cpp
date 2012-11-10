// Copyright (c) 2012 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.

#include "mainwindow.h"
#include "ui_mainwindow.h"


MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    mSensorThread = new SensorThread(this);

    mThreeDWidget = new ThreeDWidget;
    ui->gridLayout->addWidget(mThreeDWidget, 0, 0);
    mSensorWidget = new SensorWidget;
    ui->gridLayout->addWidget(mSensorWidget, 0, 1);

    QObject::connect(mSensorThread, SIGNAL(depthFrameReady(const QImage&)), mSensorWidget, SLOT(depthFrameReady(const QImage&)), Qt::BlockingQueuedConnection);
    QObject::connect(mSensorThread, SIGNAL(videoFrameReady(const QImage&)), mThreeDWidget, SLOT(videoFrameReady(const QImage&)), Qt::BlockingQueuedConnection);

    mSensorThread->resume();
}


MainWindow::~MainWindow()
{
    delete mSensorThread;
    delete mSensorWidget;
    delete mThreeDWidget;
    delete ui;
}


void MainWindow::closeEvent(QCloseEvent*)
{
    mSensorThread->stop();
    mSensorThread->wait(3000);
}
