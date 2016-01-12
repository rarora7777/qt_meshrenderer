#define EPSILON 0.00005

#include "mainwindow.h"
#include "control.h"

#include <QApplication>
#include <QtGui/QMatrix4x4>
#include <QtGui/QVector3D>
#include <QPointF>
#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/QScreen>
#include <QWidget>
#include <QFileDialog>
#include <QKeyEvent>

#include <QtGui/QImage>

#include <QtOpenGL/QGLWidget>

#include <QtCore/qmath.h>

#include <QtWidgets>

#include <QTime>
#include <QDebug>

#include <fstream>

using namespace std;

ControlWindow *ctrlWindow;
//! [1]


WarpWindow::WarpWindow(char *inputFile)
    : m_program(0)
    , m_frame(0)
    , m_translate(0.0)
    , activeOrigImage(0)
{
    loadInputData(inputFile);
    resize(2*i_width, i_height);
    m_ctrlWindowSize = 700;
}

WarpWindow::~WarpWindow()
{
    for (unsigned int iter=0; iter<i_numImage; ++iter)
    {
        delete[] i_tri[iter];
        delete[] uvBufferData[iter];
        delete[] origVertexBufferData[iter];
        delete[] warpVertexBufferData[iter];
        delete[] origAffBufferData[iter];
        delete[] warpAffBufferData[iter];
        if (warpType)
            delete warpPositions[iter];

        for (unsigned int iter2=0; iter2<i_numImage; ++iter2)
        {
            delete[] i_vert[iter][iter2];
            delete[] i_affinity[iter][iter2];
        }
    }
    if (warpType)
        delete[] warpPositions;
}

void WarpWindow::visibilityChanged(QWindow::Visibility visibility)
{
    if(!visibility)
    {
        if (ctrlWindow->isVisible())
            ctrlWindow->close();

        QApplication::quit();
    }
}

bool WarpWindow::event(QEvent *event)
{
    QKeyEvent *key;
    switch (event->type())
    {
        case QEvent::KeyPress:
            key = static_cast<QKeyEvent *>(event);
            switch (key->key())
            {
//                case Qt::Key_Up:
//                    m_translate += 1.0;
//                    break;
//                case Qt::Key_Down:
//                    m_translate -= 1.0;
//                    break;
                //exit
                case Qt::Key_Escape:
                    exit(0);
                //pause/unpause animation
                case Qt::Key_P:
                case Qt::Key_Space:
                    ctrlWindow->toggleAnimation();
//                    playAnimation = !playAnimation;
//                    if (externalAlpha)
//                    {
//                        externalAlpha = false;
//                        for (unsigned int iter=0; iter<i_numImage; ++iter)
//                            warpAlpha[iter] = warpAlphaBegin[iter];
//                    }
                    break;
                case Qt::Key_Left:
                    activeOrigImage = (activeOrigImage-1)%i_numImage;
                    break;
                case Qt::Key_Right:
                    activeOrigImage = (activeOrigImage+1)%i_numImage;
                    break;
                case Qt::Key_W:
                    showWireframe = !showWireframe;
                    glUniform1i(m_showWireframeUniform, (GLuint)showWireframe);
                    break;
                //save the current render to an image
                case Qt::Key_S:
                    screenshotTrigger = true;
                    break;
//                case Qt::Key_A:
//                    ctrlWindow->toggleArrangement(warpPositions);
//                    break;
                case Qt::Key_1:
                    blendImage[0] = !blendImage[0];
                    showBlendAlphasTrigger = true;
                    break;
                case Qt::Key_2:
                    blendImage[1] = !blendImage[1];
                    showBlendAlphasTrigger = true;
                    break;
                case Qt::Key_3:
                    blendImage[2] = !blendImage[2];
                    showBlendAlphasTrigger = true;
                    break;
                case Qt::Key_4:
                    blendImage[3] = !blendImage[3];
                    showBlendAlphasTrigger = true;
                    break;
                case Qt::Key_5:
                    blendImage[4] = !blendImage[4];
                    showBlendAlphasTrigger = true;
                    break;
                case Qt::Key_6:
                    blendImage[5] = !blendImage[5];
                    showBlendAlphasTrigger = true;
                    break;
                case Qt::Key_7:
                    blendImage[6] = !blendImage[6];
                    showBlendAlphasTrigger = true;
                    break;
                case Qt::Key_8:
                    blendImage[7] = !blendImage[7];
                    showBlendAlphasTrigger = true;
                    break;
                case Qt::Key_9:
                    blendImage[8] = !blendImage[8];
                    showBlendAlphasTrigger = true;
                    break;
                case Qt::Key_0:
                    blendImage[9] = !blendImage[9];
                    showBlendAlphasTrigger = true;
                    break;
                case Qt::Key_T:
                    thinningState = !thinningState;
                    break;
            }
            return true;
        default:
            return OpenGLWindow::event(event);
    }
}
//! [1]

//! [2]
int main(int argc, char **argv)
{

    QApplication app(argc, argv);

    QSurfaceFormat format;
    format.setSamples(4);

    char *inputFile;

    if (argc>=2)
        inputFile = argv[1];
    else
    {
        inputFile = new char[20];
        strcpy_s(inputFile, 20, "O_12_input.txt");
    }

    WarpWindow mainWindow(inputFile);


    mainWindow.setFormat(format);
    mainWindow.show();

    mainWindow.setAnimating(true);


    QGraphicsScene scene(-mainWindow.m_ctrlWindowSize/2, -mainWindow.m_ctrlWindowSize/2, mainWindow.m_ctrlWindowSize, mainWindow.m_ctrlWindowSize);
    ctrlWindow = new ControlWindow(&mainWindow, &scene);
    ctrlWindow->setRenderHint(QPainter::Antialiasing);
    ctrlWindow->setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    ctrlWindow->setBackgroundBrush(QColor(255, 255, 255));
    ctrlWindow->setWindowTitle("Control");
    ctrlWindow->show();

    return app.exec();
}
//! [2]


//! [3]
static const char *vertexShaderSource =
        "#version 330 core\n"
        "// Input vertex data, different for all executions of this shader.\n"
        "layout(location = 0) in vec3 vertexPosition_modelspace;\n"
        "layout(location = 1) in vec2 vertexUV;\n"
        "layout(location = 2) in vec3 barycentric;\n"
        "layout(location = 3) in float affinity;\n"
        "// Output data ; will be interpolated for each fragment.\n"
        "out vec2 UV;\n"
        "out vec2 UV_FBO;\n"
        "out vec3 vBC;\n"
        "out float aff;\n"
        "// Values that stay constant for the whole mesh.\n"
        "uniform mat4 MVP;\n"
        "uniform int showWireframe;\n"
        "void main(){\n"
            "// Output position of the vertex, in clip space : MVP * position\n"
            "gl_Position =  MVP * vec4(vertexPosition_modelspace,1);\n"
            "// UV of the vertex. No special space for this one.\n"
            "UV = vertexUV;\n"
            "//pass barycentric coordinates for interpolation\n"
            "if (showWireframe==1)"
                "vBC = barycentric;\n"
            "else\n"
                "vBC = vec3(1);\n"
            "//pass FBO UV coordinates according to vertex position in modelspace\n"
            "UV_FBO = vec2(0.5*vertexPosition_modelspace.x, 0.5+0.5*vertexPosition_modelspace.y);\n"
            "//simply copy affinites to fragment shader inputs, which are then interpolated\n"
            "aff = affinity;\n"
        "}\n";

