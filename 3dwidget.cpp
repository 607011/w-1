// Copyright (c) 2012 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.

#include "3dwidget.h"

#include <QColor>
#include <QImage>
#include <QPixmap>
#include <QTimer>
#include <QDateTime>
#include <QtCore/QDebug>
#include <QMatrix4x4>
#include <QGLShader>
#include <qmath.h>


static const int PROGRAM_VERTEX_ATTRIBUTE = 0;
static const int PROGRAM_TEXCOORD_ATTRIBUTE = 1;

const float ThreeDWidget::DefaultZoom = -2.45f;
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
    , mDepthFBO(NULL)
    , mImageFBO(NULL)
    , mImageDupFBO(NULL)
    , mDepthData(NULL)
    , mDepthShaderProgram(new QGLShaderProgram(this))
    , mMixShaderProgram(new QGLShaderProgram(this))
    , mWallShaderProgram(new QGLShaderProgram(this))
    , mFilter(0)
    , mContrast(1.0f)
    , mSaturation(1.0f)
    , mGamma(2.1f)
    , mNearThreshold(0)
    , mFarThreshold(0xffffU)
{
    setFocus(Qt::OtherFocusReason);
    setCursor(Qt::OpenHandCursor);
    // grabKeyboard();
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
}


ThreeDWidget::~ThreeDWidget()
{
    delete mWallShaderProgram;
    delete mDepthShaderProgram;
    if (mDepthFBO)
        delete mDepthFBO;
    if (mImageFBO)
        delete mImageFBO;
    if (mImageDupFBO)
        delete mImageDupFBO;
    if (mDepthData)
        delete [] mDepthData;
}


void ThreeDWidget::setVideoFrame(const XnUInt8* const pixels, int width, int height)
{
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    if ((width != int(mVideoFrameSize.x()) || height != int(mVideoFrameSize.y()))) {
        mVideoFrameSize.setX(width);
        mVideoFrameSize.setY(height);
        if (mImageFBO)
            delete mImageFBO;
        mImageFBO = new QGLFramebufferObject(width, height);
        if (mImageDupFBO)
            delete mImageDupFBO;
        mImageDupFBO = new QGLFramebufferObject(width, height);
    }
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mVideoTextureHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mDepthTextureHandle);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, mImageFBO->texture());

    QMatrix4x4 m;
    m.ortho(-1.6f, +1.6f, +1.2f, -1.2f, 0.1f, 15.0f);
    m.translate(0.0f, 0.0f, -10.0f);
    mMixShaderProgram->bind();
    mMixShaderProgram->setUniformValue("uMatrix", m);
    mMixShaderProgram->setUniformValue("uGamma", mGamma);
    mMixShaderProgram->setUniformValue("uContrast", mContrast);
    mMixShaderProgram->setUniformValue("uSaturation", mSaturation);
    mMixShaderProgram->setUniformValue("uSize", mVideoFrameSize);
    mMixShaderProgram->setUniformValueArray("uSharpen", mSharpeningKernel, 9, 1);
    mImageDupFBO->bind();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    mImageDupFBO->release();

    QRect rect(0, 0, width, height);
    QGLFramebufferObject::blitFramebuffer(mImageFBO, rect, mImageDupFBO, rect);

    glPopAttrib();
    updateGL();
}


