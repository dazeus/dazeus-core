/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#ifndef PERL_CALLBACKS_H
#define PERL_CALLBACKS_H

#ifdef __cplusplus
extern "C" {
#endif

void emoteEmbed(const char *network, const char *receiver, const char *message);
void privmsgEmbed(const char *network, const char *receiver, const char *message);
const char* getPropertyEmbed(const char *uniqueid, const char *variable);
void setPropertyEmbed(const char *uniqueid, const char *variable, const char *value);
void unsetPropertyEmbed(const char *uniqueid, const char *variable);
const char** getPropertyKeysEmbed(const char *uniqueid, const char *ns, int *length);
void sendWhoisEmbed(const char *uniqueid, const char *who);
void joinEmbed(const char *uniqueid, const char *channel);
void partEmbed(const char *uniqueid, const char *channel);
const char* getNickEmbed(const char *uniqueid);

#ifdef __cplusplus
}
#endif

#endif
