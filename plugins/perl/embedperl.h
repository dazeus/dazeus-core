/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#ifndef EMBEDPERL_H
#define EMBEDPERL_H

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

    void callEcho(const char*);
    bool loadModule(const char*);
    void message(const char*, const char*, const char*);

  private:
    PerlInterpreter *perl;
    void init();
};

#endif