static const char *fragmentShaderSource =
        "#version 330 core\n"
        "// Interpolated values from the vertex shaders\n"
        "in vec2 UV;\n"
        "in vec2 UV_FBO;\n"
        "in vec3 vBC;\n"
        "in float aff;\n"
        "// Output data\n"
        "layout (location = 0) out vec4 color;\n"
        "// Values that stay constant for the whole mesh.\n"
        "uniform sampler2D srcTexture;\n"
        "uniform sampler2D fboTexture;\n"
        "uniform float alpha;\n"
        "uniform int blend;\n"
        "uniform float affExponent;\n"
        "uniform float histEqMult;\n"
        "uniform int isMaxAlpha;"
        "void main(){\n"
            "// Output color = color of the texture at the specified UV\n"
            "float newAlpha;\n"
            "float k, c, a;\n"
            "a = 1.0f / pow(aff, affExponent);\n"
            "//The two functions meet at (2/3, 2/3)\n"
            "/*if (isMaxAlpha==0)\n"
            "{\n"
                "k = 4.0f/3.0f*(1.0f+exp(2.0f*a/3.0f))/(-1.0f+exp(2.0f*a/3.0f));\n"
                "c = -4.0f/3.0f*1.0f/(-1.0f+exp(2.0f*a/3.0f));\n"
            "}\n"
            "else"
            "{\n"
                "k = 2.0f/3.0f*(1.0f+exp(-a/3.0f))/(1.0f-exp(-a/3.0f));\n"
                "c = 2.0f/3.0f + -k/2.0f;\n"
            "}\n"
            "newAlpha = max( 0.0f, min( 1.0f, k/(1.0f+exp(-a*(alpha-2.0f/3.0f))) + c ) );*/\n"
            "//The two functions meet at (1/2, 1/2)\n"
            "if (isMaxAlpha==0)\n"
            "{\n"
                "k = (1.0f+exp(a/2.0f))/(-1.0f+exp(a/2.0f));\n"
                "c = -1.0f/(-1.0f+exp(a/2.0f));\n"
            "}\n"
            "else"
            "{\n"
                "k = (1.0f+exp(-a/2.0f))/(-1.0f+exp(-a/2.0f));\n"
                "c = (-2.0f+exp(-a/2.0f))/(-1.0f+exp(-a/2.0f));\n"
            "}\n"
            "newAlpha = max( 0.0f, min( 1.0f, k/(1.0f+exp(-a*(alpha-2.0f/3.0f))) + c ) );\n"
            "//if (aff>0.1f)\n"
                "//newAlpha = pow( alpha, pow(1.0f/aff, affExponent) );\n"
            "//else\n"
                "//newAlpha = pow( alpha, pow(10.0f, affExponent) );\n"
            "if (any(lessThan(vBC, vec3(0.05))))\n"
                "color = vec4(alpha, 0.0, 0.0, 1.0);\n"
            "else if (blend==1)\n"
                "//color = vec4(0.0, pow( 0.5, pow(1.0f/aff, affExponent) ), 0.0, 1.0);\n"
                "color = vec4(1-( newAlpha*(1-texture( srcTexture, UV).rgb) + (1-texture( fboTexture, UV_FBO).rgb) ), 1);\n"
            "else\n"
                "color = vec4( 1-( histEqMult*(1-texture( srcTexture, UV ).rgb) ), alpha );\n"
                "//color = vec4( texture( srcTexture, UV ).rgb, alpha );\n"
                "//color = vec4( newAlpha, newAlpha, newAlpha, 1.0 );\n"
        "}\n";
//! [3]

//! [4]
QOpenGLShader *WarpWindow::loadShader(QOpenGLShader::ShaderType type, const char *source)
{
    QOpenGLShader *shader = new QOpenGLShader(type);
    shader->compileSourceCode(source);

    return shader;
}

