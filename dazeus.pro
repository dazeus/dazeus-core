#-------------------------------------------------
#
# Project created by QtCreator 2010-03-04T14:16:14
#
#-------------------------------------------------

QT       += network sql
QT       -= gui

TARGET    = data/dazeus
CONFIG   += console
CONFIG   -= app_bundle
VERSION   = 1.9.1

TEMPLATE = app
MOC_DIR = build
OBJECTS_DIR = build

SOURCES += src/main.cpp src/dazeus.cpp src/network.cpp src/config.cpp
SOURCES += src/user.cpp src/server.cpp src/database.cpp
HEADERS += src/dazeus.h src/network.h src/config.h src/user.h src/server.h
HEADERS += src/server_p.h src/database.h
# plugins
SOURCES += src/plugins/pluginmanager.cpp src/plugins/testplugin.cpp
SOURCES += src/plugins/plugin.cpp src/plugins/statistics.cpp
SOURCES += src/plugins/karmaplugin.cpp src/plugins/socketplugin.cpp
HEADERS += src/plugins/pluginmanager.h src/plugins/testplugin.h
HEADERS += src/plugins/plugin.h src/plugins/statistics.h
HEADERS += src/plugins/karmaplugin.h src/plugins/socketplugin.h

INCLUDEPATH += src

LIBS += -lircclient -ljson
QMAKE_CXXFLAGS += "-I/usr/include/libircclient -I/usr/include/libjson"
