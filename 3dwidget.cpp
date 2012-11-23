// Copyright (c) 2012 Oliver Lau <ola@ct.de>
// All rights reserved.

#include "3dwidget.h"

#include <QColor>
#include <QFile>
#include <QTextStream>
#include <QImage>
#include <QPixmap>
#include <QTimer>
#include <QDateTime>
#include <QMutexLocker>
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


const QVector3D ThreeDWidget::TooNearColor = QVector3D(138, 158, 9) / 255;
const QVector3D ThreeDWidget::TooFarColor = QVector3D(158, 9, 138) / 255;
const QVector3D ThreeDWidget::InvalidDepthColor = QVector3D(9, 138, 158) / 255;


ThreeDWidget::ThreeDWidget(QWidget* parent)
    : QGLWidget(parent)
    , mXRot(DefaultXRot)
    , mYRot(DefaultYRot)
    , mZRot(DefaultZRot)
    , mXTrans(0)
    , mYTrans(0)
    , mZTrans(0)
    , mZoom(DefaultZoom)
    , mVideoFrameLag(3)
    , mActiveVideoTexture(0)
    , mMergedDepthFrames(3)
    , mActiveDepthTexture(0)
    , mDepthFBO(NULL)
    , mImageFBO(NULL)
    , mImageDupFBO(NULL)
    , mDepthData(NULL)
    , mHaloRadius(1)
    , mHalo(NULL)
    , mDepthShaderProgram(new QGLShaderProgram(this))
    , mMixShaderProgram(new QGLShaderProgram(this))
    , mWallShaderProgram(new QGLShaderProgram(this))
    , mFilter(0)
    , mContrast(1.0f)
    , mSaturation(1.0f)
    , mGamma(2.1f)
    , mNearThreshold(0)
    , mFarThreshold(0xffff)
{
    setFocus(Qt::OtherFocusReason);
    setCursor(Qt::OpenHandCursor);
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
}


ThreeDWidget::~ThreeDWidget()
{
    delete mWallShaderProgram;
    delete mMixShaderProgram;
    delete mDepthShaderProgram;
    if (mDepthFBO)
        delete mDepthFBO;
    if (mImageFBO)
        delete mImageFBO;
    if (mImageDupFBO)
        delete mImageDupFBO;
    if (mDepthData)
        delete [] mDepthData;
    if (mHalo)
        delete [] mHalo;
}


void ThreeDWidget::setDepthFrame(const XnDepthPixel* const pixels, int width, int height)
{
    QMutexLocker locker(&mDepthShaderMutex);
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
    glViewport(0, 0, width, height);

    for (int i = 0; i < mMergedDepthFrames; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, mDepthTextureHandle[i]);
    }
    // aktuelle Textur festlegen
    glActiveTexture(GL_TEXTURE0 + mActiveDepthTexture);
    glBindTexture(GL_TEXTURE_2D, mDepthTextureHandle[mActiveDepthTexture]);
    // 16-Bit-Werte des Tiefenbilds in aktuelle Textur kopieren
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, pixels);
    if (++mActiveDepthTexture >= mMergedDepthFrames)
        mActiveDepthTexture = 0;

    // Tiefenwerte per GL-Shader in RGBA-Bild konvertieren
    QMatrix4x4 m;
    m.ortho(-1.6f, +1.6f, +1.2f, -1.2f, 0.1f, 15.0f);
    m.translate(0.0f, 0.0f, -10.0f);
    mDepthShaderProgram->bind();
    mDepthShaderProgram->setUniformValue("uMatrix", m);
    mDepthShaderProgram->setUniformValue("uNearThreshold", (GLfloat)mNearThreshold);
    mDepthShaderProgram->setUniformValue("uFarThreshold", (GLfloat)mFarThreshold);
    mDepthShaderProgram->setUniformValue("uSize", mVideoFrameSize);
    // Framebufferobjekt als Ziel für Zeichenbefehle festlegen
    mDepthFBO->bind();
    // Fläche zeichnen
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    // GL-Framebuffer nach mDepthData (RGBA) kopieren
    glReadPixels(0, 0, width, height, GL_BGRA, GL_UNSIGNED_BYTE, mDepthData);
    mDepthFBO->release();
    // aus RGBA-Daten ein QImage produzieren und verschicken
    emit depthFrameReady(QImage((uchar*)mDepthData, width, height, QImage::Format_RGB32));

    // RGBA-Daten des Tiefenbildes in aktuelle Textur kopieren
    glBindTexture(GL_TEXTURE_2D, mDepthTextureHandle[mActiveDepthTexture]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, mDepthData);

    // Zustande der GL-Engine von Beginn dieser Methode wiederherstellen
    glPopAttrib();
}


