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
#include <sys/uio.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>

#include <libjson.h>

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

int _send(dazeus *d, JSONNODE *node) {
	_connect(d);
	if(!d->socket) {
		return -1;
	}
	json_char *raw_json = json_write(node);
	// TODO is json_char char or wchar_t here?
	// what happens with size if it is wchar?
	int size = strlen(raw_json);
	dprintf(d->socket, "%d%s\n", size, raw_json);
	json_free(raw_json);
	return size;
}

// Don't forget to free the JSONNODE returned via 'result'
// using json_free()!
int _readPacket(dazeus *d, int timeout, JSONNODE **result) {
	// TODO implement timeout well in read()
	assert(result != NULL);
	*result = 0;
	time_t starttime = time(NULL);
	int once = timeout == 0;
	_connect(d);
	if(!d->socket) {
		return -1;
	}
	while(timeout < 0 || once == 1 || time(NULL) <= starttime + timeout) {
		once = 0;

		// First, append the buffer by as much as we can
		char buf[LIBDAZEUS_READAHEAD_SIZE];
		int max_size = LIBDAZEUS_READAHEAD_SIZE - d->readahead_size;
		ssize_t r = read(d->socket, buf, max_size);
		if(r < 0) {
			d->error = strerror(errno);
			return -1;
		}
		memcpy(d->readahead + d->readahead_size, buf, r);
		d->readahead_size += r;

		// Now, try reading a packet out of this
		// First the size, then the JSON data
		memset(buf, 0, LIBDAZEUS_READAHEAD_SIZE);
		char *jsonptr = buf;
		uint16_t i;
		for(i = 0; i < d->readahead_size; ++i) {
			char c = (d->readahead)[i];
			if(isdigit(c)) {
				jsonptr[0] = c;
				++jsonptr;
			} else if(c == '{') {
				break;
			} // skip all other characters
		}
		jsonptr[0] = 0;
		int json_size = atoi(buf);
		if(json_size >= LIBDAZEUS_READAHEAD_SIZE) {
			// We can't fit a packet of this size in our buffers,
			// so close the connection
			// TODO: implement growing buffers
			close(d->socket);
			d->socket = 0;
			d->readahead_size = 0;
			d->error = "JSON packet size is too large";
			return -1;
		}
		if(d->readahead_size >= i + json_size) {
			// enough data available to read a packet!
			// first, move it into our own buffer
			memcpy(buf, d->readahead + i, json_size);
			buf[json_size] = 0;
			// then, shrink the saved buffer
			memmove(d->readahead, d->readahead + i + json_size, d->readahead_size - i - json_size);
			d->readahead_size -= i + json_size;

			JSONNODE *message = json_parse(buf);
			if(message == NULL) {
				close(d->socket);
				d->socket = 0;
				d->readahead_size = 0;
				d->error = "Invalid JSON data sent to socket";
				return -1;
			}
			*result = message;
			return json_size;
		}
	}
	return 0;
}

// Read incoming JSON requests until a non-event comes in (blocking)
int _read(dazeus *d, JSONNODE **result) {
	assert(result != NULL);
	*result = 0;
	while(1) {
		JSONNODE *res = 0;
		if(_readPacket(d, -1, &res) <= 0) {
			// read error
			return 0;
		}
		if(res != NULL) {
			// TODO don't assume it's not an event
			*result = res;
			return 1;
		}
	}
}

int _check_success(dazeus *d, JSONNODE *response) {
	JSONNODE *success = json_pop_back(response, "success");
	if(success == NULL) {
		d->error = "No 'success' field in response.";
		return 0;
	}
	json_char *succval = json_as_string(success);
	if(strcmp(succval, "true") != 0) {
		d->error = "Request failed, no error";
		JSONNODE *error = json_pop_back(response, "error");
		if(error) {
			d->error = json_as_string(error);
		}
		json_free(error);
		json_free(succval);
		json_free(success);
		return 0;
	}
	json_free(succval);
	json_free(success);
	return 1;
}

