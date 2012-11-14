// Copyright (c) 2012 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.

#include "3dwidget.h"

#include <QColor>
#include <QImage>
#include <QPixmap>
#include <QTimer>
#include <QtCore/QDebug>
#include <qmath.h>

#ifdef USE_SHADER
#include <QMatrix4x4>
#include <QGLShader>
#endif


const float ThreeDWidget::DefaultZoom = -2.0f;
const float ThreeDWidget::DefaultXRot = 180.0f;
const float ThreeDWidget::DefaultYRot = 0.0f;
const float ThreeDWidget::DefaultZRot = 0.0f;


const QVector2D ThreeDWidget::mOffset[9] = {
    QVector2D(1,  1), QVector2D(0 , 1), QVector2D(-1,  1),
    QVector2D(1,  0), QVector2D(0,  0), QVector2D(-1,  0),
    QVector2D(1, -1), QVector2D(0, -1), QVector2D(-1, -1)
};


GLfloat ThreeDWidget::mSharpeningKernel[9] = {
     0.0f, -0.5f,  0.0f,
    -0.5f,  3.0f, -0.5f,
     0.0f, -0.5f,  0.0f
};


const QVector2D ThreeDWidget::mTexCoords[4] = {
    QVector2D(0, 0),
    QVector2D(0, 1),
    QVector2D(1, 0),
    QVector2D(1, 1)
};


const QVector3D ThreeDWidget::mVertices[4] = {
    QVector3D(-1.6, -1.2, 0),
    QVector3D(-1.6,  1.2, 0),
    QVector3D( 1.6, -1.2, 0),
    QVector3D( 1.6,  1.2, 0)
};


ThreeDWidget::ThreeDWidget(QWidget* parent)
    : QGLWidget(parent)
    , mXRot(DefaultXRot)
    , mYRot(DefaultYRot)
    , mZRot(DefaultZRot)
    , mXTrans(0)
    , mYTrans(0)
    , mZTrans(0)
    , mZoom(DefaultZoom)
    , mVideoTextureHandle(0)
    , mDepthTextureHandle(0)
    , mDepthDataBuffer(NULL)
    #ifdef USE_SHADER
    , mShaderProgram(new QGLShaderProgram(this))
    #endif
    , mNearThreshold(0)
    , mFarThreshold(0xffffU)
{
    setFocus(Qt::OtherFocusReason);
    setCursor(Qt::OpenHandCursor);
    grabKeyboard();
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
}


ThreeDWidget::~ThreeDWidget()
{
#ifdef USE_SHADER
    delete mShaderProgram;
#endif
    if (mDepthDataBuffer)
        delete mDepthDataBuffer;
}


void ThreeDWidget::setVideoFrame(const XnUInt8* const pixels, int width, int height)
{
    mVideoFrameSize.setX(width);
    mVideoFrameSize.setY(height);
    glBindTexture(GL_TEXTURE_2D, mVideoTextureHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
}


void ThreeDWidget::setDepthFrame(const XnDepthPixel* const pixels, int width, int height)
{
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    if ((width != int(mDepthFrameSize.x()) || height != int(mDepthFrameSize.y()))) {
        mDepthFrameSize.setX(width);
        mDepthFrameSize.setY(height);
        if (mDepthDataBuffer)
            delete [] mDepthDataBuffer;
        mDepthDataBuffer = new GLuint[width * height];
    }

    GLuint* dst = mDepthDataBuffer;
    const XnDepthPixel* depthPixels = pixels;
//    const XnDepthPixel* const depthPixelsEnd = depthPixels + (width * height);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            GLuint v = GLuint(*depthPixels++);
            *dst++ = (y % 2 == 0)? qRgb(qRed(v), qGreen(v), qBlue(v)) : qRgb(255, 255, 255);
        }
    }

    glBindTexture(GL_TEXTURE_2D, mDepthTextureHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ARRAY_BUFFER, width, height, 0, GL_ARRAY_BUFFER, GL_UNSIGNED_BYTE, mDepthDataBuffer);

    glPopAttrib();
}


void ThreeDWidget::setThresholds(int nearThreshold, int farThreshold)
{
    mNearThreshold = nearThreshold;
    mFarThreshold = farThreshold;
}


void ThreeDWidget::initializeGL(void)
{
    glGenTextures(1, &mVideoTextureHandle);
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, mVideoTextureHandle);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glActiveTexture(GL_TEXTURE1);
    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &mDepthTextureHandle);
    glBindTexture(GL_TEXTURE_2D, mDepthTextureHandle);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glEnable(GL_DEPTH_TEST);
#ifndef USE_SHADER
    glEnable(GL_TEXTURE_2D);
#else
#define PROGRAM_VERTEX_ATTRIBUTE 0
#define PROGRAM_TEXCOORD_ATTRIBUTE 1
    QGLShader* vshader = new QGLShader(QGLShader::Vertex, this);
    vshader->compileSourceFile(":/shaders/wallvertexshader.glsl");
    QGLShader *fshader = new QGLShader(QGLShader::Fragment, this);
    fshader->compileSourceFile(":/shaders/wallfragmentshader.glsl");
    mShaderProgram->addShader(vshader);
    mShaderProgram->addShader(fshader);
    mShaderProgram->bindAttributeLocation("aVertex", PROGRAM_VERTEX_ATTRIBUTE);
    mShaderProgram->bindAttributeLocation("aTexCoord", PROGRAM_TEXCOORD_ATTRIBUTE);
    mShaderProgram->link();
    mShaderProgram->bind();
    const int videoTextureLocation = mShaderProgram->uniformLocation("uVideoTexture");
    const int depthTextureLocation = mShaderProgram->uniformLocation("uDepthTexture");
    mShaderProgram->setUniformValue(videoTextureLocation, (GLint)0);
    mShaderProgram->setUniformValue(depthTextureLocation, (GLint)1);
    mShaderProgram->setUniformValueArray("uOffset", mOffset, 9);
