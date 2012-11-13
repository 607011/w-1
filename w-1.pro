# Copyright (c) 2012 Oliver Lau <oliver@von-und-fuer-lau.de>
# All rights reserved.

QT += core gui opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = W-1
TEMPLATE = app
INCLUDEPATH += "$(OPEN_NI_INCLUDE)" \
    "dep/glew-1.9.0/include"
LIBS += -L"C:/Program Files (x86)/OpenNI/Lib" \
    -L"C:/Program Files/OpenNI/Lib" \
    -lopenNI \
    -L"../w-1/dep/glew-1.9.0/lib" \
    -lglew32

SOURCES += main.cpp \
    mainwindow.cpp \
    3dwidget.cpp \
    sensorwidget.cpp \
    sensormotor.cpp

HEADERS += mainwindow.h \
    3dwidget.h \
    sensorwidget.h \
    main.h \
    sensormotor.h

FORMS += mainwindow.ui

RESOURCES += \
    config.qrc \
    shaders.qrc

OTHER_FILES += \
    config/sample.xml \
    shaders/fragmentshader.glsl \
    shaders/vertexshader.glsl \
    README.md