void WarpWindow::initialize()
{
    m_program = new QOpenGLShaderProgram(this);
    m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
    m_program->link();
    qDebug()<<m_program->log();
    m_matrixUniform = m_program->uniformLocation("MVP");
    m_textureUniform = m_program->uniformLocation("srcTexture");
    m_textureFBOUniform = m_program->uniformLocation("fboTexture");
    m_alphaUniform = m_program->uniformLocation("alpha");
    m_baryAttrib = m_program->attributeLocation("barycentric");
    m_blendUniform = m_program->uniformLocation("blend");
    m_showWireframeUniform = m_program->uniformLocation("showWireframe");
    m_affExponentUniform = m_program->uniformLocation("affExponent");
    m_affinityAttrib = m_program->attributeLocation("affinity");
    m_histEqMultUniform = m_program->uniformLocation("histEqMult");
    m_isMaxAlphaUniform = m_program->uniformLocation("isMaxAlpha");

    showWireframe = false;

    // Load, generate, and bind textures
    QImage texImg;
    for (unsigned int iter=0; iter<i_numImage; ++iter)
    {
        QString t(i_textureFile[iter].c_str());

        if (!texImg.load(t, "PNG"))
        {
            qDebug()<<t<<"not found. Quitting!";
            exit(0);
        }

        uchar *data = texImg.bits();
        float sum = 0;
        int numChannel = texImg.depth()/8;
        for(int i=0; i<texImg.width()*texImg.height(); ++i)
            sum+=float(255.0f-data[numChannel*i])/255.0f;
        pixelSum[iter] = sum/float(texImg.height()*texImg.width());
        qDebug()<<pixelSum[iter];
        texImg = QGLWidget::convertToGLFormat(texImg);
        glGenTextures(1, &i_texture[iter]);
        glBindTexture(GL_TEXTURE_2D, i_texture[iter]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texImg.width(),
                     texImg.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE,
                     (GLvoid *)texImg.bits());
//        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    }
    for (unsigned int iter=0; iter<i_numImage; ++iter)
    {
        for (unsigned int i=0; i<i_numThinningLevels; ++i)
        {
            QString t(i_thinTextureFile[iter][i].c_str());
            if (!texImg.load(t, "PNG"))
            {
                qDebug()<<t<<"not found. Quitting!";
                exit(0);
            }
            texImg = QGLWidget::convertToGLFormat(texImg);
            glGenTextures(1, &i_thinTexture[iter][i]);
            glBindTexture(GL_TEXTURE_2D, i_thinTexture[iter][i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texImg.width(),
                         texImg.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE,
                         (GLvoid *)texImg.bits());
            glGenerateMipmap(GL_TEXTURE_2D);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        }
    }

    i_height = texImg.height();
    i_width = texImg.width();

    // White background
    glClearColor(1.0f, 1.0f, 1.0f, 0.0f);

    // Disable depth test
    glDisable(GL_DEPTH_TEST);
//    glEnable(GL_BLEND);


    //Generate buffers
    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);

    glGenBuffers(1, &uvBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);

    glGenBuffers(1, &baryBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, baryBuffer);

    glGenBuffers(1, &affinityBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, affinityBuffer);

    curAlpha = 0.0;
    m_numAnimationMoves = 0;
    direction = 1;
    blendMode = 2;
    playAnimation = true;
    screenshotTrigger = false;
    externalAlpha = true;
    m_maxAnimationSteps = 100;
    affExponent = 5.0f;
    showBlendAlphasTrigger = false;

    initializeMeshData();

    //initialize blending choices
    for (unsigned int iter=0; iter<10; ++iter)
    {
        if (iter<i_numImage)
            blendImage[iter] = true;
        else
            blendImage[iter] = false;
    }
    m_time.start();

    glGenFramebuffers(1, &m_FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
    glGenTextures(1, &m_textureFBO_0);
    glBindTexture(GL_TEXTURE_2D, m_textureFBO_0);
    GLfloat *white = new GLfloat[i_width*i_height*4];
    for (unsigned int i=0; i<i_width*i_height; ++i)
    {
        //RGBA = (1.0, 1.0, 1.0, 1.0)
        white[4*i] = 1.0f; white[4*i+1] = 1.0f; white[4*i+2] = 1.0f; white[4*i+3] = 1.0f;
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, i_width, i_height, 0, GL_RGBA, GL_FLOAT, (GLvoid *)white);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_textureFBO_0, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glGenTextures(1, &m_textureFBO_1);
    glBindTexture(GL_TEXTURE_2D, m_textureFBO_1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, i_width, i_height, 0, GL_RGBA, GL_FLOAT, (GLvoid *)white);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_textureFBO_1, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenTextures(1, &m_textureFBO_2);
    glBindTexture(GL_TEXTURE_2D, m_textureFBO_2);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, i_width, i_height, 0, GL_RGBA, GL_FLOAT, (GLvoid *)white);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, m_textureFBO_2, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenTextures(1, &m_textureFBO_3);
    glBindTexture(GL_TEXTURE_2D, m_textureFBO_3);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, i_width, i_height, 0, GL_RGBA, GL_FLOAT, (GLvoid *)white);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, m_textureFBO_3, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    delete[] white;

    glGenTextures(1, &texture_temp);
    glBindTexture(GL_TEXTURE_2D, texture_temp);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        qDebug()<<"Framebuffer initialization error!";
    }

    qDebug()<<"Initialization complete!";
    qDebug()<<"Image dimentions and window dimensions (Wi, Hi, Ww, Hw): "<<i_width<<i_height<<width()<<height();

    ctrlWindow->init(i_numImage, warpType, warpPositions, i_textureFile);

//    ctrlWindow->setPosition(warpAlpha);
}
//! [4]

//! [5]
void WarpWindow::render()
{
    const qreal retinaScale = devicePixelRatio();
//    glViewport(0, 0, width() * retinaScale, height() * retinaScale);

    //Computer the projective transformation matrix
    QMatrix4x4 view;
    view.lookAt(
                QVector3D(0, 0, 2.0),   //camera position
                QVector3D(0, 0, 0.0),     //lookAt direction
                QVector3D(0, 1.0, 0)    //UP vector
                );

    QMatrix4x4 projection;
    projection.ortho(-2.0f, 2.0f, -1.0f, 1.0f, 0.1f, 5.0f);

    QMatrix4x4 projectionFBO;
    projectionFBO.ortho(0.0f, 2.0f, -1.0f, 1.0f, 0.1f, 5.0f);

    QMatrix4x4 model;

    //Animation timing
    if (m_time.elapsed() >= 15 && ctrlWindow->getAnimationState())
    {
        //animation is handled by control window. Main window handles timing, and asks for updated warpAlpha when needed

        //Move this code to control window's getWarpAlpha function
        if (direction)
        {
            m_numAnimationMoves++;
            if (m_numAnimationMoves>=m_maxAnimationSteps)
            {
                m_numAnimationMoves = 0;
                direction = 0;
//                for (unsigned int iter=0; iter<i_numImage; ++iter)
//                    warpAlpha[iter] = warpAlphaEnd[iter];
            }
//            else
//            {
//                for (unsigned int iter=0; iter<i_numImage; ++iter)
//                    warpAlpha[iter] = warpAlphaBegin[iter] + m_numAnimationMoves*warpAlphaMove[iter];
//            }
        }
        else
        {
            m_numAnimationMoves++;
            if (m_numAnimationMoves>=m_maxAnimationSteps)
            {
                m_numAnimationMoves = 0;
                direction = 1;
//                for (unsigned int iter=0; iter<i_numImage; ++iter)
//                    warpAlpha[iter] = warpAlphaBegin[iter];
            }
//            else
//            {
//                for (unsigned int iter=0; iter<i_numImage; ++iter)
//                    warpAlpha[iter] = warpAlphaEnd[iter] - m_numAnimationMoves*warpAlphaMove[iter];
//            }
        }

        ctrlWindow->setAlphaAnim(direction, m_numAnimationMoves, m_maxAnimationSteps);

        for (unsigned int iter=0; iter<i_numImage; ++iter)
            blendAlpha[iter] = min<float>(1.0f, max<float>(0.0f, warpAlpha[iter]));     //Since warpAlpha can exceed the [0, 1] while doing extrapolating warps

        m_time.start();
//        ctrlWindow->setPosition(warpAlpha);
    }

    //Warp the meshes before sending them to the graphics pipeline
    warpMeshes();


    //normalize blending alpha values

    for (unsigned int iter=0; iter<i_numImage; ++iter)
        blendAlpha[iter] = min<float>(1.0f, max<float>(0.0f, warpAlpha[iter]));

    GLfloat sumAlpha = 0.0f;
    GLuint numActiveImage = 0;
    GLfloat maxAlpha = 0.0f;
    GLuint maxAlphaIdx = 0;
    for (unsigned int iter=0; iter<i_numImage; ++iter)
        if (blendImage[iter])
        {
            sumAlpha += blendAlpha[iter];
            numActiveImage++;
        }

    if (sumAlpha>=EPSILON)
    {
        for (unsigned int iter=0; iter<i_numImage; ++iter)
            if (blendImage[iter])
                blendAlpha[iter] /= sumAlpha;
    }
    else if (numActiveImage>0)
    {
        for (unsigned int iter=0; iter<i_numImage; ++iter)
            if (warpAlpha[iter] < -EPSILON)
                blendAlpha[iter] = 1.0;
    }

    for(unsigned int iter = 0; iter<i_numImage; ++iter)
        if (blendImage[iter] && blendAlpha[iter]>maxAlpha)
        {
            maxAlpha = blendAlpha[iter];
            maxAlphaIdx = iter;
        }

    if (showBlendAlphasTrigger)
    {
        showblendAlphas();
        showBlendAlphasTrigger = false;
    }
    //Use m_FBO as the current framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);

    mvp = projectionFBO*view*model;

    m_program->bind();

    glUniform1f(m_affExponentUniform, affExponent);
    glUniformMatrix4fv(m_matrixUniform, 1, GL_FALSE, mvp.constData());
    glUniform1i(m_blendUniform, 1);
    glUniform1f(m_histEqMultUniform, 1.0f);
    glUniform1i(m_isMaxAlphaUniform, 0);

    //Send barycentric coordinates to shader
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, baryBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*9*i_numTriangleMax,
            barycentricCoors, GL_STATIC_DRAW);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);


    //Copy data to vertex buffer objects, and draw the warped meshes

    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glDrawBuffer(GL_COLOR_ATTACHMENT1);
    glViewport(0, 0, i_width * retinaScale, i_height * retinaScale);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

