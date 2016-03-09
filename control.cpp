#include "mainwindow.h"
#include "control.h"
#include "node.h"
#include "edge.h"
#include "triangle.h"

#include <QtCore/qmath.h>
#include <QDebug>
#include <QVector2D>
#include <QPointF>
#include <algorithm>

#define ANSI_DECLARATORS


ControlWindow::ControlWindow(WarpWindow *mainWindow, QGraphicsScene *sceneIn) : QGraphicsView(sceneIn), meshRenderer(mainWindow), scene(sceneIn)
{
    windowSize = sceneIn->sceneRect().width();
    anchorPen = new QPen(Qt::white);
    anchorBrush = new QBrush(Qt::lightGray);
    movePen = new QPen(Qt::darkBlue);
    moveBrush= new QBrush(Qt::darkCyan);
}

void ControlWindow::init(int i_numImage, bool warpTypeIn, QPointF **positions, std::string imFiles[])
{
    numImage = i_numImage;

    circles = new Node *[numImage];

    posBeginX = 10*windowSize;
    posBeginY = posEndX = posEndY = posBeginX;
    startPosGivenFlag = 0;

    polySize = 0.7*0.5*windowSize;

    warpType = warpTypeIn;

    qDebug()<<"Arrangement: (regular: 0, matching-based: 1)"<<warpType;

    for (int i=0; i<numImage; ++i)
    {
        circles[i] = new Node(this, i, ANCHOR_NODE);
        scene->addItem(circles[i]);
        if (!warpType)
        {
            circles[i]->setPos(polySize*cos(2*M_PI*(float)i/numImage), -polySize*sin(2*M_PI*(float)i/numImage));
        }
        else
        {
            circles[i]->setPos(-polySize + 2*polySize*positions[i]->x(), -polySize + 2*polySize*positions[i]->y());
        }
    }

    mainNode = new Node(this, -1, WARP_CURRENT_NODE);
    scene->addItem(mainNode);
    if (numImage>2)
        triangleWrapper();  //triangulation as well as edge creation is handled by triangleWrapper()
    else
    {
        numEdge = 1;
        lines = new Edge *[1];
        lines[0] = new Edge(circles[0], circles[1]);
        scene->addItem(lines[0]);
    }

    startPosGivenFlag = false;
    posBeginX = posBeginY = posEndX = posEndY = 10*windowSize;
    playAnimation = false;
    meshRenderer->resetAnimation();
    setAlpha(0.0, 0.0);

    thumbHeight = 200;
    loadThumbs(imFiles);
    addSlider();
}

void ControlWindow::addSlider()
{
    int sliderHeight = 20;
    int labelWidth = windowSize/6;
    int labelHeight = 50;
    QPalette pal(palette());
    pal.setColor(QPalette::Background, Qt::white);

    fuzzinessSlider = new QSlider(Qt::Horizontal);
    alphaSliderProxy = scene->addWidget(fuzzinessSlider);
    fuzzinessSlider->setGeometry(-windowSize/4, windowSize/2, windowSize/2, sliderHeight);
    fuzzinessSlider->setMinimum(0);
    fuzzinessSlider->setMaximum(49);
    fuzzinessSlider->setTracking(true);
    fuzzinessSlider->setTickPosition(QSlider::TicksBelow);
    fuzzinessSlider->setTickInterval(25);
    connect(fuzzinessSlider, SIGNAL(valueChanged(int)), meshRenderer, SLOT(setAffExponent(int)));

    fuzzinessSlider->setAutoFillBackground(true);
    fuzzinessSlider->setPalette(pal);

    //create labels
    QLabel *labelStart = new QLabel(QString("Draw All Contours"));
    QLabel *labelMid = new QLabel(QString("Prefer Contours from Nearest Sketch"));
    QLabel *labelEnd = new QLabel(QString("Draw Contours from Nearset Sketch Only"));

    labelStart->setGeometry(-windowSize/4-labelWidth/2, windowSize/2 + sliderHeight + 5, labelWidth, labelHeight);
    labelMid->setGeometry(-labelWidth/2, windowSize/2 + sliderHeight + 5, labelWidth, labelHeight);
    labelEnd->setGeometry(windowSize/4-labelWidth/2, windowSize/2 + sliderHeight + 5, labelWidth, labelHeight);

    labelStart->setWordWrap(true);
    labelMid->setWordWrap(true);
    labelEnd->setWordWrap(true);

    labelStart->setAlignment(Qt::AlignCenter);
    labelMid->setAlignment(Qt::AlignCenter);
    labelEnd->setAlignment(Qt::AlignCenter);

//    labelStart->setAutoFillBackground(true);
//    labelStart->setPalette(pal);
//    labelMid->setAutoFillBackground(true);
//    labelMid->setPalette(pal);
//    labelEnd->setAutoFillBackground(true);
//    labelEnd->setPalette(pal);

    labelMid->setStyleSheet(QString("background-color: rgba(255, 255, 255, 0)"));
    labelStart->setStyleSheet(QString("background-color: rgba(255, 255, 255, 0)"));
    labelEnd->setStyleSheet(QString("background-color: rgba(255, 255, 255, 0)"));

    scene->addWidget(labelStart);
    scene->addWidget(labelMid);
    scene->addWidget(labelEnd);

    fuzzinessSlider->setValue(20);
}

