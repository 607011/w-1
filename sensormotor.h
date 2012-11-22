// Copyright (c) 2012 Oliver Lau <ola@ct.de>
// All rights reserved.

#ifndef __SENSORMOTOR_H_
#define __SENSORMOTOR_H_

#include <XnUSB.h>
#include <QObject>

class SensorMotor : public QObject
{
    Q_OBJECT

public:
    explicit SensorMotor(QObject* parent = NULL);
    ~SensorMotor();
    bool open(void);
    void close(void);
    bool isOpen(void) const { return mOpened; }

public slots:
    bool setTilt(int angle);

private:
    XN_USB_DEV_HANDLE mDevice;
    bool mOpened;
};

#endif // __SENSORMOTOR_H_