void ThreeDWidget::setDepthFrame(const XnDepthPixel* const pixels, int width, int height)
{
    // alle Zustände der GL-Engine sichern
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    if ((width != int(mDepthFrameSize.x()) || height != int(mDepthFrameSize.y()))) {
        mDepthFrameSize.setX(width);
        mDepthFrameSize.setY(height);
        if (mDepthFBO)
            delete mDepthFBO;
        mDepthFBO = new QGLFramebufferObject(width, height);
        if (mDepthData)
            delete mDepthData;
        mDepthData = new GLuint[width * height];
    }
    // aktuelle Textur festlegen
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mDepthTextureHandle);
    // 16-Bit-Werte des Tiefenbilds in aktuelle Textur kopieren
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, pixels);
    // Tiefenwerte per GL-Shader in RGBA-Bild konvertieren
    QMatrix4x4 m;
    m.ortho(-1.6f, +1.6f, +1.2f, -1.2f, 0.1f, 15.0f);
    m.translate(0.0f, 0.0f, -10.0f);
    mDepthShaderProgram->bind();
    mDepthShaderProgram->setUniformValue("uMatrix", m);
    mDepthShaderProgram->setUniformValue("uNearThreshold", (GLfloat)mNearThreshold);
    mDepthShaderProgram->setUniformValue("uFarThreshold", (GLfloat)mFarThreshold);
    // Framebufferobjekt als Ziel für Zeichenbefehle festlegen
    mDepthFBO->bind();
    // Fläche zeichnen
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    // GL-Framebuffer nach mDepthData (RGBA) kopieren
    glReadPixels(0, 0, width, height, GL_BGRA, GL_UNSIGNED_BYTE, mDepthData);
    mDepthFBO->release();
    // aus RGBA-Daten ein QImage produzieren und verschicken
    emit depthFrameReady(QImage((uchar*)mDepthData, width, height, QImage::Format_RGB32));

    // RGBA-Daten in aktuelle Textur kopieren
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mDepthTextureHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, mDepthData);

    // Zustande der GL-Engine von Beginn dieser Methode wiederherstellen
    glPopAttrib();
}


void ThreeDWidget::setThresholds(int nearThreshold, int farThreshold)
{
    mNearThreshold = nearThreshold;
    mFarThreshold = farThreshold;
    updateGL();
}


void ThreeDWidget::initializeGL(void)
{
    GLint GLMajorVer, GLMinorVer;
    glGetIntegerv(GL_MAJOR_VERSION, &GLMajorVer);
    glGetIntegerv(GL_MINOR_VERSION, &GLMinorVer);
    qDebug() << QString("OpenGL %1.%2").arg(GLMajorVer).arg(GLMinorVer);

    glActiveTexture = (PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture");

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_TEXTURE_2D);

    glGenTextures(1, &mDepthTextureHandle);
    glBindTexture(GL_TEXTURE_2D, mDepthTextureHandle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glGenTextures(1, &mVideoTextureHandle);
    glBindTexture(GL_TEXTURE_2D, mVideoTextureHandle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    qDebug() << "Making mDepthShaderProgram ...";
    mDepthShaderProgram->addShaderFromSourceFile(QGLShader::Vertex, ":/shaders/depthvertexshader.glsl");
    mDepthShaderProgram->addShaderFromSourceFile(QGLShader::Fragment, ":/shaders/depthfragmentshader.glsl");
    mDepthShaderProgram->bindAttributeLocation("aVertex", PROGRAM_VERTEX_ATTRIBUTE);
    mDepthShaderProgram->bindAttributeLocation("aTexCoord", PROGRAM_TEXCOORD_ATTRIBUTE);
    mDepthShaderProgram->enableAttributeArray(PROGRAM_VERTEX_ATTRIBUTE);
    mDepthShaderProgram->enableAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE);
    mDepthShaderProgram->setAttributeArray(PROGRAM_VERTEX_ATTRIBUTE, mVertices);
    mDepthShaderProgram->setAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE, mTexCoords);
    mDepthShaderProgram->link();
    mDepthShaderProgram->bind();
    mDepthShaderProgram->setUniformValue("uDepthTexture", 1);

    qDebug() << "Making mMixShaderProgram ...";
    mMixShaderProgram->addShaderFromSourceFile(QGLShader::Vertex, ":/shaders/mixvertexshader.glsl");
    mMixShaderProgram->addShaderFromSourceFile(QGLShader::Fragment, ":/shaders/mixfragmentshader.glsl");
    mMixShaderProgram->bindAttributeLocation("aVertex", PROGRAM_VERTEX_ATTRIBUTE);
    mMixShaderProgram->bindAttributeLocation("aTexCoord", PROGRAM_TEXCOORD_ATTRIBUTE);
    mMixShaderProgram->enableAttributeArray(PROGRAM_VERTEX_ATTRIBUTE);
    mMixShaderProgram->enableAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE);
    mMixShaderProgram->setAttributeArray(PROGRAM_VERTEX_ATTRIBUTE, mVertices);
    mMixShaderProgram->setAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE, mTexCoords);
    mMixShaderProgram->link();
    mMixShaderProgram->bind();
    mMixShaderProgram->setUniformValue("uVideoTexture", 0);
    mMixShaderProgram->setUniformValue("uDepthTexture", 1);
    mMixShaderProgram->setUniformValue("uImageTexture", 2);
    mMixShaderProgram->setUniformValueArray("uOffset", mOffset, 9);

    qDebug() << "Making mWallShaderProgram ...";
    mWallShaderProgram->addShaderFromSourceFile(QGLShader::Vertex, ":/shaders/wallvertexshader.glsl");
    mWallShaderProgram->addShaderFromSourceFile(QGLShader::Fragment, ":/shaders/wallfragmentshader.glsl");
    mWallShaderProgram->bindAttributeLocation("aVertex", PROGRAM_VERTEX_ATTRIBUTE);
    mWallShaderProgram->bindAttributeLocation("aTexCoord", PROGRAM_TEXCOORD_ATTRIBUTE);
    mWallShaderProgram->enableAttributeArray(PROGRAM_VERTEX_ATTRIBUTE);
    mWallShaderProgram->enableAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE);
    mWallShaderProgram->setAttributeArray(PROGRAM_VERTEX_ATTRIBUTE, mVertices);
    mWallShaderProgram->setAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE, mTexCoords);
    mWallShaderProgram->link();
    mWallShaderProgram->bind();
    mWallShaderProgram->setUniformValue("uImageTexture", 0);
}


