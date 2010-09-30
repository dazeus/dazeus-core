/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#include "perl_callbacks.h"
#include "embedperl.h"
#include <assert.h>

#define MAX_EMBED_PERL 10

// Totally thread-unsafe...
static EmbedPerl *ePerl[MAX_EMBED_PERL];
static int numEmbedPerl = 0;

extern "C" {
void xs_init(pTHX);

/***** CALBACKS *****/
void emoteEmbed(const char *uniqueid, const char *receiver, const char *message)
{
  printf("***Callback*** Send emote to %s (on %s): %s\n", receiver, uniqueid, message);
  for( int i = 0; i < numEmbedPerl; ++i )
  {
    if( strcmp(ePerl[i]->uniqueid(),uniqueid)==0 && ePerl[i]->emoteCallback )
    {
      ePerl[i]->emoteCallback( receiver, message, ePerl[i]->data );
    }
  }
}

void privmsgEmbed(const char *uniqueid, const char *receiver, const char *message)
{
  printf("***Callback*** Send privmsg to %s (on %s): %s\n", receiver, uniqueid, message);
  for( int i = 0; i < numEmbedPerl; ++i )
  {
    if( strcmp(ePerl[i]->uniqueid(),uniqueid)==0 && ePerl[i]->privmsgCallback )
    {
      ePerl[i]->privmsgCallback( receiver, message, ePerl[i]->data );
    }
  }
}
/***** END CALLBACKS ****/
}

EmbedPerl::EmbedPerl(const char *uniqueid)
: emoteCallback(0)
, privmsgCallback(0)
{
  if( MAX_EMBED_PERL == numEmbedPerl )
  {
    fprintf(stderr, "Too many EmbedPerls. Raise MAX_EMBED_PERL\n");
    abort();
  }

  for( int i = 0; i < numEmbedPerl; ++i )
  {
    if( strcmp(ePerl[i]->uniqueid(), uniqueid) != 0 )
    {
      fprintf(stderr,"EmbedPerl created for a duplicate uniqueid.\n");
      abort();
    }
  }
  uniqueid_ = uniqueid;
  ePerl[numEmbedPerl++] = this;

  perl = perl_alloc();
  perl_construct(perl);

  char *argv[] = {"", "dazeus2_api_emulate.pl"};
  perl_parse(perl, xs_init, 1, argv, NULL);

  init();
}

EmbedPerl::~EmbedPerl()
{
  fprintf(stderr, "EmbedPerl destructor\n");
  perl_destruct(perl);
  perl_free(perl);
}

const char *EmbedPerl::uniqueid() const
{
  return uniqueid_;
}

void EmbedPerl::setCallbacks( void (*emoteCallback)  (const char*, const char*, void*),
                              void (*privmsgCallback)(const char*, const char*, void*),
                              void *data )
{
  this->emoteCallback = emoteCallback;
  this->privmsgCallback = privmsgCallback;
  this->data = data;
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
