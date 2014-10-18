#-------------------------------------------------
#
# Project created by QtCreator 2014-10-07T22:33:57
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = qtimerge
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    imgmergeform.cpp \
    opendialog.cpp

HEADERS  += mainwindow.h \
    imgdiffbuffer.hpp \
    image.hpp \
    imgmergebuffer.hpp \
    imgmergeform.h \
    opendialog.h

FORMS    += mainwindow.ui \
    imgmergeform.ui \
    opendialog.ui

RESOURCES += \
    qtimerge.qrc
