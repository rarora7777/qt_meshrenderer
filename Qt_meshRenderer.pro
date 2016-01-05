#-------------------------------------------------
#
# Project created by QtCreator 2015-05-25T16:56:47
#
#-------------------------------------------------

QT       += core gui opengl


greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Qt_meshRenderer
TEMPLATE = app


SOURCES += edge.cpp node.cpp control.cpp mainwindow.cpp main.cpp \
    triangle.c

HEADERS  += triangle.h \
    mainwindow.h \
    control.h \
    node.h \
    edge.h

#LIBS += triangle.obj

#QMAKE_CFLAGS += -DTRILIBRARY

FORMS    += mainwindow.ui
