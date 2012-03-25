/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#include "dazeus.h"
#include "dazeusglobal.h"

#include <libircclient.h>
#include <time.h>

int main(int argc, char *argv[])
{
	fprintf(stderr, "DaZeus version: %s\n", DAZEUS_VERSION_STR);
	unsigned int high, low;
	irc_get_version(&high, &low);
	fprintf(stdout, "IRC library version: %d.%02d\n", high, low);

	// Initialise random seed
	srand(time(NULL));

	DaZeus d( "./dazeus.conf" );
	if(!d.loadConfig()) {
		fprintf(stderr, "Failed to load configuration, bailing out.\n");
		return 3;
	}
	d.initPlugins();
	d.autoConnect();
	d.run();
	return 0;
}