void ControlWindow::setPosition(float *warpAlpha)
{
    float posX = 0.0, posY = 0.0;
    for (int i=0; i<numImage; ++i)
    {
        posX += warpAlpha[i]*circles[i]->x();
        posY += warpAlpha[i]*circles[i]->y();
    }
    mainNode->setPos(posX, posY);
}
void ControlWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton)
    {
        float xPos = event->x() - windowSize/2;
        float yPos = event->y() - windowSize/2;
        if (yPos>=windowSize/2)
        {
            fuzzinessSlider->setValue(std::max(std::min(int(floor(50*(xPos+windowSize/4)/(windowSize/2))), 50), 0));
            return;
        }
        posBeginX = posBeginY = posEndX = posEndY = 10*windowSize;
        playAnimation = false;
        meshRenderer->resetAnimation();
        setAlpha(xPos, yPos);
    }
}

void ControlWindow::mousePressEvent(QMouseEvent *event)
{
//    qDebug()<<"Mouse pressed";
    float xPos = event->x() - windowSize/2;
    float yPos = event->y() - windowSize/2;

    if (yPos>=windowSize/2)
    {
        fuzzinessSlider->setValue(std::max(std::min(int(floor(50*(xPos+windowSize/4)/(windowSize/2))), 50), 0));
        return;
    }

//    qDebug()<<xPos<<yPos;

    if (event->button()==Qt::LeftButton)     //left click
    {
        posBeginX = posBeginY = posEndX = posEndY = 10*windowSize;
        playAnimation = false;
        startPosGivenFlag = false;
        meshRenderer->resetAnimation();
        setAlpha(xPos, yPos);
    }
    else        //right button (or any other button but the left)
    {
        if (startPosGivenFlag==false)
        {
            startPosGivenFlag = true;
            posBeginX = xPos;
            posBeginY = yPos;
            posEndX = posEndY = 10*windowSize;
            playAnimation = false;
        }
        else
        {
            startPosGivenFlag = false;
            posEndX = xPos;
            posEndY = yPos;

            playAnimation = true;
            meshRenderer->resetAnimation();
        }
    }
//    qDebug()<<"mousePressEvent is okay";
}

void ControlWindow::triangleWrapper()
{
    struct triangulateio *in, *out;
    in = new struct triangulateio;
    out = new struct triangulateio;
    in->numberofpointattributes = 0;
    in->pointlist = new REAL[2*numImage];
    in->numberofpoints = numImage;
    in->pointmarkerlist = NULL;


    for(int i=0; i<numImage; ++i)
    {
        in->pointlist[2*i] = circles[i]->x();
        in->pointlist[2*i+1] = circles[i]->y();
    }

    out->pointlist = NULL;
    out->trianglelist = NULL;
    out->triangleattributelist = NULL;
    out->edgelist = NULL;
    out->edgemarkerlist = NULL;

    qDebug()<<"Triangulating now!";

    char opts[] = "zBPo0e";
    triangulate(opts, in, out, NULL);

    for (int i=0; i<out->numberoftriangles; ++i)
        qDebug()<<out->trianglelist[3*i]<<out->trianglelist[3*i+1]<<out->trianglelist[3*i+2];

    triangles = new int *[out->numberoftriangles];
    numTriangle = out->numberoftriangles;

    for (int i=0; i<numTriangle; ++i)
    {
        triangles[i] = new int[3];
        triangles[i][0] = out->trianglelist[3*i];
        triangles[i][1] = out->trianglelist[3*i+1];
        triangles[i][2] = out->trianglelist[3*i+2];

        qDebug()<<triangles[i][0]<<' '<<triangles[i][1]<<' '<<triangles[i][2];
    }

    numEdge = out->numberofedges;
    lines = new Edge *[numEdge];
    for (int i=0; i<numEdge; ++i)
    {
        lines[i] = new Edge(circles[out->edgelist[2*i]], circles[out->edgelist[2*i+1]]);
        scene->addItem(lines[i]);
    }

    free(in->pointlist);
    free(out->trianglelist);
    free(out->pointlist);
    free(out->edgelist);
    delete in, out;
    qDebug()<<"Triangulation computed!";
}