#endif
}


void ThreeDWidget::paintGL(void)
{
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#ifndef USE_SHADER
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, mZoom);
    glRotatef(mXRot, 1.0f, 0.0f, 0.0f);
    glRotatef(mYRot, 0.0f, 1.0f, 0.0f);
    glRotatef(mZRot, 0.0f, 0.0f, 1.0f);
    glTranslatef(mXTrans, mYTrans, mZTrans);
    glVertexPointer(3, GL_FLOAT, 0, mVertices);
    glTexCoordPointer(2, GL_FLOAT, 0, mTexCoords);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
#else
    QMatrix4x4 m;
    m.perspective((mFOVx + mFOVy) / 2, qreal(width()) / height(), 0.48, 12.0);
    m.translate(0.0f, 0.0f, mZoom);
    m.rotate(mXRot, 1.0f, 0.0f, 0.0f);
    m.rotate(mYRot, 0.0f, 1.0f, 0.0f);
    m.rotate(mZRot, 0.0f, 0.0f, 1.0f);
    m.translate(mXTrans, mYTrans, mZTrans);
    mShaderProgram->setUniformValue("uGamma", mGamma);
    mShaderProgram->setUniformValue("uMatrix", m);
    mShaderProgram->setUniformValue("uSize", mVideoFrameSize);
    mShaderProgram->setUniformValueArray("uSharpen", mSharpeningKernel, 9, 1);
    mShaderProgram->enableAttributeArray(PROGRAM_VERTEX_ATTRIBUTE);
    mShaderProgram->enableAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE);
    mShaderProgram->setAttributeArray(PROGRAM_VERTEX_ATTRIBUTE, mVertices);
    mShaderProgram->setAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE, mTexCoords);
#endif

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}


void ThreeDWidget::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
#ifndef USE_SHADER
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective((mFOVx + mFOVy) / 2, (GLdouble)width/(GLdouble)height, 0.48, 12.0);
    glMatrixMode(GL_MODELVIEW);
#endif
}


void ThreeDWidget::keyPressEvent(QKeyEvent* e)
{
    const float dTrans = (e->modifiers() & Qt::ShiftModifier)? 0.05f : 0.5f;
    const float dRot = (e->modifiers() & Qt::ShiftModifier)? 0.1f : 1.0f;
    switch(e->key()) {
    case Qt::Key_Left:
        mZRot -= dRot;
        updateGL();
        break;
    case Qt::Key_Right:
        mZRot += dRot;
        updateGL();
        break;
    case Qt::Key_Down:
        mZTrans -= dTrans;
        updateGL();
        break;
    case Qt::Key_Up:
        mZTrans += dTrans;
        updateGL();
        break;
    case Qt::Key_Escape:
        mZoom = DefaultZoom;
        mXRot = DefaultXRot;
        mYRot = DefaultYRot;
        mZRot = DefaultYRot;
        mXTrans = 0;
        mYTrans = 0;
        mZTrans = 0;
        updateGL();
        break;
    default:
        e->ignore();
        return;
    }
    e->accept();
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
        setXTranslation(mXTrans + 0.01f * (e->x() - mLastPos.x()));
        setYTranslation(mYTrans + 0.01f * (e->y() - mLastPos.y()));
    }
    mLastPos = e->pos();
}


void ThreeDWidget::setXTranslation(float x)
{
    if (!qFuzzyCompare(mXTrans, x)) {
        mXTrans = x;
        updateGL();
    }
}


void ThreeDWidget::setYTranslation(float y)
{
    if (!qFuzzyCompare(mYTrans, y)) {
        mYTrans = y;
        updateGL();
    }
}


void ThreeDWidget::setXRotation(float degrees)
{
    if (!qFuzzyCompare(degrees, mXRot)) {
        mXRot = degrees;
        updateGL();
    }
}


void ThreeDWidget::setYRotation(float degrees)
{
    if (!qFuzzyCompare(degrees, mYRot)) {
        mYRot = degrees;
        updateGL();
    }
}


void ThreeDWidget::setZRotation(float degrees)
{
    if (!qFuzzyCompare(degrees, mZRot)) {
        mYRot = degrees;
        updateGL();
    }
}


void ThreeDWidget::setZoom(float zoom)
{
    if (!qFuzzyCompare(zoom, mZoom)) {
        mZoom = zoom;
        updateGL();
    }
}


void ThreeDWidget::setGamma(double gradient)
{
    mGamma = (GLfloat)gradient;
    updateGL();
}


void ThreeDWidget::setFOV(float x, float y)
{
    mFOVx = x;
    mFOVy = y;
    updateGL();
}


void ThreeDWidget::setSharpening(int percent)
{
    const GLfloat intensity = -3.0f * GLfloat(percent) / 100.0f;
    mSharpeningKernel[1] = intensity;
    mSharpeningKernel[3] = intensity;
    mSharpeningKernel[4] = 1.0f + -4.0f * intensity;
    mSharpeningKernel[5] = intensity;
    mSharpeningKernel[7] = intensity;
    updateGL();
}