dazeus_stringlist *_jsonarray_to_stringlist(JSONNODE *array) {
	if(json_type(array) != JSON_ARRAY)
		return NULL;

	JSONNODE_ITERATOR it = json_begin(array);
	dazeus_stringlist *first = 0;
	dazeus_stringlist *cur = first;
	while(it != json_end(array)) {
		JSONNODE *item = *it;

		dazeus_stringlist *new = malloc(sizeof(dazeus_stringlist));
		json_char *value = json_as_string(item);
		new->value = malloc(strlen(value) + 1);
		strcpy(new->value, value);
		json_free(value);
		new->next = 0;
		if(first == 0) {
			first = new;
			cur = new;
		} else {
			cur->next = new;
			cur = new;
		}
		it++;
	}
	return first;
}

int _add_scope(dazeus *d, dazeus_scope *s, JSONNODE *params) {
	assert(json_type(params) == JSON_ARRAY);

	if(s->scope_type == DAZEUS_GLOBAL_SCOPE) {
		return 1; // don't add anything
	}
	if(s->scope_type != DAZEUS_NETWORK_SCOPE
	&& s->scope_type != DAZEUS_RECEIVER_SCOPE
	&& s->scope_type != DAZEUS_SENDER_SCOPE) {
		d->error = "Scope parameter was incorrect";
		return 0;
	}
	assert(s->scope_type & DAZEUS_NETWORK_SCOPE);

	JSONNODE *netp = json_new_a("", s->network);
	json_push_back(params, netp);

	if(s->scope_type & DAZEUS_RECEIVER_SCOPE) {
		JSONNODE *recvp = json_new_a("", s->receiver);
		json_push_back(params, recvp);
		if(s->scope_type & DAZEUS_SENDER_SCOPE) {
			JSONNODE *sendp = json_new_a("", s->sender);
			json_push_back(params, sendp);
		}
	} else {
		assert((s->scope_type & DAZEUS_SENDER_SCOPE) == 0);
	}
	return 1;
}

dazeus_scope *libdazeus_scope_global()
{
	dazeus_scope *s = malloc(sizeof(dazeus_scope));
	s->scope_type = DAZEUS_GLOBAL_SCOPE;
	s->network = 0;
	s->receiver = 0;
	s->sender = 0;
	return s;
}

dazeus_scope *libdazeus_scope_network(const char *network)
{
	dazeus_scope *s = libdazeus_scope_global();
	s->scope_type = DAZEUS_NETWORK_SCOPE;
	s->network = strdup(network);
	return s;
}

dazeus_scope *libdazeus_scope_receiver(const char *network, const char *receiver)
{
	dazeus_scope *s = libdazeus_scope_network(network);
	s->scope_type = DAZEUS_RECEIVER_SCOPE;
	s->receiver = strdup(receiver);
	return s;
}

dazeus_scope *libdazeus_scope_sender(const char *network, const char *receiver, const char *sender)
{
	dazeus_scope *s = libdazeus_scope_receiver(network, receiver);
	s->scope_type = DAZEUS_SENDER_SCOPE;
	s->sender = strdup(receiver);
	return s;
}

void libdazeus_scope_free(dazeus_scope *s)
{
	free(s->network);
	free(s->receiver);
	free(s->sender);
	free(s);
}

/**
 * Create a new libdazeus instance.
 */
dazeus *libdazeus_create() {
	dazeus *ret = malloc(sizeof(dazeus));
	ret->socketfile = 0;
	ret->socket = 0;
	ret->error = 0;
	ret->readahead_size = 0;
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
	close(d->socket);
	free(d->socketfile);
	free(d);
}

/**
 * Returns a linked list of networks on this DaZeus instance, or NULL if
 * an error occured. Remember to free the returned structure with
 * libdazeus_stringlist_free().
 */
dazeus_stringlist *libdazeus_networks(dazeus *d)
{
	// {'get':'networks'}
	JSONNODE *fulljson = json_new(JSON_NODE);
	JSONNODE *request  = json_new_a("get", "networks");
	json_push_back(fulljson,request);
	_send(d, fulljson);
	json_free(request);
	json_free(fulljson);

	JSONNODE *response;
	if(!_read(d, &response)) {
		return NULL;
	}

	if(!_check_success(d, response)) {
		json_free(response);
		return NULL;
	}

	JSONNODE *networks = json_pop_back(response, "networks");
	if(networks == NULL) {
		json_free(response);
		d->error = "No networks present in reply";
		return NULL;
	}
	dazeus_stringlist *list = _jsonarray_to_stringlist(networks);
	if(list == NULL) {
		json_free(networks);
		json_free(response);
		d->error = "Networks in reply wasn't an array";
		return NULL;
	}

	json_free(networks);
	json_free(response);
	return list;
}

