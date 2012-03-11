/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#include "dazeus.h"
#include "dazeusglobal.h"

#include <libircclient.h>

int main(int argc, char *argv[])
{
    fprintf(stderr, "DaZeus version: %s\n", DAZEUS_VERSION_STR);
    unsigned int high, low;
    irc_get_version(&high, &low);
    fprintf(stdout, "IRC library version: %d.%02d\n", high, low);

    // TODO parse command-line options
    DaZeus d( "./dazeus.conf" );

    // find and initialise plugins
    d.initPlugins();

    // connect to servers marked as "autoconnect"
    d.autoConnect();

    // start the event loop
    d.run();
    return 0;
}
