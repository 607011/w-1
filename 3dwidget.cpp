// Copyright (c) 2012 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.

#include "3dwidget.h"

#include <QColor>
#include <QImage>
#include <QPixmap>
#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <qmath.h>

#ifdef USE_SHADER
#include <QMatrix4x4>
#include <QGLShader>
#endif


const float ThreeDWidget::DefaultZoom = -3.4f;
const float ThreeDWidget::DefaultXRot = 180.0f;
const float ThreeDWidget::DefaultYRot = 0.0f;
const float ThreeDWidget::DefaultZRot = 0.0f;
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
    , mZRot(DefaultZRot)
    , mXTrans(0.0f)
    , mYTrans(0.0f)
    , mZTrans(0.0f)
    , mZoom(DefaultZoom)
    , mTextureHandle(0)
#ifdef USE_SHADER
    , mShaderProgram(NULL)
#endif
{
    setFocus(Qt::OtherFocusReason);
    setCursor(Qt::OpenHandCursor);
    grabKeyboard();
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
}


void ThreeDWidget::videoFrameReady(const QImage& frame)
{
    glBindTexture(GL_TEXTURE_2D, mTextureHandle);
    if (mTextureHandle)
        deleteTexture(mTextureHandle);
    mTextureHandle = bindTexture(frame, GL_TEXTURE_2D);
    updateGL();
}


void ThreeDWidget::initializeGL(void)
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
#ifndef USE_SHADER
    glEnable(GL_TEXTURE_2D);
#else
#define PROGRAM_VERTEX_ATTRIBUTE 0
#define PROGRAM_TEXCOORD_ATTRIBUTE 1
    QGLShader* vshader = new QGLShader(QGLShader::Vertex, this);
    vshader->compileSourceFile(":/shaders/vertexshader.vsh");
    QGLShader *fshader = new QGLShader(QGLShader::Fragment, this);
    fshader->compileSourceFile(":/shaders/fragmentshader.fsh");
    mShaderProgram = new QGLShaderProgram(this);
    mShaderProgram->addShader(vshader);
    mShaderProgram->addShader(fshader);
    mShaderProgram->bindAttributeLocation("vertex", PROGRAM_VERTEX_ATTRIBUTE);
    mShaderProgram->bindAttributeLocation("texCoord", PROGRAM_TEXCOORD_ATTRIBUTE);
    mShaderProgram->link();
    mShaderProgram->bind();
    mShaderProgram->setUniformValue("texture", 0);
#endif
}


void ThreeDWidget::paintGL(void)
{
    qglClearColor(QColor(173, 164, 146));
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#ifndef USE_SHADER
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
    m.perspective(38, qreal(width()) / height(), 1, 100);
    m.translate(0.0f, 0.0f, mZoom);
    m.rotate(mXRot, 1.0f, 0.0f, 0.0f);
    m.rotate(mYRot, 0.0f, 1.0f, 0.0f);
    m.rotate(mZRot, 0.0f, 0.0f, 1.0f);
    m.translate(mXTrans, mYTrans, mZTrans);
    mShaderProgram->setUniformValue("matrix", m);
    mShaderProgram->enableAttributeArray(PROGRAM_VERTEX_ATTRIBUTE);
    mShaderProgram->enableAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE);
    mShaderProgram->setAttributeArray(PROGRAM_VERTEX_ATTRIBUTE, mVertices);
    mShaderProgram->setAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE, mTexCoords);
#endif

    glBindTexture(GL_TEXTURE_2D, mTextureHandle);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}


void ThreeDWidget::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
#ifndef USE_SHADER
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(38, (GLdouble)width/(GLdouble)height, 1, 100);
    glMatrixMode(GL_MODELVIEW);
#endif
}


void ThreeDWidget::keyPressEvent(QKeyEvent* e)
{
    const float incrTrans = (e->modifiers() & Qt::ShiftModifier)? 0.2f : 1.0f;
    switch(e->key()) {
    case Qt::Key_Left:
        mZRot -= incrTrans;
        updateGL();
        break;
    case Qt::Key_Right:
        mZRot += incrTrans;
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
        mZRot = DefaultYRot;
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
    setZoom(mZoom + ((e->delta() < 0)? -0.2f : 0.2f));
    updateGL();
}


void ThreeDWidget::mouseMoveEvent(QMouseEvent* e)
{
    if (e->buttons() & Qt::LeftButton) {
        setXRotation(mXRot + 0.333f * (e->y() - mLastPos.y()));
        setYRotation(mYRot + 0.333f * (e->x() - mLastPos.x()));
    }
    else if (e->buttons() & Qt::RightButton) {
        setTranslation(mXTrans + 0.01f * (e->x() - mLastPos.x()), mYTrans + 0.01f * (e->y() - mLastPos.y()));
    }
    mLastPos = e->pos();
}


void ThreeDWidget::setTranslation(float x, float y)
{
    mXTrans = x;
    mYTrans = y;
    updateGL();
}


void ThreeDWidget::setXRotation(float degrees)
{
    if (degrees != mXRot) {
        mXRot = degrees;
        qDebug() << "mXRot =" << mXRot;
        updateGL();
    }
}


void ThreeDWidget::setYRotation(float degrees)
{
    if (degrees != mYRot) {
        mYRot = degrees;
        qDebug() << "mYRot =" << mYRot;
        updateGL();
    }
}


void ThreeDWidget::setZoom(float zoom)
{
    if (!qFuzzyCompare(zoom, mZoom)) {
        mZoom = zoom;
        qDebug() << "mZoom =" << mZoom;
        updateGL();
    }
}
