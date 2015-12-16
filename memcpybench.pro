#-------------------------------------------------
#
# Project created by QtCreator 2015-12-11T14:46:25
#
#-------------------------------------------------

QT       -= core
QT       -= gui

TARGET = memcpybench
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

CONFIG += C++11

SOURCES += main.cpp \
    memcpy_64.S

OTHER_FILES += \
    memcpy_64.S
