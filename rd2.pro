#-------------------------------------------------
#
# Project created by QtCreator 2013-08-19T23:07:08
#
#-------------------------------------------------

QT       += core gui

QMAKE_CXXFLAGS += -std=c++0x

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/release/ -lqextserialport
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/debug/ -lqextserialport
else:unix: LIBS += -L$$OUT_PWD/../build-qextserialport-qt_5_1-Debug/ -lQt5ExtSerialPort

INCLUDEPATH += $$PWD/../qextserialport/src
DEPENDPATH += $$PWD/../qextserialport/

TARGET = rd2
TEMPLATE = app

SOURCES += main.cpp\
        RayDisplayWindow.cpp \
    RayDisplayScene.cpp \
    CommunicationThread.cpp

HEADERS  += RayDisplayWindow.h \
    RayDisplayScene.h \
    CommunicationThread.h

FORMS    += RayDisplayWindow.ui
