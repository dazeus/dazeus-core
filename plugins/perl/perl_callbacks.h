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

#ifdef __cplusplus
}
#endif

#endif