void ThreeDWidget::setVideoFrame(const XnUInt8* const pixels, int width, int height)
{
    QMutexLocker locker(&mVideoShaderMutex);
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
    glViewport(0, 0, width, height);
    glActiveTexture(GL_TEXTURE0);
    int nextActiveVideoTexture = mActiveVideoTexture + 1;
    if (nextActiveVideoTexture >= mVideoFrameLag)
        nextActiveVideoTexture = 0;
    glBindTexture(GL_TEXTURE_2D, mVideoTextureHandle[mActiveVideoTexture]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);

    glBindTexture(GL_TEXTURE_2D, mVideoTextureHandle[nextActiveVideoTexture]);
    mActiveVideoTexture = nextActiveVideoTexture;

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mDepthTextureHandle[mActiveDepthTexture]);
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
}


void ThreeDWidget::setVideoFrameLag(int lag)
{
    QMutexLocker locker(&mVideoShaderMutex);
    mVideoFrameLag = lag;
}


void ThreeDWidget::setMergedDepthFrames(int n)
{
    mMergedDepthFrames = n;
    makeDepthShader();
}


void ThreeDWidget::setThresholds(int nearThreshold, int farThreshold)
{
    if (nearThreshold >= farThreshold)
        return;
    mNearThreshold = nearThreshold;
    mFarThreshold = farThreshold;
    updateGL();
}


template <typename T>
T square(T x) { return x * x; }


void ThreeDWidget::makeDepthShader(void)
{
    QMutexLocker locker(&mDepthShaderMutex);
    qDebug() << "Making mDepthShaderProgram w/ halo radius of" << mHaloRadius << "...";
    if (mHalo)
        delete [] mHalo;
    const int haloArraySize = square(2 * mHaloRadius);
    mHalo = new QVector2D[haloArraySize];
    int i = 0;
    for (int x = -mHaloRadius; x < mHaloRadius; ++x)
        for (int y = -mHaloRadius; y < mHaloRadius; ++y)
            mHalo[i++] = QVector2D(x, y);
    QFile file(":/shaders/depthfragmentshader.glsl");
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QTextStream in(&file);
    const QString& fragmentShaderSource = in.readAll().arg(haloArraySize).arg(mMergedDepthFrames);
    mDepthShaderProgram->removeAllShaders();
    mDepthShaderProgram->addShaderFromSourceCode(QGLShader::Fragment, fragmentShaderSource);
    mDepthShaderProgram->addShaderFromSourceFile(QGLShader::Vertex, ":/shaders/depthvertexshader.glsl");
    mDepthShaderProgram->bindAttributeLocation("aVertex", PROGRAM_VERTEX_ATTRIBUTE);
    mDepthShaderProgram->bindAttributeLocation("aTexCoord", PROGRAM_TEXCOORD_ATTRIBUTE);
    mDepthShaderProgram->enableAttributeArray(PROGRAM_VERTEX_ATTRIBUTE);
    mDepthShaderProgram->enableAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE);
    mDepthShaderProgram->setAttributeArray(PROGRAM_VERTEX_ATTRIBUTE, mVertices);
    mDepthShaderProgram->setAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE, mTexCoords);
    mDepthShaderProgram->link();
    mDepthShaderProgram->bind();
    for (int i = 0; i < mMergedDepthFrames; ++i)
        mDepthShaderProgram->setUniformValue(QString("uDepthTexture[%1]").arg(i).toLatin1().constData(), (GLuint)i);
    mDepthShaderProgram->setUniformValueArray("uHalo", mHalo, haloArraySize);
    mDepthShaderProgram->setUniformValue("uTooNearColor", TooNearColor);
    mDepthShaderProgram->setUniformValue("uTooFarColor", TooFarColor);
    mDepthShaderProgram->setUniformValue("uInvalidDepthColor", InvalidDepthColor);
}


