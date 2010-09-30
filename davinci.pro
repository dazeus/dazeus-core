#-------------------------------------------------
#
# Project created by QtCreator 2010-03-04T14:16:14
#
#-------------------------------------------------

QT       += network sql
QT       -= gui

TARGET    = davinci
CONFIG   += console libircclient-qt
CONFIG   -= app_bundle

TEMPLATE = app

SOURCES += main.cpp
SOURCES += davinci.cpp network.cpp config.cpp user.cpp server.cpp
HEADERS += davinci.h network.h config.h user.h server.h
# plugins
SOURCES += plugins/pluginmanager.cpp plugins/testplugin.cpp plugins/plugin.cpp
HEADERS += plugins/pluginmanager.h plugins/testplugin.h plugins/plugin.h

# perl implementation
QMAKE_PRE_LINK = "cd plugins/perl; sh make.sh;"
LIBS += plugins/perl/embedperl.a