//    unsigned char *image = new unsigned char[i_width*i_height*4];
//    glReadPixels(0, 0, i_width, i_height, GL_RGB, GL_UNSIGNED_BYTE, image);
//    QImage imgQ(image, i_width, i_height, QImage::Format_RGB888);
//    imgQ.save("image.bmp", "BMP");

    for (unsigned int iter=0; iter<i_numImage; ++iter)
    {
        //do nothing if image is not to be blended in
        if (!blendImage[iter])
            continue;


        // Bind our texture in Texture Unit 1+iter

        if (!thinning || !thinningState)
        {
            glActiveTexture(GL_TEXTURE1+iter);
            glBindTexture(GL_TEXTURE_2D, i_texture[iter]);
            glUniform1i(m_textureUniform, 1+iter);
        }
        else
        {
            //blendAlpha=1.0 => minimum visibility => use thinning level min (= 0)
            //blendAlpha=0.0 => maximum visibility => use thinning level max (= i_numThinningLevels-1)
//            unsigned int thinningLevel = floor( (1.0f-blendAlpha[iter]) / (1.0f/i_numThinningLevels) );
            unsigned int thinningLevel0, thinningLevel1;
            float thinBlend0, thinBlend1;
            float intervalSize = 1.0/(2*i_numThinningLevels);

//            qDebug()<<iter<<' '<<thinningLevel;

//            if (thinningLevel >= i_numThinningLevels)
//                thinningLevel = i_numThinningLevels-1;

            //find thinning levels 0 and 1
            thinningLevel0 = max<unsigned int>(0, (floor( (1.0f-blendAlpha[iter]) / intervalSize )-1)/2);
            thinningLevel1 = min<unsigned int>(i_numThinningLevels-1, (floor( (1.0f-blendAlpha[iter]) / intervalSize )+1)/2);

            //find blending alphas 0 and 1
            if (thinningLevel0 == thinningLevel1)
            {
                thinBlend0 = 0.0;
                thinBlend1 = 1.0;
            }
            else
            {
                thinBlend0 = 1.0f-fabs( (1.0f-blendAlpha[iter]) - (2*thinningLevel0+1)*intervalSize ) / (2*intervalSize);
                thinBlend1 = 1.0f-fabs( (1.0f-blendAlpha[iter]) - (2*thinningLevel1+1)*intervalSize ) / (2*intervalSize);
            }

            glEnableVertexAttribArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
            glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*12,
                    vertexDataFBO, GL_STATIC_DRAW);
            glVertexAttribPointer(
                0,                  // attribute. No particular reason for 0, but must match the layout in the shader.
                3,                  // size
                GL_FLOAT,           // type
                GL_FALSE,           // normalize?
                0,                  // stride
                (void*)0            // array buffer offset
                );

            // Load src texture coordinates into uv buffer
            glEnableVertexAttribArray(1);
            glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
            glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*8,
                    uvDataFBO, GL_STATIC_DRAW);
            glVertexAttribPointer(
                1,                   // attribute. No particular reason for 1, but must match the layout in the shader.
                2,                   // size : U+V => 2
                GL_FLOAT,            // type
                GL_FALSE,            // normalize?
                0,                   // stride
                (void*)0             // array buffer offset
                );

            glEnableVertexAttribArray(3);
            glBindBuffer(GL_ARRAY_BUFFER, affinityBuffer);
            glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*4,
                    affDataFBO, GL_STATIC_DRAW);
            glVertexAttribPointer(
                3,                  // attribute. No particular reason for 3, but must match the layout in the shader.
                1,                  // size
                GL_FLOAT,           // type
                GL_FALSE,           // normalize?
                0,                  // stride
                (void*)0            // array buffer offset
                );

            glUniform1f(m_alphaUniform, 1.0);
            glUniform1i(m_blendUniform, 0);

            glEnableVertexAttribArray(2);
            glBindBuffer(GL_ARRAY_BUFFER, baryBuffer);
            glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*12,
                    barycentricCoorsFBO, GL_STATIC_DRAW);
            glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

            //prepare color attachments 2 and 3 (by clearing them to white)
            glReadBuffer(GL_COLOR_ATTACHMENT2);
            glDrawBuffer(GL_COLOR_ATTACHMENT3);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glReadBuffer(GL_COLOR_ATTACHMENT3);
            glDrawBuffer(GL_COLOR_ATTACHMENT2);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            //draw the first image
            glActiveTexture(GL_TEXTURE1+i_numImage);
            glBindTexture(GL_TEXTURE_2D, m_textureFBO_3);
            glUniform1i(m_textureFBOUniform, 1+i_numImage);
            glActiveTexture(GL_TEXTURE1+iter);
            glBindTexture(GL_TEXTURE_2D, i_thinTexture[iter][thinningLevel0]);
            glUniform1i(m_textureUniform, 1+iter);
            glUniform1f(m_alphaUniform, thinBlend0);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            //swap buffers before drawing the second image
            glReadBuffer(GL_COLOR_ATTACHMENT2);
            glDrawBuffer(GL_COLOR_ATTACHMENT3);

            //read the first image into fboTexture sampler
            glActiveTexture(GL_TEXTURE1+i_numImage);
            glBindTexture(GL_TEXTURE_2D, m_textureFBO_2);
            glUniform1i(m_textureFBOUniform, 1+i_numImage);

            //draw the second image while blending with the first image stored as fboTexture
            glActiveTexture(GL_TEXTURE1+iter);
            glBindTexture(GL_TEXTURE_2D, i_thinTexture[iter][thinningLevel1]);
            glUniform1i(m_textureUniform, 1+iter);
            glUniform1f(m_alphaUniform, thinBlend1);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            //Switch to attachment 3 as read buffer and read its contents into the srcTexture sampler
            glReadBuffer(GL_COLOR_ATTACHMENT3);
            glActiveTexture(GL_TEXTURE1+iter);
            glBindTexture(GL_TEXTURE_2D, m_textureFBO_3);
            glUniform1i(m_textureUniform, 1+iter);

            //Switch to attachments 0 and 1 to continue iterative rendering
            glReadBuffer(GL_COLOR_ATTACHMENT0);
            glDrawBuffer(GL_COLOR_ATTACHMENT1);
        }

        //Load the read buffer (color attachment 0) into texture unit 0, and set "fboTexture" sampler to use it
        glActiveTexture(GL_TEXTURE0);   //set active texture unit (for manipulation) as texture unit 0
        glBindTexture(GL_TEXTURE_2D, m_textureFBO_0);   //copy the contents of color attachment 0 to texture unit 0
        glUniform1i(m_textureFBOUniform, 0);    //set the fboTexture sampler to use texture unit 0

        // Load texture coordinates into uv buffer (buffer 1)
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*6*i_numTriangle[iter],
                uvBufferData[iter], GL_STATIC_DRAW);
        glVertexAttribPointer(
            1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
            2,                                // size : U+V => 2
            GL_FLOAT,                         // type
            GL_FALSE,                         // normalize?
            0,                                // stride
            (void*)0                          // array buffer offset
            );

        //Load vertex coordinates into vertex buffer (buffer 0)
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*9*i_numTriangle[iter],
                warpVertexBufferData[iter], GL_STATIC_DRAW);
        glVertexAttribPointer(
            0,                  // attribute. No particular reason for 0, but must match the layout in the shader.
            3,                  // size
            GL_FLOAT,           // type
            GL_FALSE,           // normalize?
            0,                  // stride
            (void*)0            // array buffer offset
            );

        //Load vertex affinites into buffer 3
        glEnableVertexAttribArray(3);
        glBindBuffer(GL_ARRAY_BUFFER, affinityBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*3*i_numTriangle[iter],
                warpAffBufferData[iter], GL_STATIC_DRAW);
        glVertexAttribPointer(
            3,                  // attribute. No particular reason for 3, but must match the layout in the shader.
            1,                  // size
            GL_FLOAT,           // type
            GL_FALSE,           // normalize?
            0,                  // stride
            (void*)0            // array buffer offset
            );