void ThreeDWidget::paintGL(void)
{
    if (mImageFBO == NULL)
        return;
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    QMatrix4x4 m;
    m.perspective((mFOVx + mFOVy) / 2, qreal(width()) / height(), 0.48, 12.0);
    m.translate(0.0f, 0.0f, mZoom);
    m.rotate(mXRot, 1.0f, 0.0f, 0.0f);
    m.rotate(mYRot, 0.0f, 1.0f, 0.0f);
    m.rotate(mZRot, 0.0f, 0.0f, 1.0f);
    m.translate(mXTrans, mYTrans, mZTrans);
    mWallShaderProgram->bind();
    mWallShaderProgram->setUniformValue("uMatrix", m);
    mWallShaderProgram->setUniformValue("uFilter", mFilter);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mImageFBO->texture());
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}


void ThreeDWidget::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
}


void ThreeDWidget::keyPressEvent(QKeyEvent* e)
{
    const float dTrans = (e->modifiers() & Qt::ShiftModifier)? 0.05f : 0.5f;
    const float dRot = (e->modifiers() & Qt::ShiftModifier)? 0.1f : 1.0f;
    switch (e->key()) {
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
    const float dZoom = (e->modifiers() & Qt::ShiftModifier)? 0.04f : 0.2f;
    setZoom(mZoom + ((e->delta() < 0)? -dZoom : dZoom));
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


void ThreeDWidget::setFilter(int index)
{
    mFilter = (GLint)index;
    updateGL();
}


void ThreeDWidget::setContrast(int contrast)
{
    mContrast = 1e-2f * GLfloat(contrast);
    qDebug() << "mContrast =" << mContrast;
    updateGL();
}


void ThreeDWidget::setSaturation(int saturation)
{
    mSaturation = 1e-2f * GLfloat(saturation);
    qDebug() << "mSaturation =" << mSaturation;
    updateGL();
}


void ThreeDWidget::setGamma(double gamma)
{
    mGamma = (GLfloat)gamma;
    qDebug() << "mGamma =" << mGamma;
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
