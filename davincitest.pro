#-------------------------------------------------
#
# Project created by QtCreator 2010-03-04T14:16:14
#
#-------------------------------------------------

QT       += network sql testlib
QT       -= gui

TARGET   = davincitest
CONFIG   += qtestlib debug
CONFIG   -= app_bundle release
QMAKE_CXXFLAGS += -DDAVINCI_TESTS -O0 -g

TEMPLATE = app

SOURCES += main.cpp
SOURCES += davinci.cpp network.cpp config.cpp user.cpp server.cpp
HEADERS += davinci.h network.h config.h user.h server.h
# plugins
SOURCES += plugins/pluginmanager.cpp
HEADERS += plugins/pluginmanager.h
# tests
SOURCES += tests/testuser.cpp tests/testserver.cpp
HEADERS += tests/testuser.h tests/testserver.h
