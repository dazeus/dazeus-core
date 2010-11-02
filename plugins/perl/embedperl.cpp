/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#include "perl_callbacks.h"
#include "embedperl.h"
#include <assert.h>

#define MAX_EMBED_PERL 10
// #define DEBUG

// Totally thread-unsafe...
static EmbedPerl *ePerl[MAX_EMBED_PERL];
static int numEmbedPerl = 0;

extern "C" {
void xs_init(pTHX);

/***** CALLBACKS *****/

#define FOR_EVERY_EMBED(UNIQUEID,CALLBACK) \
  for( int i = 0; i < numEmbedPerl; ++i ) \
     if( strcmp(ePerl[i]->uniqueid(),UNIQUEID)==0 && ePerl[i]->CALLBACK )

void emoteEmbed(const char *uniqueid, const char *receiver, const char *message)
{
#ifdef DEBUG
  printf("***Callback*** Send emote to %s (on %s): %s\n", receiver, uniqueid, message);
#endif
  FOR_EVERY_EMBED(uniqueid,emoteCallback) {
    ePerl[i]->emoteCallback( uniqueid, receiver, message, ePerl[i]->data );
    return;
  }
  fprintf(stderr,"[CallbackError] No handler for Emote callback (uniqueid=%s)\n", uniqueid);
}

void privmsgEmbed(const char *uniqueid, const char *receiver, const char *message)
{
#ifdef DEBUG
  printf("***Callback*** Send privmsg to %s (on %s): %s\n", receiver, uniqueid, message);
#endif
  FOR_EVERY_EMBED(uniqueid,privmsgCallback) {
    ePerl[i]->privmsgCallback( uniqueid, receiver, message, ePerl[i]->data );
    return;
  }
  fprintf(stderr,"[CallbackError] No handler for Privmsg callback (uniqueid=%s)\n", uniqueid);
}

const char *getPropertyEmbed(const char *uniqueid, const char *variable)
{
  FOR_EVERY_EMBED(uniqueid,getPropertyCallback) {
    return ePerl[i]->getPropertyCallback( uniqueid, variable, ePerl[i]->data );
  }
}

void setPropertyEmbed(const char *uniqueid, const char *variable, const char *value)
{
  FOR_EVERY_EMBED(uniqueid,setPropertyCallback) {
    ePerl[i]->setPropertyCallback( uniqueid, variable, value, ePerl[i]->data );
    return;
  }
}

void unsetPropertyEmbed(const char *uniqueid, const char *variable)
{
  FOR_EVERY_EMBED(uniqueid,unsetPropertyCallback) {
    ePerl[i]->unsetPropertyCallback( uniqueid, variable, ePerl[i]->data );
    return;
  }
}

const char *getNickEmbed(const char *uniqueid)
{
  FOR_EVERY_EMBED(uniqueid,getNickCallback) {
    return ePerl[i]->getNickCallback( ePerl[i]->data );
  }
}

void sendWhoisEmbed(const char *uniqueid, const char *who)
{
  FOR_EVERY_EMBED(uniqueid,sendWhoisCallback) {
    ePerl[i]->sendWhoisCallback( who, ePerl[i]->data );
    return;
  }
}

void joinEmbed(const char *uniqueid, const char *channel)
{
  FOR_EVERY_EMBED(uniqueid,joinCallback) {
    ePerl[i]->joinCallback( channel, ePerl[i]->data );
    return;
  }
}

void partEmbed(const char *uniqueid, const char *channel)
{
  FOR_EVERY_EMBED(uniqueid,partCallback) {
    ePerl[i]->partCallback( channel, ePerl[i]->data );
    return;
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
  uniqueid_ = (char*)malloc(strlen(uniqueid));
  strcpy(uniqueid_, uniqueid);
  ePerl[numEmbedPerl++] = this;

  perl = perl_alloc();
  perl_construct(perl);

  char *argv[] = {"", "dazeus2_api_emulate.pl"};
  perl_parse(perl, xs_init, 1, argv, NULL);

  init();
}

EmbedPerl::~EmbedPerl()
{
#ifdef DEBUG
  fprintf(stderr, "EmbedPerl destructor\n");
#endif
  perl_destruct(perl);
  perl_free(perl);
}

const char *EmbedPerl::uniqueid() const
{
  return uniqueid_;
}

void EmbedPerl::setCallbacks( void (*emoteCallback)  (const char*, const char*, const char*, void*),
                              void (*privmsgCallback)(const char*, const char*, const char*, void*),
                              const char* (*getPropertyCallback)(const char*, const char*, void*),
                              void (*setPropertyCallback)(const char*, const char*, const char*, void*),
                              void (*unsetPropertyCallback)(const char*, const char*, void*),
                              void (*sendWhoisCallback)(const char*, void*),
                              void (*joinCallback)(const char*, void*),
                              void (*partCallback)(const char*, void*),
                              const char* (*getNickCallback)(void*),
                              void *data )
{
  this->emoteCallback = emoteCallback;
  this->privmsgCallback = privmsgCallback;
  this->getPropertyCallback = getPropertyCallback;
  this->setPropertyCallback = setPropertyCallback;
  this->unsetPropertyCallback = unsetPropertyCallback;
  this->sendWhoisCallback = sendWhoisCallback;
  this->joinCallback = joinCallback;
  this->partCallback = partCallback;
  this->getNickCallback = getNickCallback;
  this->data = data;
}

void EmbedPerl::init()
{
#ifdef DEBUG
  fprintf(stderr, "EmbedPerl::init()\n");
#endif
  PerlInterpreter *my_perl = perl;
  dSP;
  ENTER;
  SAVETMPS;
  PUSHMARK(SP);
  XPUSHs( newSVpv(uniqueid_, strlen(uniqueid_)));
  PUTBACK;
  call_pv("init", G_SCALAR);
  SPAGAIN;
  PUTBACK;
  FREETMPS;
  LEAVE;
}

void EmbedPerl::message(const char *from, const char *to, const char *msg)
{
#ifdef DEBUG
  fprintf(stderr, "EmbedPerl::message(%s,%s,%s)\n", from, to, msg);
#endif
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
#ifdef DEBUG
  fprintf(stderr, "... message(...)=%d\n", result);
#endif
}

void EmbedPerl::whois(char const *nick, int isIdentified)
{
#ifdef DEBUG
  fprintf(stderr, "EmbedPerl::whois(%s,%d)\n", nick, isIdentified);
#endif
  PerlInterpreter *my_perl = perl;
  dSP;
  ENTER;
  SAVETMPS;
  PUSHMARK(SP);
  XPUSHs( newSVpv(nick, strlen(nick)) );
  XPUSHs( newSVpv(isIdentified == 1 ? "1" : "0", strlen("1")));
  PUTBACK;
  call_pv("whois", G_SCALAR);
  SPAGAIN;
  int result = POPi;
  PUTBACK;
  FREETMPS;
  LEAVE;
#ifdef DEBUG
  fprintf(stderr, ".. whois(...)=%d\n", result);
#endif
}

bool EmbedPerl::loadModule(const char *module)
{
#ifdef DEBUG
  fprintf(stderr, "EmbedPerl::loadModule(%s)", module);
#endif
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
#ifdef DEBUG
  fprintf(stderr, "=%d\n", result);
#endif
  return result != 0;
}
