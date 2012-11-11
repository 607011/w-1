// Copyright (c) 2012 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.

#include <QtCore/QDebug>
#include "sensormotor.h"

SensorMotor::SensorMotor(QObject* parent)
    : QObject(parent)
    , mOpened(false)
{
    /* ... */
}


SensorMotor::~SensorMotor()
{
    close();
}


bool SensorMotor::open(void)
{
    const XnUSBConnectionString* paths;
    XnUInt32 count;
    XnStatus rc;
    rc = xnUSBInit();
    if (rc != XN_STATUS_OK) {
        qDebug() << "xnUSBInit failed";
        return false;
    }
    rc = xnUSBEnumerateDevices(0x045E, 0x02B0, &paths, &count);
    if (rc != XN_STATUS_OK) {
        qDebug() << "xnUSBEnumerateDevices failed";
        return false;
    }

    rc = xnUSBOpenDeviceByPath(paths[0], &mDevice);
    if(rc != XN_STATUS_OK) {
        qDebug() << "xnUSBOpenDeviceByPath failed";
        return false;
    }
    XnUChar buf[1]; // output buffer
    rc = xnUSBSendControl(mDevice, (XnUSBControlType) 0xc0, 0x10, 0x00, 0x00, buf, sizeof(buf), 0);
    if (rc != XN_STATUS_OK) {
        qDebug() << "xnUSBSendControl failed";
        close();
        return false;
    }
    rc = xnUSBSendControl(mDevice, XnUSBControlType::XN_USB_CONTROL_TYPE_VENDOR, 0x06, 0x01, 0x00, NULL, 0, 0);
    if (rc != XN_STATUS_OK) {
        qDebug() << "xnUSBSendControl failed";
        close();
        return false;
    }
    mOpened = true;
    return mOpened;
}


void SensorMotor::close(void)
{
    if (isOpen()) {
        xnUSBCloseDevice(mDevice);
        mOpened = false;
    }
}


bool SensorMotor::setTilt(int angle)
{
    if (!isOpen())
        return false;
    XnStatus res = xnUSBSendControl(mDevice, XN_USB_CONTROL_TYPE_VENDOR, 0x31, angle, 0x00, NULL, 0, 0);
    if (res != XN_STATUS_OK) {
        qDebug() << "xnUSBSendControl failed";
        return false;
    }
    return true;
}
