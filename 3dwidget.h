// Copyright (c) 2012 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.

#ifndef __3DWIDGET_H_
#define __3DWIDGET_H_

#define USE_SHADER

#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QPoint>
#include <QVector2D>
#include <QVector3D>
#include <QGLWidget>
#include <gl/GLU.h>
#ifdef USE_SHADER
#include <QGLShaderProgram>
#endif


class ThreeDWidget : public QGLWidget
{
    Q_OBJECT

public: // methods
    explicit ThreeDWidget(QWidget* parent = NULL);
    QSize minimumSizeHint(void) const { return QSize(320, 240); }
    QSize sizeHint(void) const { return QSize(640, 480); }

    void setXRotation(float);
    void setYRotation(float);
    void setZRotation(float);
    void setXTranslation(float);
    void setYTranslation(float);
    void setZoom(float);

    float xRotation(void) const { return mXRot; }
    float yRotation(void) const { return mYRot; }
    float zRotation(void) const { return mZRot; }
    float zoom(void) const { return mZoom; }

    static const GLfloat DefaultZoom;
    static const GLfloat DefaultXRot;
    static const GLfloat DefaultYRot;
    static const GLfloat DefaultZRot;

public slots:
    void videoFrameReady(const QImage&);
    void setGamma(double);
    void setSharpening(int percent);

private: // variables
    static const QVector3D mVertices[4];
    static const QVector2D mTexCoords[4];
    static const QVector2D mOffset[9];
    static GLfloat mSharpeningKernel[9];

    GLfloat mXRot;
    GLfloat mYRot;
    GLfloat mZRot;
    GLfloat mXTrans;
    GLfloat mYTrans;
    GLfloat mZTrans;
    GLfloat mZoom;
    QPoint mLastPos;
    GLuint mTextureHandle;
#ifdef USE_SHADER
    QGLShaderProgram* mShaderProgram;
#endif
    GLfloat mGamma;
    QVector2D mFrameSize;

protected: // methods
    void initializeGL(void);
    void resizeGL(int, int);
    void paintGL(void);
    void mousePressEvent(QMouseEvent*);
    void mouseReleaseEvent(QMouseEvent*);
    void mouseMoveEvent(QMouseEvent*);
    void wheelEvent(QWheelEvent*);
    void keyPressEvent(QKeyEvent*);

private: // methods

};

#endif // __3DWIDGET_H_
