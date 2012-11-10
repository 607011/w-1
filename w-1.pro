# Copyright (c) 2012 Oliver Lau <oliver@von-und-fuer-lau.de>
# All rights reserved.

QT += core gui opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = W-1
TEMPLATE = app
INCLUDEPATH += "C:/Program Files (x86)/OpenNI/Include"
LIBS += "C:/Program Files (x86)/OpenNI/Lib/openNI.lib"

SOURCES += main.cpp \
    mainwindow.cpp \
    3dwidget.cpp \
    sensorthread.cpp \
    sensorwidget.cpp

HEADERS += mainwindow.h \
    3dwidget.h \
    sensorthread.h \
    sensorwidget.h

FORMS += mainwindow.ui \
    sensorwidget.ui

RESOURCES += \
    config.qrc \
    shaders.qrc

OTHER_FILES += \
    config/sample.xml \
    shaders/fragmentshader.fsh \
    shaders/vertexshader.vsh \
    README.md
