#include "newcontrol.h"
#include "mainwindow.h"

NewControlWindow::NewControlWindow(WarpWindow *window) : meshRenderer(window)
{
    sliderWidth = 30;
    sliderHeight = 100;
    thumbHeight = 150;
    thumbWidth = 150;
    radioHeight = 30;
}

NewControlWindow::~NewControlWindow()
{

}

void NewControlWindow::initialize(int numImage, std::string imFiles[])
{
    resize(5*thumbWidth, radioHeight+ceil(numImage/5)*(sliderHeight+thumbHeight+radioHeight) );
    QWidget *widget = new QWidget(this);
    widget->setLayout(&grid);
    setCentralWidget(widget);

    createSliders(numImage);
    createThumbs(imFiles, numImage);
    createWarpImageSelectButtons(numImage);
    createButton();
    blendModeGroup.setParent(&grid);
    warpImageGroup.setParent(&grid);

    blendModeGroup.setExclusive(true);
    warpImageGroup.setExclusive(true);

}

void NewControlWindow::createButton()
{
    blendModeButton[0].setText(QString("Blend Multiple Sketches"));
    blendModeButton[0].setParent(this);
    blendModeButton[1].setText(QString("Warp One Sketch"));
    blendModeButton[1].setParent(this);
    blendModeGroup.addButton(blendModeButton, 0);
    blendModeGroup.addButton(blendModeButton, 1);
    grid.addWidget(&blendModeButton[0], 0, 2);
    grid.addWidget(&blendModeButton[1], 0, 3);
    blendModeButton[0].setGeometry(width()/2-200, 0, radioHeight, 200);
    qDebug()<<width()<<height();
    blendModeButton[1].setGeometry(width()/2, 0, radioHeight, 200);

    blendModeButton[0].setDown(true);
}

void NewControlWindow::createSliders(int numImage)
{
    alphaSlider = new QSlider *[numImage];
    for(unsigned int i=0; i<numImage; ++i)
    {
        alphaSlider[i] = new QSlider;
        alphaSlider[i]->setMinimum(0);
        alphaSlider[i]->setMaximum(100);
        alphaSlider[i]->setGeometry(0, 0, sliderHeight, sliderWidth);
        grid.addWidget(alphaSlider[i], 1 + 3*(i/5), i%5, Qt::AlignCenter);
    }
}

void NewControlWindow::createWarpImageSelectButtons(int numImage)
{

}

void NewControlWindow::createThumbs(std::string *imFiles, int numImage)
{
    thumbs = new QPixmap *[numImage];

    for (unsigned int i=0; i<numImage; ++i)
    {
        thumbs[i] = new QPixmap;

        thumbs[i]->load(QString::fromStdString(imFiles[i]));
        thumbs[i]->setMask(thumbs[i]->createMaskFromColor(QColor(255, 255, 255)));
        float targetHeight = thumbHeight;

        if (thumbs[i]->height()<thumbs[i]->width())
            targetHeight = std::max(float(thumbHeight*thumbs[i]->height()/thumbs[i]->width()), 100.0f);

        thumbs[i] = thumbs[i].scaledToHeight(targetHeight, Qt::SmoothTransformation);

        QLabel *label = new QLabel(this);
        label->setPixmap(thumbs[i]);
        label->setGeometry(0, 0, thumbs[i]->width(), thumbs[i]->height());
        grid.addWidget(label, 1 + 3*(i/5) + 1, i%5, Qt::AlignCenter);

    }
}
