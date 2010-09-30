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

    void callEcho(const char*);
    bool loadModule(const char*);
    void message(const char*, const char*, const char*);

    void setCallbacks( void (*emoteCallback)  (const char*, const char*, const char*, void*),
                       void (*privmsgCallback)(const char*, const char*, const char*, void*),
                       void *data );

    void (*emoteCallback)(const char*, const char*, const char*, void*);
    void (*privmsgCallback)(const char*, const char*, const char*, void*);
    void *data;

    const char *uniqueid() const;

  private:
    PerlInterpreter *perl;
    char *uniqueid_;
    void init();
};

#endif