//        QDebug deb = qDebug();
//        for (unsigned int t=0; t<3*i_numTriangle[iter]; ++t)
//            deb<<warpAffBufferData[iter][t];

        // Set blending alpha (if thinning is off) and turn on blending
        if (!thinning || !thinningState)
            glUniform1f(m_alphaUniform, blendAlpha[iter]);
        else
            glUniform1f(m_alphaUniform, 1.0);
        glUniform1i(m_blendUniform, 1);
        glUniform1f(m_histEqMultUniform, 1.0);

        if(maxAlphaIdx == iter)
            glUniform1i(m_isMaxAlphaUniform, 1);
        else
            glUniform1i(m_isMaxAlphaUniform, 0);

        // Draw the warped mesh!
        glDrawArrays(GL_TRIANGLES, 0, i_numTriangle[iter]*3);

        //Swap read and draw buffers before copying pixel color data
        glReadBuffer(GL_COLOR_ATTACHMENT1);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);

        // Set alpha to 1.0 (100% opaque) and turn blending off
        glUniform1f(m_alphaUniform, 1.0f);
        glUniform1i(m_blendUniform, 0);

        // Load FBO color_attachment_1 texture into texture unit 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_textureFBO_1);
    //     Set our "srcTexture" sampler to user Texture Unit 0
        glUniform1i(m_textureUniform, 0);

        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*12,
                vertexDataFBO, GL_STATIC_DRAW);
        glVertexAttribPointer(
            0,                  // attribute. No particular reason for 0, but must match the layout in the shader.
            3,                  // size
            GL_FLOAT,           // type
            GL_FALSE,           // normalize?
            0,                  // stride
            (void*)0            // array buffer offset
            );

        // Load src texture coordinates into uv buffer
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*8,
                uvDataFBO, GL_STATIC_DRAW);
        glVertexAttribPointer(
            1,                   // attribute. No particular reason for 1, but must match the layout in the shader.
            2,                   // size : U+V => 2
            GL_FLOAT,            // type
            GL_FALSE,            // normalize?
            0,                   // stride
            (void*)0             // array buffer offset
            );

        glUniform1f(m_alphaUniform, 1.0);

        glEnableVertexAttribArray(2);
        glBindBuffer(GL_ARRAY_BUFFER, baryBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*12,
                barycentricCoorsFBO, GL_STATIC_DRAW);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

        //Load vertex affinites into buffer 3
        glEnableVertexAttribArray(3);
        glBindBuffer(GL_ARRAY_BUFFER, affinityBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*4,
                affDataFBO, GL_STATIC_DRAW);
        glVertexAttribPointer(
            3,                  // attribute. No particular reason for 3, but must match the layout in the shader.
            1,                  // size
            GL_FLOAT,           // type
            GL_FALSE,           // normalize?
            0,                  // stride
            (void*)0            // array buffer offset
            );

        glUniform1f(m_histEqMultUniform, 1.0f);

    //     Draw the rectangle to copy color attachment 1 data to color attachment 0 which is used in the next iteration!
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

//        glReadPixels(0, 0, i_width, i_height, GL_RGB, GL_UNSIGNED_BYTE, image);
//        QImage imgQ(image, i_width, i_height, QImage::Format_RGB888);
//        if (iter==0)
//            imgQ.save("image0.bmp", "BMP");
//        else
//            imgQ.save("image1.bmp", "BMP");

        //Swap again to continue iterative rendering
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glDrawBuffer(GL_COLOR_ATTACHMENT1);
    }