/**
 * Returns a linked list of channels on this DaZeus instance, or NULL if
 * an error occured. Remember to free the returned structure with
 * libdazeus_stringlist_free().
 */
dazeus_stringlist *libdazeus_channels(dazeus *d, const char *network)
{
	// {'get':'channels','params':['networkname']}
	JSONNODE *fulljson = json_new(JSON_NODE);
	JSONNODE *request  = json_new_a("get", "channels");
	JSONNODE *params   = json_new(JSON_ARRAY);
	json_set_name(params, "params");
	JSONNODE *param1   = json_new_a("", network);
	json_push_back(params, param1);
	json_push_back(fulljson, request);
	json_push_back(fulljson, params);
	_send(d, fulljson);
	json_free(fulljson);

	JSONNODE *response;
	if(!_read(d, &response)) {
		return NULL;
	}

	if(!_check_success(d, response)) {
		json_free(response);
		return NULL;
	}

	JSONNODE *channels = json_pop_back(response, "channels");
	if(channels == NULL) {
		json_free(response);
		d->error = "Reply didn't have channels";
		return NULL;
	}
	dazeus_stringlist *list = _jsonarray_to_stringlist(channels);
	if(list == NULL) {
		json_free(channels);
		json_free(response);
		d->error = "Channels in reply wasn't an array";
		return NULL;
	}

	json_free(channels);
	json_free(response);
	return list;
}

/**
 * Clean up memory allocated by earlier stringlist functions.
 */
void libdazeus_stringlist_free(dazeus_stringlist *n)
{
	while(n != 0) {
		dazeus_stringlist *next = n->next;
		free(n->value);
		free(n);
		n = next;
	}
}

/**
 * Retrieve the value of a variable in the DaZeus 2 database. Remember to
 * free() the returned variable after use.
 */
char *libdazeus_get_property(dazeus *d, const char *variable, dazeus_scope *s)
{
	// {'do':'property','params':['get','variable',...scope...]}
	JSONNODE *fulljson = json_new(JSON_NODE);
	JSONNODE *request  = json_new_a("do", "property");
	JSONNODE *params   = json_new(JSON_ARRAY);
	JSONNODE *p1       = json_new_a("", "get");
	JSONNODE *p2       = json_new_a("", variable);
	json_set_name(params, "params");
	json_push_back(params, p1);
	json_push_back(params, p2);
	if(!_add_scope(d, s, params)) {
		json_free(fulljson);
		json_free(request);
		json_free(params);
		return NULL;
	}
	json_push_back(fulljson, request);
	json_push_back(fulljson, params);
	_send(d, fulljson);
	json_free(fulljson);

	JSONNODE *response;
	if(!_read(d, &response)) {
		return NULL;
	}

	if(!_check_success(d, response)) {
		json_free(response);
		return NULL;
	}

	JSONNODE *value = json_pop_back(response, "value");
	char *ret = NULL;
	if(value) {
		json_char *str = json_as_string(value);
		ret = malloc(strlen(str) + 1);
		strcpy(ret, str);
		json_free(str);
	}

	json_free(value);
	json_free(response);
	return ret;
}

/**
 * Set the value of a variable in the DaZeus 2 database.
 */
int libdazeus_set_property(dazeus *d, const char *variable, const char *value, dazeus_scope *s)
{
	// {'do':'property','params':['set','variable','value',...scope...]}
	JSONNODE *fulljson = json_new(JSON_NODE);
	JSONNODE *request  = json_new_a("do", "property");
	JSONNODE *params   = json_new(JSON_ARRAY);
	JSONNODE *p1       = json_new_a("", "set");
	JSONNODE *p2       = json_new_a("", variable);
	JSONNODE *p3       = json_new_a("", value);
	json_set_name(params, "params");
	json_push_back(params, p1);
	json_push_back(params, p2);
	json_push_back(params, p3);
	if(!_add_scope(d, s, params)) {
		json_free(fulljson);
		json_free(request);
		json_free(params);
		return 0;
	}
	json_push_back(fulljson, request);
	json_push_back(fulljson, params);
	_send(d, fulljson);
	json_free(fulljson);

	JSONNODE *response;
	if(!_read(d, &response)) {
		return 0;
	}

	if(!_check_success(d, response)) {
		json_free(response);
		return 0;
	}

	json_free(response);
	return 1;
}

