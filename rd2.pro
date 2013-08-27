#-------------------------------------------------
#
# Project created by QtCreator 2013-08-19T23:07:08
#
#-------------------------------------------------

QT       += core gui serialport

QMAKE_CXXFLAGS += -std=c++0x

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = rd2
TEMPLATE = app

SOURCES += main.cpp\
        RayDisplayWindow.cpp \
    RayDisplayScene.cpp \
    CommunicationsThread.cpp

HEADERS  += RayDisplayWindow.h \
    RayDisplayScene.h \
    CommunicationsThread.h

FORMS    += RayDisplayWindow.ui
