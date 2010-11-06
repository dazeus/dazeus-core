/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include "davinci.h"
#include "davinciglobal.h"

#include <IrcGlobal>
#include <Irc>

#if IRC_VERSION == 0xffffff || IRC_VERSION == 0x000000
 #warning **WARNING**
 #warning You are using libircclient-qt trunk. Use at your own risk!
 #warning **WARNING**
#else
 #if IRC_VERSION < 0x000500
  #error This bot requires at least libircclient-qt 0.5.0.
 #endif
#endif

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    qDebug() << "DaVinci version: " << DAVINCI_VERSION_STR;
    qDebug() << "libircclient-qt version: " << IRC_VERSION_STR;
    qDebug() << "Irc::version: " << Irc::version();

    // TODO parse command-line options
    DaVinci d( "./davinci.conf" );

    // find and initialise plugins
    d.initPlugins();

    // connect to servers marked as "autoconnect"
    d.autoConnect();

    return a.exec();
}
