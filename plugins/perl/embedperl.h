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
    EmbedPerl(const char *uniqueid);
    ~EmbedPerl();

    bool loadModule(const char*);
    void message(const char*, const char*, const char*);
    void whois(const char*, int);
    void join(const char *channel, const char *who);
    void nick(const char *who, const char *new_nick);
    void connected();
    void namesReceived(const char *channel, const char *names);
    void tick();

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

    const char *uniqueid() const;

  private:
    PerlInterpreter *my_perl;
    char *uniqueid_;
    void init();
};

#endif
