#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui/QWindow>
#include <QtGui/QOpenGLFunctions_4_2_Core>
#include <QtGui/QOpenGLShaderProgram>
#include <QTime>
#include <QPointF>
#include <algorithm>

QT_BEGIN_NAMESPACE
class QPainter;
class QOpenGLContext;
class QOpenGLPaintDevice;
QT_END_NAMESPACE

//! [1]
class OpenGLWindow : public QWindow, protected QOpenGLFunctions_4_2_Core
{
    Q_OBJECT
public:
    explicit OpenGLWindow(QWindow *parent = 0);
    ~OpenGLWindow();

    virtual void render(QPainter *painter);
    virtual void render();

    virtual void initialize();

    void setAnimating(bool animating);

public slots:
    void renderLater();
    void renderNow();

protected:
    bool event(QEvent *event) Q_DECL_OVERRIDE;

    void exposeEvent(QExposeEvent *event) Q_DECL_OVERRIDE;

private:
    bool m_update_pending;
    bool m_animating;

    QOpenGLContext *m_context;
    QOpenGLPaintDevice *m_device;
};
//! [1]


//! [2]
class WarpWindow : public OpenGLWindow
{
    Q_OBJECT
public:
    WarpWindow(char *inputFile);
    ~WarpWindow();

    void initialize() Q_DECL_OVERRIDE;
    void loadInputData(std::string);
    void render() Q_DECL_OVERRIDE;
    void setAlpha(float *alphaIn);
    float m_ctrlWindowSize;

    GLuint getNumImage()
    {
        return i_numImage;
    }

    void resetAnimation();
    int getMaxAnimationSteps()
    {
        return m_maxAnimationSteps;
    }

public slots:
    void setAffExponent(int);
    void setBlendMode(int);
    void setWarpingImage(int);

private:
    QOpenGLShader *loadShader(QOpenGLShader::ShaderType type, const char *source);
    bool event(QEvent *event) Q_DECL_OVERRIDE;
    void visibilityChanged(QWindow::Visibility visibility);
    void initializeMeshData();
    void warpMeshes();
    void saveCurrentRender(int, int, int, int);
    void computeHistEqMultiplier();
    void showblendAlphas()
    {
        QDebug deb = qDebug();
        for (unsigned int i=0; i<i_numImage; ++i)
            deb<<blendAlpha[i]<<"("<<blendImage[i]<<")";
    }

    GLuint m_matrixUniform;     //location of MVP matrix in v-shader
    GLuint m_textureUniform;    //location of texture attribute in v-shader
    GLuint m_alphaUniform;      //location of alpha in f-shader
    GLuint m_baryAttrib;        //location of barycentric coordinate attribute in v-shader
    GLuint m_blendUniform;
    GLuint m_histEqMultUniform;
    GLuint m_textureFBOUniform;
    GLuint m_affExponentUniform;
    GLuint m_affinityAttrib;
    GLuint m_FBO;
    GLuint m_isMaxAlphaUniform;
    GLuint texture_temp;
    GLuint m_textureFBO_0, m_textureFBO_1, m_textureFBO_2, m_textureFBO_3;
    GLfloat m_translate;        //degubbing variable: translate all objects on z-axis by m_translate
    GLuint vertexBuffer;
    GLuint uvBuffer;
    GLuint baryBuffer;
    GLuint affinityBuffer;
    QMatrix4x4 mvp;
    GLboolean externalAlpha;
    GLfloat curAlpha;           //debugging variable for alpha blending
    GLfloat warpAlpha[15];      //alpha values for warping all images (actually decides warping as well as blending)
    GLfloat warpAlphaBegin[15];
    GLfloat warpAlphaEnd[15];
    GLfloat warpAlphaMove[15];
    GLfloat blendAlpha[15];     //alpha values for blending images (differs from warpAlpha by only a normalizing factor)
    GLfloat pixelSum[15];

    GLboolean direction;        //animation direction: fwd or back
    GLboolean playAnimation;    //play/pause animation
    GLuint m_numAnimationMoves;
    GLuint m_maxAnimationSteps;
    GLuint blendMode;           //blending mode: 1: texture[0], 2: both, 3: texture[1]
    GLboolean blendImage[10];   //blendImage[i]=1 implies that texture[i] is to be used in blending
    GLuint activeOrigImage;
    GLboolean showWireframe;
    GLuint m_showWireframeUniform;
    GLboolean screenshotTrigger;
    GLboolean showBlendAlphasTrigger;
    bool warpType;         //read warp type from file: 0 for shape interpolation only, 1 for viewpoint interpolation
                                //this affects how the images are arranged in the final application
    bool affinitiesGiven;   //if affinities have been provided, or should be assumed to be uniform throughout (default value is 1)
    float affExponent;
    QPointF **warpPositions;
    float histEqMult;

    GLuint i_texture[15];           //stores all textures
    std::string i_textureFile[15];  //stores texture filenames (useful until textures have been loaded)
    GLuint i_thinTexture[15][8];    //stores all thinned textures
    std::string i_thinTextureFile[15][8];   //thinned texture filenames
    GLboolean thinning;
    GLboolean thinningState;

    GLuint i_numImage;          //number of images
    GLuint i_numTriangle[15];   //number of triangles per mesh
    GLuint i_numTriangleMax;
    GLuint i_numThinningLevels; //number of thinning levels per sketch
    GLuint i_height;            //height of each image (all images must have the same dimenstions)
    GLuint i_width;
    GLuint *i_tri[15];          //mesh data: triangulation (triplets representing triangles)
    GLfloat *i_vert[15][15];    //mesh data: vertex locations
    GLfloat *i_affinity[15][15];     //affnities of mesh vertices in each image towards all other images

    GLfloat *warpVertexBufferData[15];
    GLfloat *origVertexBufferData[15];
    GLfloat *uvBufferData[15];
    GLfloat *barycentricCoors;
    GLfloat *warpAffBufferData[15];
    GLfloat *origAffBufferData[15];
    GLfloat vertexDataFBO[12];
    GLfloat uvDataFBO[8];
    GLfloat barycentricCoorsFBO[12];
    GLfloat affDataFBO[4];

    QOpenGLShaderProgram *m_program;
    int m_frame;

    QTime m_time;

};

//! [2]
#endif
