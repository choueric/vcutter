#-------------------------------------------------
#
# Project created by QtCreator 2017-10-10T11:45:11
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = vcutter
TEMPLATE = app


SOURCES +=\
    cuttime.cpp \
    vcutter.cpp \
    log.cpp \
    main.cpp

HEADERS  += \
    cuttime.h \
    vcutter.h \
    log.h \
    config.h

FORMS += \
    vcutter.ui
