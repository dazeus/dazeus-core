/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#include "dazeus.h"
#include "dazeusglobal.h"

#include <time.h>
#include <stdio.h>
#include <string.h>

void usage(char*);

int main(int argc, char *argv[])
{
	fprintf(stderr, "DaZeus version: %s\n", DAZEUS_VERSION_STR);

	// Initialise random seed
	srand(time(NULL));

	std::string configfile = DAZEUS_DEFAULT_CONFIGFILE;
	std::vector<std::string> sockets;

	std::string type;
	for(int i = 1; i < argc; ++i) {
		if(type.length() == 0) {
			if(strcmp(argv[i], "-h") == 0) {
				usage(argv[0]);
				return 1;
			} else if(strcmp(argv[i], "-s") == 0) {
				type = "socket";
			} else if(strcmp(argv[i], "-c") == 0) {
				type = "config";
			} else {
				fprintf(stderr, "Didn't understand option: %s\n", argv[i]);
				usage(argv[0]);
				return 2;
			}
		} else {
			if(type == "socket") {
				sockets.push_back(std::string(argv[i]));
			} else if(type == "config") {
				configfile = std::string(argv[i]);
			} else {
				abort();
			}
			type = "";
		}
	}

	DaZeus d(configfile);
	if(!d.loadConfig()) {
		fprintf(stderr, "Failed to load configuration, bailing out.\n");
		return 3;
	}
	d.addAdditionalSockets(sockets);
	d.initPlugins();
	d.autoConnect();
	d.run();
	return 0;
}

void usage(char *exec)
{
	fprintf(stderr, "Usage: %s options\n", exec);
	fprintf(stderr, "Available options:\n");
	fprintf(stderr, "  -c configfile  - Use this configuration file\n");
	fprintf(stderr, "  -s socket      - Listen on this socket too\n");
	fprintf(stderr, "  -h             - Display this help message\n");
}
