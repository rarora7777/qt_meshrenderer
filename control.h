#ifndef CONTROL_H
#define CONTROL_H

#include <QtWidgets>
#include <QPen>
#include <QBrush>
#include "mainwindow.h"
#include <QSlider>
#include <QGraphicsProxyWidget>

class Node;
class Edge;

struct bary
{
    int triangleId;
    float coordinates[3];
};

class ControlWindow : public QGraphicsView
{
public:
    ControlWindow(WarpWindow *mainWindow, QGraphicsScene *sceneIn);
    ~ControlWindow();
    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE;
    void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;
    void init(int i_numImage, bool warpTypeIn, QPointF **positions, std::string imFiles[10]);
    void setPosition(float *);
    bool getAnimationState();
    void setAlphaAnim(bool, int, int);
//    float *getNewAlphaValues();
    void toggleAnimation();
    void toggleArrangement(QPointF **positions);
//    bool canControl;

//protected:
//    virtual void resizeEvent(QResizeEvent *) Q_DECL_OVERRIDE;

private:
    WarpWindow *meshRenderer;
    Node **circles;
    Edge **lines;
    int numEdge;
    int *edgeList[2];
    Node *mainNode;
    int windowSize;
    int polySize;
    int numImage;
    QPen *anchorPen;
    QBrush *anchorBrush;
    QPen *movePen;
    QBrush *moveBrush;
    QGraphicsScene *scene;
    int **triangles;
    int numTriangle;
    bool warpType;
    float posBeginX, posBeginY, posEndX, posEndY;
    bool playAnimation;
    bool startPosGivenFlag;
    bool thumbsVisible;
    QGraphicsPixmapItem **thumbs;
    float thumbHeight;
    QSlider *fuzzinessSlider;
    QGraphicsProxyWidget *alphaSliderProxy;

    bary *barycentric(float xPos, float yPos);
    enum axes {X, Y};
    void flipEmbedding(axes axis);
    void switchAxes();
    void setAlpha(float xPos, float yPos);
    void triangleWrapper();
    void loadThumbs(std::string imFiles[]);
    void toggleThumbsVisibility();
    void addSlider();
};


#endif // CONTROL_H
