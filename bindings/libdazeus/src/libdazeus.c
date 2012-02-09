/**
 * LibDazeus -- a library that implements the DaZeus 2 Plugin Protocol
 * 
 * Copyright (c) Sjors Gielen, 2010-2012
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the DaVinci or DaZeus team nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SJORS GIELEN OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "libdazeus.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>

void _connect(dazeus *d)
{
	if(d->socket != 0) {
		// TODO check if it's not in error state
		return;
	}

	// TODO actually connect
	if(strncmp(d->socketfile, "unix:", 5) == 0) {
		struct sockaddr_un addr;
		// Open a UNIX socket
		d->socket = socket(AF_UNIX, SOCK_STREAM, 0);
		if(d->socket <= 0) {
			d->socket = 0;
			d->error = strerror(errno);
			return;
		}
		addr.sun_family = AF_UNIX;
		strcpy(addr.sun_path, d->socketfile + 5);
		if(connect(d->socket, (struct sockaddr*) &addr,
		  sizeof(struct sockaddr_un)) < 0) {
			close(d->socket);
			d->socket = 0;
			d->error = strerror(errno);
			return;
		}
	} else if(strncmp(d->socketfile, "tcp:", 4) == 0) {
		// Open a TCP socket
		// tcp:1.2.3.4:1234
		// tcp:dazeus.example.org:1234
		char *colon = strstr(d->socketfile + 4, ":");
		if(colon == NULL) {
			d->error = "No colon found in TCP specification";
			return;
		}
		char *portstr = colon + 1;
		int hostlen = colon - d->socketfile;
		char *hoststr = malloc(hostlen + 1);
		strncpy(hoststr, d->socketfile, hostlen);
		hoststr[hostlen + 1] = 0;

		struct addrinfo *hints, *result;
		hints = calloc(1, sizeof(struct addrinfo));
		hints->ai_flags = 0;
		hints->ai_family = AF_INET;
		hints->ai_socktype = SOCK_STREAM;
		hints->ai_protocol = 0;

		int s = getaddrinfo(hoststr, portstr, hints, &result);
		free(hints);
		hints = 0;
		if(s != 0) {
			d->error = (char*) gai_strerror(s);
			free(hoststr);
			return;
		}

		d->socket = socket(AF_INET, SOCK_STREAM, 0);
		if(d->socket <= 0) {
			d->socket = 0;
			d->error = strerror(errno);
			free(hoststr);
			freeaddrinfo(result);
			return;
		}
		if(connect(d->socket, result->ai_addr, result->ai_addrlen) < 0) {
			close(d->socket);
			d->socket = 0;
			d->error = strerror(errno);
			free(hoststr);
			freeaddrinfo(result);
			return;
		}
		free(hoststr);
		freeaddrinfo(result);
	} else {
		d->error = "Didn't understand format of socketfile -- does it begin with unix: or tcp:?";
		return;
	}

	assert(d->socket);
}

/**
 * Create a new libdazeus instance.
 */
dazeus *libdazeus_create() {
	dazeus *ret = malloc(sizeof(dazeus));
	ret->socketfile = 0;
	ret->socket = 0;
	ret->error = 0;
	return ret;
}

/**
 * Retrieve the last error generated or NULL if there was no error.
 */
const char *libdazeus_error(dazeus *d)
{
	return d->error;
}

/**
 * Connect this dazeus instance. Takes socket location as its only parameter.
 * Socket can be either of the form "unix:/path/to/unix/socket" or
 * "tcp:hostname:port" (including "tcp:127.0.0.1:1234").
 */
int libdazeus_open(dazeus *d, const char *socketfile)
{
	if(d->socket) {
		close(d->socket);
	}
	free(d->socketfile);
	d->socketfile = malloc(strlen(socketfile) + 1);
	strcpy(d->socketfile, socketfile);
	_connect(d);
	return d->socket != 0;
}

/**
 * Clean up a dazeus instance.
 */
void libdazeus_close(dazeus *d)
{
	free(d->socketfile);
	free(d);
}

/**
 * Returns a linked list of networks on this DaZeus instance, or NULL if
 * an error occured. Remember to free the returned structure with
 * libdazeus_networks_free().
 */
dazeus_network *libdazeus_networks(dazeus *d)
{
	// {'get':'networks'}
	dazeus_network *n1 = malloc(sizeof(dazeus_network));
	dazeus_network *n2 = malloc(sizeof(dazeus_network));
	dazeus_network *n3 = malloc(sizeof(dazeus_network));
	n1->network_name = "freenode";
	n1->next = n2;
	n2->network_name = "kassala";
	n2->next = n3;
	n3->network_name = "oftc";
	n3->next = 0;

	return n1;
}

/**
 * Clean up memory allocated by libdazeus_networks().
 */
void libdazeus_networks_free(dazeus_network *n)
{
	while(n != 0) {
		dazeus_network *next = n->next;
		free(n);
		n = next;
	}
}
