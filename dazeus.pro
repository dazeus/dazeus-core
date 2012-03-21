#-------------------------------------------------
#
# Project created by QtCreator 2010-03-04T14:16:14
#
#-------------------------------------------------

QT       += network sql
QT       -= gui

TARGET    = bin/dazeus
CONFIG   += console
CONFIG   -= app_bundle
VERSION   = 1.9.1

TEMPLATE = app
MOC_DIR = build
OBJECTS_DIR = build

SOURCES += src/main.cpp src/dazeus.cpp src/network.cpp src/config.cpp
SOURCES += src/user.cpp src/server.cpp src/database.cpp src/plugincomm.cpp
SOURCES += src/utils.cpp
HEADERS += src/dazeus.h src/network.h src/config.h src/user.h src/server.h
HEADERS += src/database.h src/plugincomm.h src/utils.h

INCLUDEPATH += src

LIBS += -lircclient -ljson -lmongo-client
QMAKE_CXXFLAGS += "-I/usr/include/libircclient -I/usr/include/libjson -I/usr/include/mongo-client -I/usr/include/glib-2.0"
