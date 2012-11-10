// Copyright (c) 2012 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.

#ifndef __3DWIDGET_H_
#define __3DWIDGET_H_

#include <QObject>
#include <QString>
#include <QSize>
#include <QPoint>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QCloseEvent>
#include <QThread>
#include <QVector>
#include <QVector2D>
#include <QVector3D>
#include <QGLShaderProgram>
#include <QGLWidget>
#include <gl/GLU.h>

//#ifndef GL_MULTISAMPLE
//#define GL_MULTISAMPLE  0x809D
//#endif

class ThreeDWidget : public QGLWidget
{
    Q_OBJECT

public: // methods
    explicit ThreeDWidget(QWidget* parent = NULL);
    ~ThreeDWidget(void);
    QSize minimumSizeHint(void) const { return QSize(320, 240); }
    QSize sizeHint(void) const { return QSize(640, 480); }
    void setXRotation(int);
    void setYRotation(int);

public slots:
    void videoFrameReady(const QImage&);

private: // variables
    int mXRot;
    int mYRot;
    float mXTrans;
    float mYTrans;
    float mZTrans;
    float mZoom;
    QPoint mLastPos;
    GLuint mTextureHandle;
    static const QVector3D mVertices[4];
    static const QVector2D mTexCoords[4];
#ifdef QT_OPENGL_ES_2
    QGLShaderProgram* mShaderProgram;
#endif
    static const float DefaultZoom;
    static const float DefaultXRot;
    static const float DefaultYRot;

private: // methods
    void makeWall(void);

protected: // methods
    void initializeGL(void);
    void resizeGL(int, int);
    void paintGL(void);
    void mousePressEvent(QMouseEvent*);
    void mouseReleaseEvent(QMouseEvent*);
    void mouseMoveEvent(QMouseEvent*);
    void wheelEvent(QWheelEvent*);
    void keyPressEvent(QKeyEvent*);

};

#endif // __3DWIDGET_H_
