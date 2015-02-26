/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#include "dazeus.h"
#include "dazeusglobal.h"

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <assert.h>

void usage(char*);

dazeus::DaZeus *d = NULL;

void sigchild_handler(int sig);
void sigchild_handler(int sig) {
	assert(sig == SIGCHLD);
	assert(d != NULL);
	d->sigchild();

	if(signal(sig, sigchild_handler) == SIG_ERR) {
		perror("Failed to install SIGCHLD handler");
	}
}

void sighup_handler(int sig);
void sighup_handler(int sig) {
	assert(sig == SIGHUP);
	assert(d != NULL);
	d->reloadConfig();

	if(signal(sig, sighup_handler) == SIG_ERR) {
		perror("Failed to install SIGHUP handler");
	}
}

int main(int argc, char *argv[])
{
	fprintf(stderr, "DaZeus version: %s\n", DAZEUS_VERSION_STR);

	// Initialise random seed
	srand(time(NULL));

	std::string configfile = DAZEUS_DEFAULT_CONFIGFILE;

	std::string type;
	for(int i = 1; i < argc; ++i) {
		if(type.length() == 0) {
			if(strcmp(argv[i], "-h") == 0) {
				usage(argv[0]);
				return 1;
			} else if(strcmp(argv[i], "-c") == 0) {
				type = "config";
			} else {
				fprintf(stderr, "Didn't understand option: %s\n", argv[i]);
				usage(argv[0]);
				return 2;
			}
		} else {
			if(type == "config") {
				configfile = std::string(argv[i]);
			} else {
				abort();
			}
			type = "";
		}
	}

	if(signal(SIGCHLD, sigchild_handler) == SIG_ERR) {
		perror("Failed to install SIGCHLD handler");
	}
	if(signal(SIGHUP, sighup_handler) == SIG_ERR) {
		perror("Failed to install SIGHUP handler");
	}

	if(signal(SIGPIPE, SIG_IGN)) {
		perror("Failed to set SIGPIPE ignore");
	}

	d = new dazeus::DaZeus(configfile);
	d->run();
	delete d;
	return 0;
}

void usage(char *exec)
{
	fprintf(stderr, "Usage: %s options\n", exec);
	fprintf(stderr, "Available options:\n");
	fprintf(stderr, "  -c configfile  - Use this configuration file\n");
	fprintf(stderr, "  -h             - Display this help message\n");
}
