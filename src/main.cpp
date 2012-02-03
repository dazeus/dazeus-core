/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include "dazeus.h"
#include "dazeusglobal.h"

#include <libircclient.h>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    qDebug() << "DaZeus version: " << DAZEUS_VERSION_STR;
    unsigned int high, low;
    irc_get_version(&high, &low);
    fprintf(stdout, "IRC library version: %d.%02d\n", high, low);

    // TODO parse command-line options
    DaZeus d( QLatin1String("./dazeus.conf") );

    // find and initialise plugins
    d.initPlugins();

    // connect to servers marked as "autoconnect"
    d.autoConnect();

    return a.exec();
}
