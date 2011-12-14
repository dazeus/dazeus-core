/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include "davinci.h"
#include "davinciglobal.h"

#include <libircclient.h>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    qDebug() << "DaVinci version: " << DAVINCI_VERSION_STR;
    unsigned int high, low;
    irc_get_version(&high, &low);
    fprintf(stdout, "IRC library version: %d.%02d\n", high, low);

    // TODO parse command-line options
    DaVinci d( QLatin1String("./davinci.conf") );

    // find and initialise plugins
    d.initPlugins();

    // connect to servers marked as "autoconnect"
    d.autoConnect();

    return a.exec();
}
