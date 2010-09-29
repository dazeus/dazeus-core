/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#include "embedperl.h"

extern "C" {
void xs_init(pTHX);

/***** CALBACKS *****/
void callbackEmbed( const char *s )
{
  printf("***Callback*** Embed! String=%s\n", s);
}

void emoteEmbed(const char *receiver, const char *message)
{
  printf("***Callback*** Send emote to %s: %s\n", receiver, message);
}

void privmsgEmbed(const char *receiver, const char *message)
{
  printf("***Callback*** Send privmsg to %s: %s\n", receiver, message);
}
/***** END CALLBACKS ****/
}

EmbedPerl::EmbedPerl()
{
  fprintf(stderr, "EmbedPerl constructor\n");
  perl = perl_alloc();
  perl_construct(perl);

  fprintf(stderr, "EmbedPerl constructor\n");

  char *argv[] = {"", "dazeus2_api_emulate.pl"};
  perl_parse(perl, xs_init, 1, argv, NULL);

  fprintf(stderr, "EmbedPerl constructor\n");

  init();
}

EmbedPerl::~EmbedPerl()
{
  fprintf(stderr, "EmbedPerl destructor\n");
  perl_destruct(perl);
  perl_free(perl);
}

void EmbedPerl::init()
{
  fprintf(stderr, "EmbedPerl::init()\n");
  PerlInterpreter *my_perl = perl;
  dSP;
  ENTER;
  SAVETMPS;
  PUSHMARK(SP);
  PUTBACK;
  call_pv("init", G_SCALAR);
  SPAGAIN;
  PUTBACK;
  FREETMPS;
  LEAVE;
}

void EmbedPerl::message(const char *from, const char *to, const char *msg)
{
  fprintf(stderr, "EmbedPerl::message(%s,%s,%s)\n", from, to, msg);
  PerlInterpreter *my_perl = perl;
  dSP;
  ENTER;
  SAVETMPS;
  PUSHMARK(SP);
  XPUSHs( newSVpv(from, strlen(from)) );
  XPUSHs( newSVpv(to,   strlen(to))   );
  XPUSHs( newSVpv(msg,  strlen(msg))  );
  XPUSHs( newSVpv(msg,  strlen(msg))  );
  PUTBACK;
  call_pv("message", G_SCALAR);
  SPAGAIN;
  int result = POPi;
  PUTBACK;
  FREETMPS;
  LEAVE;
  fprintf(stderr, "... message(...)=%d\n", result);
}

void EmbedPerl::callEcho(const char *str)
{
  fprintf(stderr, "EmbedPerl::callEcho(%s)\n", str);
  PerlInterpreter *my_perl = perl;
  dSP;
  ENTER;
  SAVETMPS;
  PUSHMARK(SP);
  XPUSHs( newSVpv(str, strlen(str)) );
  PUTBACK;
  call_pv("echo2", G_SCALAR);
  SPAGAIN;
  SV *resultsv = POPs;
  STRLEN s_len;
  const char *text = SvPV(resultsv, s_len);
  PUTBACK;
  FREETMPS;
  LEAVE;
  printf("Het resultaat is: %s\n", text);
}

bool EmbedPerl::loadModule(const char *module)
{
  fprintf(stderr, "EmbedPerl::loadModule(%s)", module);
  PerlInterpreter *my_perl = perl;
  dSP;
  ENTER;
  SAVETMPS;
  PUSHMARK(SP);
  XPUSHs( newSVpv(module, strlen(module)) );
  PUTBACK;
  call_pv("loadModule", G_SCALAR);
  SPAGAIN;
  int result = POPi;
  PUTBACK;
  FREETMPS;
  LEAVE;
  fprintf(stderr, "=%d\n", result);
  return result != 0;
}
