/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#ifndef EMBEDPERL_H
#define EMBEDPERL_H

#include <EXTERN.h>
#include <perl.h>

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