void ThreeDWidget::makeMixShader(void)
{
    qDebug() << "Making mMixShaderProgram ...";
    mMixShaderProgram->addShaderFromSourceFile(QGLShader::Fragment, ":/shaders/mixfragmentshader.glsl");
    mMixShaderProgram->addShaderFromSourceFile(QGLShader::Vertex, ":/shaders/mixvertexshader.glsl");
    mMixShaderProgram->bindAttributeLocation("aVertex", PROGRAM_VERTEX_ATTRIBUTE);
    mMixShaderProgram->bindAttributeLocation("aTexCoord", PROGRAM_TEXCOORD_ATTRIBUTE);
    mMixShaderProgram->enableAttributeArray(PROGRAM_VERTEX_ATTRIBUTE);
    mMixShaderProgram->enableAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE);
    mMixShaderProgram->setAttributeArray(PROGRAM_VERTEX_ATTRIBUTE, mVertices);
    mMixShaderProgram->setAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE, mTexCoords);
    mMixShaderProgram->link();
    mMixShaderProgram->bind();
    mMixShaderProgram->setUniformValue("uVideoTexture", (GLuint)0);
    mMixShaderProgram->setUniformValue("uDepthTexture", (GLuint)1);
    mMixShaderProgram->setUniformValue("uImageTexture", (GLuint)2);
    mMixShaderProgram->setUniformValueArray("uOffset", mOffset, 9);
    mMixShaderProgram->setUniformValue("uTooNearColor", TooNearColor);
    mMixShaderProgram->setUniformValue("uTooFarColor", TooFarColor);
    mMixShaderProgram->setUniformValue("uInvalidDepthColor", InvalidDepthColor);
}


void ThreeDWidget::makeWallShader(void)
{
    qDebug() << "Making mWallShaderProgram ...";
    mWallShaderProgram->addShaderFromSourceFile(QGLShader::Fragment, ":/shaders/wallfragmentshader.glsl");
    mWallShaderProgram->addShaderFromSourceFile(QGLShader::Vertex, ":/shaders/wallvertexshader.glsl");
    mWallShaderProgram->bindAttributeLocation("aVertex", PROGRAM_VERTEX_ATTRIBUTE);
    mWallShaderProgram->bindAttributeLocation("aTexCoord", PROGRAM_TEXCOORD_ATTRIBUTE);
    mWallShaderProgram->enableAttributeArray(PROGRAM_VERTEX_ATTRIBUTE);
    mWallShaderProgram->enableAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE);
    mWallShaderProgram->setAttributeArray(PROGRAM_VERTEX_ATTRIBUTE, mVertices);
    mWallShaderProgram->setAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE, mTexCoords);
    mWallShaderProgram->link();
    mWallShaderProgram->bind();
    mWallShaderProgram->setUniformValue("uImageTexture", (GLuint)0);
}


void ThreeDWidget::initializeGL(void)
{
    GLint GLMajorVer, GLMinorVer;
    glGetIntegerv(GL_MAJOR_VERSION, &GLMajorVer);
    glGetIntegerv(GL_MINOR_VERSION, &GLMinorVer);
    qDebug() << QString("OpenGL %1.%2").arg(GLMajorVer).arg(GLMinorVer);

    glActiveTexture = (PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture");
    glGenSamplers = (PFNGLGENSAMPLERSPROC)wglGetProcAddress("glGenSamplers");
    glSamplerParameteri = (PFNGLSAMPLERPARAMETERIPROC)wglGetProcAddress("glSamplerParameteri");
    glBindSampler = (PFNGLBINDSAMPLERPROC)wglGetProcAddress("glBindSampler");

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_TEXTURE_2D);
    glDepthMask(GL_FALSE);

    glGenTextures(MaxMergedDepthFrames, mDepthTextureHandle);
    for (int i = 0; i < MaxMergedDepthFrames; ++i) {
        glBindTexture(GL_TEXTURE_2D, mDepthTextureHandle[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    }

    glGenTextures(MaxVideoFrameLag, mVideoTextureHandle);
    for (int i = 0; i < MaxVideoFrameLag; ++i) {
        glBindTexture(GL_TEXTURE_2D, mVideoTextureHandle[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    }

    makeDepthShader();
    makeMixShader();
    makeWallShader();
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


void ThreeDWidget::resetTransformations(void)
{
    mZoom = DefaultZoom;
    mXRot = DefaultXRot;
    mYRot = DefaultYRot;
    mZRot = DefaultYRot;
    mXTrans = 0;
    mYTrans = 0;
    mZTrans = 0;
    updateGL();
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


void ThreeDWidget::setContrast(GLfloat contrast)
{
    mContrast = contrast;
}


void ThreeDWidget::setSaturation(GLfloat saturation)
{
    mSaturation = saturation;
}


void ThreeDWidget::setGamma(double gamma)
{
    mGamma = (GLfloat)gamma;
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


void ThreeDWidget::setHaloRadius(int radius)
{
    mHaloRadius = radius;
    makeDepthShader();
}