bary *ControlWindow::barycentric(float xPos, float yPos)
{
    float *allBary = new float [3*numTriangle];
    float *distCenter = new float [numTriangle];
    float x0, y0, x1, y1, x2, y2, u, v, w;
    int minDistIdx = 0;
    bary *b = new bary;
    for (int i=0; i<numTriangle; ++i)
    {
        x0 = circles[triangles[i][0]]->x();
        y0 = circles[triangles[i][0]]->y();
        x1 = circles[triangles[i][1]]->x();
        y1 = circles[triangles[i][1]]->y();
        x2 = circles[triangles[i][2]]->x();
        y2 = circles[triangles[i][2]]->y();

        v = ((x0-x2)*(yPos-y2) - (xPos-x2)*(y0-y2)) / ((x1-x0)*(y2-y0) - (x2-x0)*(y1-y0));
        w = ((x1-x0)*(yPos-y0) - (xPos-x0)*(y1-y0)) / ((x1-x0)*(y2-y0) - (x2-x0)*(y1-y0));
        u = 1.0 - v - w;

        allBary[3*i] = u; allBary[3*i+1] = v; allBary[3*i+2] = w;
        distCenter[i] = sqrt(pow(xPos-(x0+x1+x2)/3.0, 2) + pow(yPos-(y0+y1+y2)/3.0, 2));

        if(distCenter[i]<distCenter[minDistIdx])
            minDistIdx = i;

        if (u>=0.0 && v>=0.0 && w>=0.0 && u<=1.0 && v<=1.0 && w<=1.0)
        {
            b->triangleId = i;
            b->coordinates[0] = u; b->coordinates[1] = v; b->coordinates[2] = w;
            delete[] allBary;
            delete[] distCenter;
            return b;
        }
    }

    b->triangleId = minDistIdx;
    b->coordinates[0] = allBary[3*minDistIdx]; b->coordinates[1] = allBary[3*minDistIdx+1]; b->coordinates[2] = allBary[3*minDistIdx+2];

//    qDebug()<<"*Extrapolating*\nTriangle No. "<<b->triangleId;
//    qDebug()<<"Sketches used: "<<triangles[b->triangleId][0]<<" "<<triangles[b->triangleId][1]<<" "<<triangles[b->triangleId][2];
//    qDebug()<<"Barycentric Coordinates:"<<b->coordinates[0]<<" "<<b->coordinates[1]<<" "<<b->coordinates[2];

    delete[] allBary;
    delete[] distCenter;
    return b;
}

ControlWindow::~ControlWindow()
{
    for (int i=0; i<numImage; ++i)
    {
        delete[] triangles[i];
        delete circles[i];
        delete thumbs[i];
    }

    for (int i=0; i<numEdge; ++i)
            delete lines[i];

    delete mainNode;
    delete[] triangles;
    delete[] lines;
    delete[] circles;
    delete[] thumbs;

    delete anchorBrush, anchorPen, moveBrush, movePen;
}

void ControlWindow::setAlphaAnim(bool direction, int numAnimationSteps, int maxAnimationSteps)
{
    float xPos, yPos;

    float xStep = (posEndX - posBeginX)/maxAnimationSteps;
    float yStep = (posEndY - posBeginY)/maxAnimationSteps;

    if (direction)
    {
        xPos = posBeginX + numAnimationSteps*xStep;
        yPos = posBeginY + numAnimationSteps*yStep;
    }
    else
    {
        xPos = posEndX - numAnimationSteps*xStep;
        yPos = posEndY - numAnimationSteps*yStep;
    }
    return setAlpha(xPos, yPos);
}

