#include "newcontrol.h"
#include "mainwindow.h"

NewControlWindow::NewControlWindow(WarpWindow *window) : meshRenderer(window)
{
    sliderWidth = 30;
    sliderHeight = 150;
    thumbHeight = 100;
    thumbWidth = 100;
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
    createButton(numImage);
    blendModeGroup.setParent(&grid);
    warpImageGroup.setParent(&grid);

    blendModeGroup.setExclusive(true);
    warpImageGroup.setExclusive(true);

    i_numImage = numImage;

    residualDelta = new float[i_numImage];
    for (int i=0; i<i_numImage; ++i)\
        residualDelta[i] = 0.0f;

//    emit blendModeGroup.buttonClicked(0);
}

void NewControlWindow::createButton(int numImage)
{
    blendModeButton[0].setText(QString("Blend Multiple Sketches"));
    blendModeButton[0].setParent(this);
    blendModeButton[1].setText(QString("Warp One Sketch"));
    blendModeButton[1].setParent(this);
    blendModeGroup.addButton(&blendModeButton[0], 0);
    blendModeGroup.addButton(&blendModeButton[1], 1);
    grid.addWidget(&blendModeButton[0], 0, numImage/2);
    grid.addWidget(&blendModeButton[1], 0, numImage/2+1);
    blendModeButton[0].setGeometry(0, 0, 200, radioHeight);
    blendModeButton[1].setGeometry(0, 0, 200, radioHeight);

    blendModeButton[0].setChecked(true);

    connect(&blendModeGroup, SIGNAL(buttonClicked(int)), meshRenderer, SLOT(setBlendMode(int)));
    connect(&blendModeGroup, SIGNAL(buttonClicked(int)), this, SLOT(toggleWarpImageButtons(int)));
}

void NewControlWindow::createSliders(int numImage)
{
    alphaSlider = new QSlider *[numImage];
    for(unsigned int i=0; i<numImage; ++i)
    {
        alphaSlider[i] = new QSlider;
        alphaSlider[i]->setMinimum(1);
        alphaSlider[i]->setMaximum(100);
        alphaSlider[i]->setGeometry(0, 0, sliderWidth, sliderHeight);
        alphaSlider[i]->setMinimumHeight(sliderHeight);
        alphaSlider[i]->setMaximumHeight(sliderHeight);
        alphaSlider[i]->setTracking(true);
        alphaSlider[i]->setValue(100/numImage);
        grid.addWidget(alphaSlider[i], 1 + 3*(i/5), i%5, Qt::AlignCenter);
        connect(alphaSlider[i], &QSlider::valueChanged, [this, i](int val)
            {
                this->setAlphaValues(i, val);
            } );
    }
    alphaSlider[0]->setValue(100/numImage);
}

void NewControlWindow::createWarpImageSelectButtons(int numImage)
{
    warpImageButton = new QRadioButton *[numImage];

    for (unsigned int i=0; i<numImage; ++i)
    {
        warpImageButton[i] = new QRadioButton(this);
        warpImageGroup.addButton(warpImageButton[i], i);
        grid.addWidget(warpImageButton[i], 1 + 3*(i/5) + 2, i%5, Qt::AlignCenter);
        warpImageButton[i]->setGeometry(0, 0, thumbWidth, radioHeight);
        warpImageButton[i]->setEnabled(false);
        connect(warpImageButton[i], &QRadioButton::clicked, [this, i]()
            {
                meshRenderer->setWarpingImage(i);
            } );
    }
    warpImageButton[0]->setChecked(true);

}

void NewControlWindow::createThumbs(std::string *imFiles, int numImage)
{
    thumbs = new QPixmap *[numImage];

    for (unsigned int i=0; i<numImage; ++i)
    {
        QPixmap temp;

        temp.load(QString::fromStdString(imFiles[i]));
        float targetHeight = thumbHeight;

        if (temp.height()<temp.width())
            targetHeight = std::max(float(thumbHeight*temp.height()/temp.width()), 100.0f);

        thumbs[i] = new QPixmap(temp.scaledToHeight(targetHeight, Qt::SmoothTransformation));

        QLabel *label = new QLabel(this);
        label->setGeometry(0, 0, thumbs[i]->width(), thumbs[i]->height());
        label->setPixmap(*thumbs[i]);
        grid.addWidget(label, 1 + 3*(i/5) + 1, i%5, Qt::AlignCenter);
    }
}

void NewControlWindow::setAlphaValues(int sliderIdx, int val)
{
    //qDebug()<<sliderIdx<<val;
    float *alpha = new float[i_numImage];
    float sumAll = 0, deltaSum = 0, sumInactive;
    float *delta = new float[i_numImage];
    float *values = new float[i_numImage];

    for (int i=0; i<i_numImage; ++i)
    {
        alphaSlider[i]->blockSignals(true);
        values[i] = alphaSlider[i]->value();
        sumAll+=values[i];
    }

    deltaSum = (100+i_numImage-1)-sumAll;
    sumInactive = sumAll - val;

    if (sumInactive)
    {
        QDebug deb = qDebug();
        for (int i=0; i<i_numImage; ++i)
            if (i!=sliderIdx)
            {
//                delta[i] = (deltaSum*values[i])/sumInactive;
//                residualDelta[i]+= delta[i] - int(delta[i]);
//                if (fabs(residualDelta[i])>=1.0)
//                {
//                    delta[i] = delta[i] + (residualDelta>0) ? 1.0f : -1.0f;
//                    residualDelta[i] = residualDelta[i] - (residualDelta>0) ? 1.0f : -1.0f;
//                }
//                deb<<delta[i]<<residualDelta[i];
                alphaSlider[i]->setValue(values[i] + deltaSum/(i_numImage-1));
            }
    }
    else
    {
        for (int i=0; i<i_numImage; ++i)
            if (i!=sliderIdx)
                alphaSlider[i]->setValue(values[i] + 1);
            else
                alphaSlider[i]->setValue(values[i] - (i_numImage-1));
    }
    for (int i=0; i<i_numImage; ++i)
        alphaSlider[i]->blockSignals(false);

    getAlphaValues(alpha);
    meshRenderer->setAlpha(alpha);

    delete[] alpha, delta, values;

}

int NewControlWindow::getActiveSlider()
{
    return warpImageGroup.checkedId();
}

void NewControlWindow::getAlphaValues(float *alpha)
{
    for (int i=0; i<i_numImage; ++i)
        alpha[i] = float(alphaSlider[i]->value())/100;
}

void NewControlWindow::toggleWarpImageButtons(int blendMode)
{
    if (blendMode)  //only warp
        for (int i=0; i<i_numImage; ++i)
        {
//            alphaSlider[i]->setEnabled(false);
            warpImageButton[i]->setEnabled(true);
        }
    else    //blend sketches
        for (int i=0; i<i_numImage; ++i)
        {
//            alphaSlider[i]->setEnabled(true);
            warpImageButton[i]->setEnabled(false);
        }
}
