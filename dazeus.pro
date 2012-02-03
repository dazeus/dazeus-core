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

SOURCES += main.cpp
SOURCES += dazeus.cpp network.cpp config.cpp user.cpp server.cpp database.cpp
HEADERS += dazeus.h network.h config.h user.h server.h server_p.h database.h
# plugins
SOURCES += plugins/pluginmanager.cpp plugins/testplugin.cpp plugins/plugin.cpp
HEADERS += plugins/pluginmanager.h plugins/testplugin.h plugins/plugin.h
SOURCES += plugins/statistics.cpp plugins/karmaplugin.cpp plugins/socketplugin.cpp
HEADERS += plugins/statistics.h plugins/karmaplugin.h plugins/socketplugin.h

LIBS += -lircclient -ljson
QMAKE_CXXFLAGS += "-I/usr/include/libircclient -I/usr/include/libjson"