void ControlWindow::setAlpha(float xPos, float yPos)
{
    //First, find out the intended position of the main node (red dot), using the parameters
    //and posBegin*, posEnd*. Set main node to this position. Then, compute the implied warp
    //alpha values using barycentric coordinates, and return these as a float array.

//    //THIS IS THE CODE THAT MIGHT NOT WORK
//    float sumAlpha = 0;
//    float *warpAlpha = new float[numImage];
//    for (int i=0; i<numImage; ++i)
//    {
//        warpAlpha[i] = sqrt((xPos - circles[i]->x())*(xPos - circles[i]->x()) + (yPos - circles[i]->y())*(yPos - circles[i]->y()));
//        sumAlpha += warpAlpha[i];
//    }

//    for (int i=0; i<numImage; ++i)
//    {
//        warpAlpha[i] = warpAlpha[i]/sumAlpha;
//        qDebug()<<warpAlpha[i];
//    }

//    meshRenderer->setAlpha(warpAlpha);
//    mainNode->setPos(xPos, yPos);

//    delete[] warpAlpha;
//    return;

    if (numImage>=3)
    {
        float *warpAlpha = new float[numImage];
        bary *b = barycentric(xPos, yPos);

        if (b==NULL)
            return;
        else while (b->coordinates[0]<0.0 || b->coordinates[1]<0.0 || b->coordinates[2]<0.0 ||
                 b->coordinates[0]>1.0 || b->coordinates[1]>1.0 || b->coordinates[2]>1.0)
        {
//            qDebug()<<b->coordinates[0]<<b->coordinates[1]<<b->coordinates[2];
//            if (xPos>0)
//                xPos = xPos - 1;
//            else if (xPos<0)
//                xPos = xPos + 1;

//            if (yPos>0)
//                yPos = yPos - 1;
//            else if (yPos<0)
//                yPos = yPos + 1;

//            b = barycentric(xPos, yPos);
            return;
        }

        //THIS IS THE CODE THAT WORKS
        for (int i=0; i<numImage; ++i)
            warpAlpha[i] = 0;

        warpAlpha[triangles[b->triangleId][0]] = b->coordinates[0];
        warpAlpha[triangles[b->triangleId][1]] = b->coordinates[1];
        warpAlpha[triangles[b->triangleId][2]] = b->coordinates[2];
        meshRenderer->setAlpha(warpAlpha);

        mainNode->setPos(b->coordinates[0]*circles[triangles[b->triangleId][0]]->x()
                + b->coordinates[1]*circles[triangles[b->triangleId][1]]->x()
                + b->coordinates[2]*circles[triangles[b->triangleId][2]]->x()
                , b->coordinates[0]*circles[triangles[b->triangleId][0]]->y()
                + b->coordinates[1]*circles[triangles[b->triangleId][1]]->y()
                + b->coordinates[2]*circles[triangles[b->triangleId][2]]->y());

//        qDebug()<<b->coordinates[0]<<b->coordinates[1]<<b->coordinates[2];
//        qDebug()<<mainNode->x()<<mainNode->y();

        delete b;
        delete[] warpAlpha;
    }
    else
    {
        float x0 = circles[0]->x(), y0 = circles[0]->y();
        float x1 = circles[1]->x(), y1 = circles[1]->y();

//        qDebug()<<xPos<<x0<<x1;
//        if (xPos>x0 || xPos<x1)
//        {
//            //Extrapolation: Insert code here
//            return;
//        }


        float dist0 = x0-xPos;
        float dist1 = xPos-x1;
        float warpAlpha[] = {dist1/(dist0+dist1), dist0/(dist0+dist1)};
        meshRenderer->setAlpha(warpAlpha);
        mainNode->setPos(warpAlpha[0]*x0+warpAlpha[1]*x1, warpAlpha[0]*y0+warpAlpha[1]*y1);
    }
}

void ControlWindow::toggleAnimation()
{
    if (playAnimation)
        playAnimation = false;
    else if (posEndX < windowSize)
        playAnimation = true;
}

bool ControlWindow::getAnimationState()
{
    return playAnimation;
}

void ControlWindow::toggleArrangement(QPointF **positions)
{
    warpType = !warpType;
    for (int i=0; i<numImage; ++i)
    {
        if (!warpType)
        {
            circles[i]->setPos(polySize*cos(2*M_PI*(float)i/numImage), -polySize*sin(2*M_PI*(float)i/numImage));
        }
        else
        {
            circles[i]->setPos(-polySize + polySize*positions[i]->x(), -polySize + polySize*positions[i]->y());
        }
    }

    startPosGivenFlag = false;
    posBeginX = posBeginY = posEndX = posEndY = 10*windowSize;
    playAnimation = false;
    meshRenderer->resetAnimation();
    setAlpha(polySize, 0.0);
}

