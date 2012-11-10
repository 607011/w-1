// Copyright (c) 2012 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.

#ifndef __MAINWINDOW_H_
#define __MAINWINDOW_H_

#include <QMainWindow>
#include <QImage>
#include <QLabel>
#include "sensorthread.h"
#include "3dwidget.h"
#include "sensorwidget.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = NULL);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent*);

private:
    Ui::MainWindow *ui;
    SensorThread mSensorThread;
    SensorWidget* mSensorWidget;
    ThreeDWidget* mThreeDWidget;

private: // methods

private slots:

};

#endif // __MAINWINDOW_H_
