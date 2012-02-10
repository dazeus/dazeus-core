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

#ifndef LIBDAZEUS_H
#define LIBDAZEUS_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define LIBDAZEUS_VERSION_STR "2.0-beta1"
#define LIBDAZEUS_VERSION 0x010901

#define LIBDAZEUS_READAHEAD_SIZE 512
typedef struct dazeus_struct
{
	char *socketfile;
	int socket;
	char *error;
	char readahead[LIBDAZEUS_READAHEAD_SIZE];
	unsigned int readahead_size;
} dazeus;

typedef struct dazeus_stringlist_struct
{
	char *value;
	struct dazeus_stringlist_struct *next;
} dazeus_stringlist;

#define DAZEUS_GLOBAL_SCOPE 0x00
#define DAZEUS_NETWORK_SCOPE 0x01
#define DAZEUS_RECEIVER_SCOPE 0x03
#define DAZEUS_SENDER_SCOPE 0x07

typedef struct dazeus_scope_struct
{
	unsigned int scope_type;
	char *network;
	char *receiver;
	char *sender;
} dazeus_scope;

/**
 * Variable scope variable constructors. Don't forget to clean them up with
 * libdazeus_scope_free() too!
 */
dazeus_scope *libdazeus_scope_global();
dazeus_scope *libdazeus_scope_network(const char *network);
dazeus_scope *libdazeus_scope_receiver(const char *network, const char *receiver);
dazeus_scope *libdazeus_scope_sender(const char *network, const char *receiver, const char *sender);
void libdazeus_scope_free(dazeus_scope*);

/**
 * Create a new libdazeus instance.
 */
dazeus *libdazeus_create();

/**
 * Retrieve the last error generated or NULL if there was no error.
 */
const char *libdazeus_error(dazeus*);

/**
 * Connect this dazeus instance. Takes socket location as its only parameter.
 * Socket can be either of the form "unix:/path/to/unix/socket" or
 * "tcp:hostname:port" (including "tcp:127.0.0.1:1234").
 */
int libdazeus_open(dazeus*, const char *socketfile);

/**
 * Clean up a dazeus instance.
 */
void libdazeus_close(dazeus*);

/**
 * Returns a linked list of networks on this DaZeus instance, or NULL if
 * an error occured. Remember to free the returned structure with
 * libdazeus_networks_free().
 */
dazeus_stringlist *libdazeus_networks(dazeus*);

/**
 * Clean up memory allocated by earlier stringlist functions.
 */
void libdazeus_stringlist_free(dazeus_stringlist*);

/**
 * Returns a linked list of channels on this DaZeus instance, or NULL if
 * an error occured. Remember to free the returned structure with
 * libdazeus_stringlist_free().
 */
dazeus_stringlist *libdazeus_channels(dazeus*, const char *network);

/**
 * Retrieve the value of a variable in the DaZeus 2 database. Remember to
 * free() the returned variable after use.
 */
char *libdazeus_get_property(dazeus*, const char *variable, dazeus_scope*);

/**
 * Set the value of a variable in the DaZeus 2 database.
 */
int libdazeus_set_property(dazeus*, const char *variable, const char *value, dazeus_scope*);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