//    delete[] image;

    //update histEqMult here so that the screenshot is also contrast equalized
    computeHistEqMultiplier();

    if (screenshotTrigger)
    {
        saveCurrentRender(0, 0, i_width, i_height);
        screenshotTrigger = false;
    }

    //Draw to the default framebuffer (program window)
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glDrawBuffer(GL_FRONT_AND_BACK);

    //re-set viewport to display unwarped as well as warped meshes
    glViewport(0, 0, width() * retinaScale, height() * retinaScale);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //Set uniform variable blend to zero (see fragment shader for detail)
    glUniform1i(m_blendUniform, 0.0f);

    mvp = projection*view*model;

    glUniformMatrix4fv(m_matrixUniform, 1, GL_FALSE, mvp.constData());

    // Bind our texture in Texture Unit activeOrigImage
    glActiveTexture(GL_TEXTURE1+activeOrigImage);
    glBindTexture(GL_TEXTURE_2D, i_texture[activeOrigImage]);
    // Set our "srcTexture" sampler to user Texture Unit 0
    glUniform1i(m_textureUniform, 1+activeOrigImage);

    // Load unwarped mesh into vertex buffer
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*9*i_numTriangle[activeOrigImage],
            origVertexBufferData[activeOrigImage], GL_STATIC_DRAW);
    glVertexAttribPointer(
        0,                  // attribute. No particular reason for 0, but must match the layout in the shader.
        3,                  // size
        GL_FLOAT,           // type
        GL_FALSE,           // normalize?
        0,                  // stride
        (void*)0            // array buffer offset
        );

    // Load src texture coordinates into uv buffer
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*6*i_numTriangle[activeOrigImage],
            uvBufferData[activeOrigImage], GL_STATIC_DRAW);
    glVertexAttribPointer(
        1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
        2,                                // size : U+V => 2
        GL_FLOAT,                         // type
        GL_FALSE,                         // normalize?
        0,                                // stride
        (void*)0                          // array buffer offset
        );

    //Send barycentric coordinates to shader
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, baryBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*9*i_numTriangleMax,
            barycentricCoors, GL_STATIC_DRAW);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    //Load vertex affinites into buffer 3
    glEnableVertexAttribArray(3);
    glBindBuffer(GL_ARRAY_BUFFER, affinityBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*3*i_numTriangle[activeOrigImage],
            origAffBufferData[activeOrigImage], GL_STATIC_DRAW);
    glVertexAttribPointer(
        3,                  // attribute. No particular reason for 3, but must match the layout in the shader.
        1,                  // size
        GL_FLOAT,           // type
        GL_FALSE,           // normalize?
        0,                  // stride
        (void*)0            // array buffer offset
        );

    // Set curAlpha to zero for drawing the input image as always 100% opaque
    glUniform1f(m_alphaUniform, 1.0);
    glUniform1i(m_blendUniform, 0);
    glUniform1f(m_histEqMultUniform, 1.0);

    // Draw the unwarped mesh!
    glDrawArrays(GL_TRIANGLES, 0, i_numTriangle[activeOrigImage]*3);


    // Load FBO color_attachment_0 texture into texture unit 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_textureFBO_0);
//     Set our "srcTexture" sampler to user Texture Unit 0
    glUniform1i(m_textureUniform, 0);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*12,
            vertexDataFBO, GL_STATIC_DRAW);
    glVertexAttribPointer(
        0,                  // attribute. No particular reason for 0, but must match the layout in the shader.
        3,                  // size
        GL_FLOAT,           // type
        GL_FALSE,           // normalize?
        0,                  // stride
        (void*)0            // array buffer offset
        );

    // Load src texture coordinates into uv buffer
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*8,
            uvDataFBO, GL_STATIC_DRAW);
    glVertexAttribPointer(
        1,                   // attribute. No particular reason for 1, but must match the layout in the shader.
        2,                   // size : U+V => 2
        GL_FLOAT,            // type
        GL_FALSE,            // normalize?
        0,                   // stride
        (void*)0             // array buffer offset
        );

    glUniform1f(m_alphaUniform, 1.0);

    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, baryBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*12,
            barycentricCoorsFBO, GL_STATIC_DRAW);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    //Load vertex affinites into buffer 3
    glEnableVertexAttribArray(3);
    glBindBuffer(GL_ARRAY_BUFFER, affinityBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*4,
            affDataFBO, GL_STATIC_DRAW);
    glVertexAttribPointer(
        3,                  // attribute. No particular reason for 3, but must match the layout in the shader.
        1,                  // size
        GL_FLOAT,           // type
        GL_FALSE,           // normalize?
        0,                  // stride
        (void*)0            // array buffer offset
        );

    // TODO: Compute contrast equalization multiplier and send it to fragment shader
    glUniform1f(m_histEqMultUniform, histEqMult);

//     Draw the rectangle to copy framebuffer (m_FBO) data to screen (default framebuffer)!
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // Clean up
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    glDisableVertexAttribArray(3);

    //Clear the read buffer m_FBO->color_attachment_0
    glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ++m_frame;
}
//! [5]

//! [6]

void WarpWindow::loadInputData(std::string inputFile)
{
    qDebug()<<"Loading input data";
//    QString path = QCoreApplication::applicationDirPath();
//    path.append("/");
    QString path = "";
    inputFile.insert(0, path.toStdString());
    std::string vertFile[10][10], triFile[10], affFile[10][10];
    std::string imagePositionFile;
    unsigned int numVertex, h, w;
    fstream curFile;

    curFile.open(inputFile.c_str());
    if (!curFile)
    {
        qDebug()<<inputFile.c_str()<<"not found. Quitting!";
        exit(0);
    }
    curFile>>i_numImage;
    //curFile.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    std::getline(curFile, triFile[0]);

    qDebug()<<"Reading files:";
    for (unsigned int i=0; i<i_numImage; ++i)
    {
        //Read the ith triangulation file's name
        std::getline(curFile, triFile[i]);
        triFile[i].insert(0, path.toStdString());

        //Read the filenames containing vertex lists, including the identity warp
        for (unsigned int j=0; j<i_numImage; ++j)
        {
            std::getline(curFile, vertFile[i][j]);
            vertFile[i][j].insert(0, path.toStdString());
        }

        //Read the ith image's filename
        std::getline(curFile, i_textureFile[i]);
        i_textureFile[i].insert(0, path.toStdString());
        qDebug()<<i_textureFile[i].c_str();
    }

    for (unsigned int i=0; i<i_numImage; ++i)
    {
        curFile>>warpAlphaBegin[i]>>warpAlphaEnd[i];
    }

    curFile>>warpType;

    curFile.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    std::getline(curFile, imagePositionFile);
    imagePositionFile.insert(0, path.toStdString());
    warpPositions = NULL;
    qDebug()<<"Positions file: "<<imagePositionFile.c_str();

    curFile>>i_numThinningLevels;
    curFile.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    qDebug()<<"Number of thinning levels: "<<i_numThinningLevels;
    if (i_numThinningLevels)
    {
        thinning = true;
        thinningState = false;
        for (unsigned int iter=0; iter<i_numImage; ++iter)
        {
            for (unsigned int i=0; i<i_numThinningLevels; ++i)
            {
                std::getline(curFile, i_thinTextureFile[iter][i]);
                qDebug()<<i_thinTextureFile[iter][i].c_str();
            }
        }
    }
    else
        thinning = false;

    if(!curFile.eof())
    {
        curFile>>affinitiesGiven;
        curFile.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        if (affinitiesGiven)
        {
            for (unsigned int i=0; i<i_numImage; ++i)
            {
                for (unsigned int j=0; j<i_numImage; ++j)
                {
                    if (i==j)
                        continue;
                    std::getline(curFile, affFile[i][j]);
                }
            }
        }
    }
    else
        affinitiesGiven = 0;

    curFile.close();

    //Now read the actual files
    for (unsigned int i=0; i<i_numImage; ++i)
    {
        //Read the ith triangulation
        curFile.open(triFile[i].c_str());
        if (!curFile)
        {
            qDebug()<<triFile[i].c_str()<<"not found. Quitting!";
            exit(0);
        }
        curFile>>i_numTriangle[i];
        i_tri[i] = new GLuint[3*i_numTriangle[i]];
        for (unsigned int iter=0; iter<i_numTriangle[i]; ++iter)
        {
            curFile>>i_tri[i][3*iter]>>i_tri[i][3*iter+1]
                    >>i_tri[i][3*iter+2];

            //decrement 1 to change from Matlab's 1-indexing to 0-indexing
            i_tri[i][3*iter]--;
            i_tri[i][3*iter+1]--;
            i_tri[i][3*iter+2]--;

        }
        curFile.close();

        for (unsigned int j=0; j<i_numImage; ++j)
        {
            //Read the (i,j)th vertex list
            curFile.open(vertFile[i][j].c_str());
            if (!curFile)
            {
                qDebug()<<vertFile[i][j].c_str()<<"not found. Quitting!";
                exit(0);
            }
            curFile>>numVertex>>h>>w;
            i_vert[i][j] = new GLfloat[2*numVertex];
            for (unsigned int iter=0; iter<numVertex; ++iter)
            {
                curFile>>i_vert[i][j][2*iter]>>i_vert[i][j][2*iter+1];
                //Normalize vertex locations to [-1, 1] range
                i_vert[i][j][2*iter] = i_vert[i][j][2*iter]/(w/2.0) - 1.0;
                i_vert[i][j][2*iter+1] = i_vert[i][j][2*iter+1]/(h/2.0) - 1.0;
            }
            curFile.close();

            i_affinity[i][j] = new GLfloat[numVertex];
            if (affinitiesGiven && i!=j)
            {
                curFile.open(affFile[i][j].c_str());
                if (!curFile)
                {
                    qDebug()<<affFile[i][j].c_str()<<"not found. Quitting!";
                    exit(0);
                }
//                qDebug()<<"Affinity file:"<<affFile[i][j].c_str();
                for (unsigned int iter=0; iter<numVertex; ++iter)
                    curFile>>i_affinity[i][j][iter];
                curFile.close();
            }
            else
                std::fill_n(i_affinity[i][j], numVertex, 1.0f);
        }
    }

    if (warpType)
    {
        warpPositions = new QPointF* [i_numImage];
        float x, y;
        curFile.open(imagePositionFile.c_str());
        if (!curFile)
        {
            qDebug()<<imagePositionFile.c_str()<<"not found. Quitting!";
            exit(0);
        }
        for (unsigned int iter=0; iter<i_numImage; ++iter)
        {
            curFile>>x>>y;
            warpPositions[iter] = new QPointF;
            warpPositions[iter]->setX(x);
            warpPositions[iter]->setY(y);
        }
    }

    i_width = w;
    i_height = h;
    qDebug()<<"Number of triangles in the first two images: "<<i_numTriangle[0]<<i_numTriangle[1];
}
//! [6]