void ControlWindow::flipEmbedding(axes axis)
{
    for (int i=0; i<numImage; ++i)
    {
        if (axis==X)
        {
            circles[i]->setPos(-circles[i]->x(), circles[i]->y());
        }
        else if(axis==Y)
        {
            circles[i]->setPos(circles[i]->x(), -circles[i]->y());
        }
        thumbs[i]->setPos(circles[i]->x()-thumbs[i]->pixmap().width()/2, circles[i]->y()-thumbs[i]->pixmap().height()/2);
    }

    if (axis==X)
        mainNode->setPos(-mainNode->x(), mainNode->y());
    else if (axis==Y)
        mainNode->setPos(mainNode->x(), -mainNode->y());
}

void ControlWindow::switchAxes()
{
    for (int i=0; i<numImage; ++i)
    {
        circles[i]->setPos(circles[i]->y(), circles[i]->x());
        thumbs[i]->setPos(circles[i]->x()-thumbs[i]->pixmap().width()/2, circles[i]->y()-thumbs[i]->pixmap().height()/2);
    }

    mainNode->setPos(mainNode->y(), mainNode->x());
}

void ControlWindow::loadThumbs(std::string imFiles[])
{
    thumbsVisible = true;
    QPixmap pixmap;
//    QString str;
    thumbs = new QGraphicsPixmapItem *[numImage];

    for (int i=0; i<numImage; ++i)
    {
        thumbs[i] = new QGraphicsPixmapItem;
        thumbs[i]->setTransformationMode(Qt::SmoothTransformation);
        thumbs[i]->setZValue(-100);

        pixmap.load(QString::fromStdString(imFiles[i]), "PNG");
//        pixmap.setMask(pixmap.createHeuristicMask());
        pixmap.setMask(pixmap.createMaskFromColor(QColor(255, 255, 255)));
//        if (pixmap.hasAlphaChannel())
//        {

//        }
        float targetHeight = thumbHeight;

        if (pixmap.height()<pixmap.width())
            targetHeight = std::max(float(thumbHeight*pixmap.height()/pixmap.width()), 100.0f);

//        while (pixmap.height()>2*targetHeight)
//            pixmap = pixmap.scaledToHeight(pixmap.height()/2, Qt::SmoothTransformation);

        pixmap = pixmap.scaledToHeight(targetHeight, Qt::SmoothTransformation);

        thumbs[i]->setPixmap(pixmap);

        thumbs[i]->setPos(circles[i]->x()-thumbs[i]->pixmap().width()/2, circles[i]->y()-pixmap.height()/2);
        scene->addItem(thumbs[i]);
    }


}

void ControlWindow::toggleThumbsVisibility()
{
    thumbsVisible = !thumbsVisible;
    for (int i=0; i<numImage; ++i)
    {
        if (thumbsVisible)
            thumbs[i]->show();
        else
            thumbs[i]->hide();
    }
}

void ControlWindow::keyPressEvent(QKeyEvent *event)
{
    switch (event->key())
    {
        case Qt::Key_X:
            flipEmbedding(X);
            break;
        case Qt::Key_Y:
            flipEmbedding(Y);
            break;
        case Qt::Key_F:
            switchAxes();
            break;
        case Qt::Key_I:
            toggleThumbsVisibility();
            break;
        case Qt::Key_J:
            posBeginX = posBeginY = posEndX = posEndY = 10*windowSize;
            playAnimation = false;
            startPosGivenFlag = false;
            meshRenderer->resetAnimation();
            setAlpha(-polySize/2, 0.0);
            break;
        case Qt::Key_K:
            posBeginX = posBeginY = posEndX = posEndY = 10*windowSize;
            playAnimation = false;
            startPosGivenFlag = false;
            meshRenderer->resetAnimation();
            setAlpha(0.0, 0.0);
            break;
        case Qt::Key_L:
            posBeginX = posBeginY = posEndX = posEndY = 10*windowSize;
            playAnimation = false;
            startPosGivenFlag = false;
            meshRenderer->resetAnimation();
            setAlpha(polySize/2, 0.0);
            break;
        case Qt::Key_Escape:
            close();
        case Qt::Key_P:
            for (int i=0; i<numImage; ++i)
                qDebug()<<circles[i]->x()<<circles[i]->y();
            qDebug()<<mainNode->x()<<mainNode->y();
    }
}

void ControlWindow::closeEvent(QCloseEvent *event)
{
    event->accept();
    meshRenderer->hide();
    qApp->quit();
}
