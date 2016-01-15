#ifndef NEWCONTROL_H
#define NEWCONTROL_H

#include <QtWidgets>
#include "mainwindow.h"
#include <QSlider>
#include <QMainWindow>

class NewControlWindow : public QMainWindow
{
    Q_OBJECT
public:
    NewControlWindow(WarpWindow *window);
    ~NewControlWindow();
    void initialize(int numImage, std::string imFiles[]);

public slots:
//    void sliderMoved(int, int);

signals:
//    void alphaChanged(float);
//    void blendModeChanged(bool);
//    void warpImageChanged(int);

private:
    QSlider **alphaSlider;
    QRadioButton **warpImageButton;
    QRadioButton blendModeButton[2];
    QButtonGroup blendModeGroup;
    QButtonGroup warpImageGroup;
    QPixmap **thumbs;
    QGridLayout grid;

    float sliderWidth;
    float thumbWidth;
    float thumbHeight;
    float sliderHeight;
    float radioHeight;

    WarpWindow *meshRenderer;

    void createButton(int numImage);
    void createSliders(int numImage);
    void createThumbs(std::string *imFiles, int numImage);
    void NewControlWindow::createWarpImageSelectButtons(int numImage);
};

#endif // NEWCONTROL_H