//! [7]

void WarpWindow::initializeMeshData()
{
    //Initialize vertex buffers
    for (unsigned int iter=0; iter<i_numImage; ++iter)
    {
        for (unsigned int iter2=0; iter2<i_numImage; ++iter2)
        {
            origVertexBufferData[iter] = new GLfloat[9*i_numTriangle[iter]];
            warpVertexBufferData[iter] = new GLfloat[9*i_numTriangle[iter]];
            uvBufferData[iter] = new GLfloat[6*i_numTriangle[iter]];
            origAffBufferData[iter] = new GLfloat[3*i_numTriangle[iter]];
            warpAffBufferData[iter] = new GLfloat[3*i_numTriangle[iter]];
        }
    }

    i_numTriangleMax = 0;
    for (unsigned int iter=0; iter<i_numImage; ++iter)
        if (i_numTriangle[iter]>i_numTriangleMax)
            i_numTriangleMax = i_numTriangle[iter];

    barycentricCoors = new GLfloat[9*i_numTriangleMax];

    for (unsigned int iter=0; iter<i_numImage; ++iter)
    {
        for (unsigned int i=0; i<i_numTriangle[iter]; ++i)
        {
            for (unsigned int j=0; j<3; ++j)
            {
                int idx = 9*i + 3*j;
//                if (iter==0)
//                {
                    origVertexBufferData[iter][idx] = i_vert[iter][iter][2 * i_tri[iter][3*i+j]] - 1.0;
                    origVertexBufferData[iter][idx+1] = -i_vert[iter][iter][2 * i_tri[iter][3*i+j]+1];
                    origVertexBufferData[iter][idx+2] = 0.0f;
//                }
//                else
//                {
//                    origVertexBufferData[iter][idx] = i_vert[iter][iter][2 * i_tri[iter][3*i+j]] - 3.0;
//                    origVertexBufferData[iter][idx+1] = -i_vert[iter][iter][2 * i_tri[iter][3*i+j]+1];
//                    origVertexBufferData[iter][idx+2] = 0.1f;
//                }
//                qDebug()<<vertexBufferData[iter][iter][idx]<<' '<<vertexBufferData[iter][iter][idx+1]<<' '<<vertexBufferData[iter][iter][idx+2];
            }

            //Assign barycentric coordinates (1, 0, 0), (0, 1, 0) and (0, 0, 1) to the vertices of the triangle
            //The fragment shader interpolates these values for other fragments, which is used to decide whether a
            //fragment lies on the edge of a triangle or not. This property is used for wireframe rendering
            barycentricCoors[9*i+0] = 1;
            barycentricCoors[9*i+1] = 0;
            barycentricCoors[9*i+2] = 0;
            barycentricCoors[9*i+3] = 0;
            barycentricCoors[9*i+4] = 1;
            barycentricCoors[9*i+5] = 0;
            barycentricCoors[9*i+6] = 0;
            barycentricCoors[9*i+7] = 0;
            barycentricCoors[9*i+8] = 1;
        }

        for (unsigned int i=0; i<i_numTriangle[iter]; ++i)
        {
            for (unsigned int j=0; j<3; ++j)
            {
                int idx = 6*i + 2*j;

                uvBufferData[iter][idx] = i_vert[iter][iter][2*i_tri[iter][3*i+j]]/2.0 + 0.5;
                uvBufferData[iter][idx+1] = 1.0 - (i_vert[iter][iter][2*i_tri[iter][3*i+j]+1]/2.0 + 0.5);

                if(uvBufferData[iter][idx]<0.0)
                    uvBufferData[iter][idx] = 0.0;
                if(uvBufferData[iter][idx]>1.0)
                    uvBufferData[iter][idx] = 1.0;
                if(uvBufferData[iter][idx+1]<0.0)
                    uvBufferData[iter][idx+1] = 0.0;
                if(uvBufferData[iter][idx+1]>1.0)
                    uvBufferData[iter][idx+1] = 1.0;
//                qDebug()<<uvBufferData[iter][idx]<<uvBufferData[iter][idx+1];
            }
        }

        for (unsigned int i=0; i<i_numTriangle[iter]; ++i)
        {
            for (unsigned int j=0; j<3; ++j)
            {
                int idx = 3*i+j;
                origAffBufferData[iter][idx] = 1.0f;
            }
        }
    }

    vertexDataFBO[0] = 0.0f; vertexDataFBO[1] = -1.0f; vertexDataFBO[2] = 0.0f;
    vertexDataFBO[3] = 2.0f; vertexDataFBO[4] = -1.0f; vertexDataFBO[5] = 0.0f;
    vertexDataFBO[6] = 0.0f; vertexDataFBO[7] = 1.0f; vertexDataFBO[8] = 0.0f;
    vertexDataFBO[9] = 2.0f; vertexDataFBO[10] = 1.0f; vertexDataFBO[11] = 0.0f;

    uvDataFBO[0] = 0.0f; uvDataFBO[1] = 0.0f;
    uvDataFBO[2] = 1.0f; uvDataFBO[3] = 0.0f;
    uvDataFBO[4] = 0.0f; uvDataFBO[5] = 1.0f;
    uvDataFBO[6] = 1.0f; uvDataFBO[7] = 1.0f;

    barycentricCoorsFBO[0] = 1.0f; barycentricCoorsFBO[1] = 1.0f; barycentricCoorsFBO[2] = 1.0f;
    barycentricCoorsFBO[3] = 1.0f; barycentricCoorsFBO[4] = 1.0f; barycentricCoorsFBO[5] = 1.0f;
    barycentricCoorsFBO[6] = 1.0f; barycentricCoorsFBO[7] = 1.0f; barycentricCoorsFBO[8] = 1.0f;
    barycentricCoorsFBO[9] = 1.0f; barycentricCoorsFBO[10] = 1.0f; barycentricCoorsFBO[11] = 1.0f;

    affDataFBO[0] = affDataFBO[1] = affDataFBO[2] = affDataFBO[3] = 1.0f;

//    warpAlphaBegin[0] = 0.33f;
//    warpAlphaBegin[1] = 0.33f;
//    warpAlphaBegin[2] = 0.33f;
//    warpAlphaEnd[0] = 0.33f;
//    warpAlphaEnd[1] = 0.33f;
//    warpAlphaEnd[2] = 0.33f;

    for (unsigned int iter=0; iter<i_numImage; ++iter)
    {
        warpAlpha[iter] = warpAlphaBegin[iter];
        warpAlphaMove[iter] = (warpAlphaEnd[iter] - warpAlphaBegin[iter])/100.0;
        blendAlpha[iter] = warpAlpha[iter];
    }

}
//! [7]

