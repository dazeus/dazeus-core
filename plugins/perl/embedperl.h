/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#ifndef EMBEDPERL_H
#define EMBEDPERL_H

#include "perl_callbacks.h"

#ifdef INSIDE_PERLEMBED
// Need to be included here for some reason.
# include <EXTERN.h>
# include <perl.h>
#else
class PerlInterpreter;
#endif

class EmbedPerl
{
  public:
    EmbedPerl();
    ~EmbedPerl();

    bool isInitialized() { return my_perl != 0; }

    void init(const char*);
    bool loadModule(const char*, const char*);
    void message(const char*, const char*, const char*, const char*);
    void whois(const char*, const char*, int);
    void join(const char*, const char *channel, const char *who);
    void nick(const char*, const char *who, const char *new_nick);
    void connected(const char*);
    void namesReceived(const char*, const char *channel, const char *names);
    void tick(const char*);

    void setCallbacks( void (*emoteCallback)  (const char*, const char*, const char*, void*),
                       void (*privmsgCallback)(const char*, const char*, const char*, void*),
                       const char* (*getPropertyCallback)(const char*, const char*, void*),
                       void (*setPropertyCallback)(const char*, const char*, const char*, void*),
                       void (*unsetPropertyCallback)(const char*, const char*, void*),
                       void (*sendWhoisCallback)(const char*, void*),
                       void (*joinCallback)(const char*, void*),
                       void (*partCallback)(const char*, void*),
                       const char* (*getNickCallback)(void*),
                       void *data );

    void (*emoteCallback)(const char*, const char*, const char*, void*);
    void (*privmsgCallback)(const char*, const char*, const char*, void*);
    const char* (*getPropertyCallback)(const char*, const char*, void*);
    void (*setPropertyCallback)(const char*, const char*, const char*, void*);
    void (*unsetPropertyCallback)(const char*, const char*, void*);
    void (*sendWhoisCallback)(const char*, void*);
    void (*joinCallback)(const char*, void*);
    void (*partCallback)(const char*, void*);
    const char* (*getNickCallback)(void*);
    void *data;

  private:
    PerlInterpreter *my_perl;
};

#endif
