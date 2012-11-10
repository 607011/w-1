// Copyright (c) 2012 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.

#include <QColor>
#include <QImage>
#include <QPixmap>
#include <QtCore/QDebug>
#include <QtCore/QTimer>
#ifdef QT_OPENGL_ES_2
#include <QGLShader>
#endif
#include <qmath.h>

#include "3dwidget.h"

const float ThreeDWidget::DefaultZoom = -3.4f;
const float ThreeDWidget::DefaultXRot = 180.0f;
const float ThreeDWidget::DefaultYRot = 0.0f;
const QVector2D ThreeDWidget::mTexCoords[4] = {
    QVector2D(0, 0),
    QVector2D(0, 1),
    QVector2D(1, 0),
    QVector2D(1, 1)
};
const QVector3D ThreeDWidget::mVertices[4] = {
    QVector3D( 1.6,  1.2, 0),
    QVector3D( 1.6, -1.2, 0),
    QVector3D(-1.6,  1.2, 0),
    QVector3D(-1.6, -1.2, 0)
};


ThreeDWidget::ThreeDWidget(QWidget* parent)
    : QGLWidget(parent)
    , mXRot(DefaultXRot)
    , mYRot(DefaultYRot)
    , mXTrans(0.0f)
    , mYTrans(0.0f)
    , mZTrans(0.0f)
    , mZoom(DefaultZoom)
    , mTextureHandle(0)
#ifdef QT_OPENGL_ES_2
    , mShaderProgram(NULL)
#endif
{
    setFocus(Qt::OtherFocusReason);
    setCursor(Qt::OpenHandCursor);
}


ThreeDWidget::~ThreeDWidget(void)
{
    // ...
}


void ThreeDWidget::videoFrameReady(const QImage& frame)
{
    glBindTexture(GL_TEXTURE_2D, mTextureHandle);
    mTextureHandle = bindTexture(frame, GL_TEXTURE_2D);
    updateGL();
}


void ThreeDWidget::initializeGL(void)
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
#ifndef QT_OPENGL_ES_2
    glEnable(GL_TEXTURE_2D);
#endif

#ifdef QT_OPENGL_ES_2

#define PROGRAM_VERTEX_ATTRIBUTE 0
#define PROGRAM_TEXCOORD_ATTRIBUTE 1

    QGLShader* vshader = new QGLShader(QGLShader::Vertex, this);
    vshader->compileSourceFile(":/shaders/vertexshader.vsh");

    QGLShader *fshader = new QGLShader(QGLShader::Fragment, this);
    fshader->compileSourceFile(":/shaders/fragmentshader.vsh");

    program = new QGLShaderProgram(this);
    program->addShader(vshader);
    program->addShader(fshader);
    program->bindAttributeLocation("vertex", PROGRAM_VERTEX_ATTRIBUTE);
    program->bindAttributeLocation("texCoord", PROGRAM_TEXCOORD_ATTRIBUTE);
    program->link();

    program->bind();
    program->setUniformValue("texture", 0);

#endif

    glShadeModel(GL_SMOOTH);
    glEnable(GL_ALPHA_TEST);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
}


void ThreeDWidget::paintGL(void)
{
    qglClearColor(QColor(173, 164, 146));
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#ifndef QT_OPENGL_ES_2
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, mZoom);
    glRotatef(mXRot, 1.0f, 0.0f, 0.0f);
    glRotatef(mYRot, 0.0f, 1.0f, 0.0f);
    glTranslatef(mXTrans, mYTrans, mZTrans);
    glVertexPointer(3, GL_FLOAT, 0, mVertices);
    glTexCoordPointer(2, GL_FLOAT, 0, mTexCoords);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
#else
    QMatrix4x4 m;
    m.ortho(-0.5f, +0.5f, +0.5f, -0.5f, 4.0f, 15.0f);
    m.translate(0.0f, 0.0f, mZoom);
    m.rotate(mXRot, 1.0f, 0.0f, 0.0f);
    m.rotate(mYRot, 0.0f, 1.0f, 0.0f);
    m.translate(mXTrans, mYTrans, mZTrans);
    program->setUniformValue("matrix", m);
    program->enableAttributeArray(PROGRAM_VERTEX_ATTRIBUTE);
    program->enableAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE);
    program->setAttributeArray
        (PROGRAM_VERTEX_ATTRIBUTE, vertices.constData());
    program->setAttributeArray
        (PROGRAM_TEXCOORD_ATTRIBUTE, texCoords.constData());
#endif

    glBindTexture(GL_TEXTURE_2D, mTextureHandle);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}


void ThreeDWidget::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
#if !defined(QT_OPENGL_ES_2)
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
#ifndef QT_OPENGL_ES
    gluPerspective(38, (GLdouble)width/(GLdouble)height, 1, 100);
//    glOrtho(-0.5, +0.5, +0.5, -0.5, 4.0, 15.0);
#else
    glOrthof(-0.5, +0.5, +0.5, -0.5, 4.0, 15.0);
#endif
    glMatrixMode(GL_MODELVIEW);
#endif
}


void ThreeDWidget::keyPressEvent(QKeyEvent* e)
{
    const double incrTrans = ((e->modifiers() & Qt::ShiftModifier) == Qt::ShiftModifier)? 1 : 10;
    switch(e->key()) {
    case Qt::Key_Left:
        mXTrans -= incrTrans;
        updateGL();
        break;
    case Qt::Key_Right:
        mXTrans += incrTrans;
        updateGL();
        break;
    case Qt::Key_Down:
        mYTrans -= incrTrans;
        updateGL();
        break;
    case Qt::Key_Up:
        mYTrans += incrTrans;
        updateGL();
        break;
    case Qt::Key_PageDown:
        mZTrans -= incrTrans;
        updateGL();
        break;
    case Qt::Key_PageUp:
        mZTrans += incrTrans;
        updateGL();
        break;
    case Qt::Key_Escape:
        mZoom = DefaultZoom;
        mXRot = DefaultXRot;
        mYRot = DefaultYRot;
        mXTrans = 0.0;
        mYTrans = 0.0;
        mZTrans = 0.0;
        updateGL();
        break;
    default:
        break;
    }
}


void ThreeDWidget::mousePressEvent(QMouseEvent* e)
{
    setCursor(Qt::ClosedHandCursor);
    mLastPos = e->pos();
}


void ThreeDWidget::mouseReleaseEvent(QMouseEvent*)
{
    setCursor(Qt::OpenHandCursor);
}


void ThreeDWidget::wheelEvent(QWheelEvent* e)
{
    mZoom = mZoom + ((e->delta() < 0)? -0.2f : 0.2f);
    qDebug() << mZoom;
    updateGL();
}


void ThreeDWidget::mouseMoveEvent(QMouseEvent* e)
{
    if ((e->buttons() & Qt::LeftButton) == Qt::LeftButton) {
        setXRotation(mXRot + (e->y() - mLastPos.y()) / 2);
        setYRotation(mYRot + (e->x() - mLastPos.x()) / 2);
    }
    mLastPos = e->pos();
}


void ThreeDWidget::setXRotation(int angle)
{
    angle %= 360;
    if (angle != mXRot) {
        mXRot = angle;
        updateGL();
    }
}


void ThreeDWidget::setYRotation(int angle)
{
    angle %= 360;
    if (angle != mYRot) {
        mYRot = angle;
        updateGL();
    }
}