//! [8]
void WarpWindow::warpMeshes()
{
    //Normalize the alpha values
    GLfloat sumAlpha = 0.0f;
    for (unsigned int iter=0; iter<i_numImage; ++iter)
        sumAlpha += warpAlpha[iter];
    for (unsigned int iter=0; iter<i_numImage; ++iter)
        warpAlpha[iter] /= sumAlpha;

    //Warp the meshes by simply computing a warpAlpha-weighted sum of all warped versions of a particular mesh
    //i.e., for image i (or triangulation i), the jth vertex is given by:
    //v_i[j] = sum_{k=1}^{i_numImage}(warpAlpha[k]*i_vert[i][k][j])

    for (unsigned int iter=0; iter<i_numImage; ++iter)
    {
        for (unsigned int i=0; i<i_numTriangle[iter]; ++i)
        {
            for(unsigned int j=0; j<3; ++j)
            {
                int idx = 9*i + 3*j;

                warpVertexBufferData[iter][idx] = 1.0f;
                warpVertexBufferData[iter][idx+1] = 0.0f;
                warpVertexBufferData[iter][idx+2] = 0.1f*iter;

                for (unsigned int k=0; k<i_numImage; ++k)
                {
                    warpVertexBufferData[iter][idx] += warpAlpha[k]*i_vert[iter][k][2 * i_tri[iter][3*i+j]];
                    warpVertexBufferData[iter][idx+1] -= warpAlpha[k]*i_vert[iter][k][2 * i_tri[iter][3*i+j] + 1];
                }
            }
        }
    }

    float selfExcludedWarpAlphaSum;
    for (unsigned int iter=0; iter<i_numImage; ++iter)
    {
        selfExcludedWarpAlphaSum = 1.0f - warpAlpha[iter];
        if (selfExcludedWarpAlphaSum<EPSILON)
            selfExcludedWarpAlphaSum = 1.0f;
        for (unsigned int i=0; i<i_numTriangle[iter]; ++i)
        {
            for (unsigned int j=0; j<3; ++j)
            {
                int idx = 3*i + j;
                warpAffBufferData[iter][idx] = 0.0f;
                for (unsigned int k=0; k<i_numImage; ++k)
                {
                    if (k==iter)
                        continue;
                    warpAffBufferData[iter][idx] += (warpAlpha[k]/selfExcludedWarpAlphaSum)*i_affinity[iter][k][i_tri[iter][3*i+j]];
                }
            }
        }
    }
}
//! [8]

//![9]

void WarpWindow::saveCurrentRender(int x_beg, int y_beg, int imWidth, int imHeight)
{
    unsigned char *image = new unsigned char[i_width*i_height*4];
    glReadPixels(x_beg, y_beg, imWidth, imHeight, GL_RGB, GL_UNSIGNED_BYTE, image);
    for (unsigned int i=0; i<imHeight*imWidth*3; ++i)
        image[i] = 255-unsigned char((255-image[i])*histEqMult);

    QImage imgQ(image, imWidth, imHeight, QImage::Format_RGB888);
    imgQ = imgQ.mirrored();
    QWidget *widget = new QWidget;
    QString imagePath = QFileDialog::getSaveFileName(widget, tr("Save File"),"",tr("PNG (*.png);;JPEG (*.jpg *.jpeg);;BMP (*.bmp)" ));
    imgQ.save(imagePath);
    delete[] image;
}
//![9]

void WarpWindow::setAlpha(float *alphaIn)
{
    externalAlpha = true;
    playAnimation = false;
    for (unsigned int iter=0; iter<i_numImage; ++iter)
        warpAlpha[iter] = alphaIn[iter];
}

void WarpWindow::resetAnimation()
{
    direction = 1;
    m_numAnimationMoves = 0;
}

void WarpWindow::computeHistEqMultiplier()
{
    float *image = new float[i_width*i_height*3];
    float curSum = 0, allSum = 0;

    glReadPixels(0, 0, i_width, i_height, GL_RGB, GL_FLOAT, image);

    for (unsigned int i=0; i<i_width*i_height; ++i)
        curSum+=(1.0f-image[3*i+1]);
    curSum /= (i_width*i_height);

    for (unsigned int iter=0; iter<i_numImage; ++iter)
        if (blendImage[iter])
            allSum += blendAlpha[iter]*pixelSum[iter];

    //contrast equalization: compute a multipler histEqMult = ( sum_{i in 1 to numImage}(alpha_i * pixelSum_i) )/pixelSum_{morphedImage}
    //then multiple the morphed image with the multiplier
    histEqMult = allSum/curSum;

    delete[] image;
}
