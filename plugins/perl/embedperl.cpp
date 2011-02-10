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
    if( strcmp(ePerl[i]->uniqueid(), uniqueid) == 0 )
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

#define PREPARE_CALL \
  PerlInterpreter *my_perl = perl; \
  dSP; \
  ENTER; \
  SAVETMPS; \
  PUSHMARK(SP);

#define DO_CALL(NAME) \
  PUTBACK; \
  call_pv(NAME, G_SCALAR); \
  SPAGAIN;

#define END_CALL \
  PUTBACK; \
  FREETMPS; \
  LEAVE;

void EmbedPerl::init()
{
#ifdef DEBUG
  fprintf(stderr, "EmbedPerl::init()\n");
#endif
  PREPARE_CALL;
  XPUSHs( newSVpv(uniqueid_, strlen(uniqueid_)));
  DO_CALL( "init" );
  END_CALL;
}

void EmbedPerl::message(const char *from, const char *to, const char *msg)
{
#ifdef DEBUG
  fprintf(stderr, "EmbedPerl::message(%s,%s,%s)\n", from, to, msg);
#endif
  PREPARE_CALL;
  XPUSHs( newSVpv(from, strlen(from)) );
  XPUSHs( newSVpv(to,   strlen(to))   );
  XPUSHs( newSVpv(msg,  strlen(msg))  );
  XPUSHs( newSVpv(msg,  strlen(msg))  );
  DO_CALL("message");
  int result = POPi;
  END_CALL;
#ifdef DEBUG
  fprintf(stderr, "... message(...)=%d\n", result);
#endif
}

void EmbedPerl::whois(char const *nick, int isIdentified)
{
#ifdef DEBUG
  fprintf(stderr, "EmbedPerl::whois(%s,%d)\n", nick, isIdentified);
#endif
  PREPARE_CALL;
  XPUSHs( newSVpv(nick, strlen(nick)) );
  XPUSHs( newSVpv(isIdentified == 1 ? "1" : "0", strlen("1")));
  DO_CALL( "whois" );
  int result = POPi;
  END_CALL;
#ifdef DEBUG
  fprintf(stderr, ".. whois(...)=%d\n", result);
#endif
}

bool EmbedPerl::loadModule(const char *module)
{
#ifdef DEBUG
  fprintf(stderr, "EmbedPerl::loadModule(%s)", module);
#endif
  PREPARE_CALL;
  XPUSHs( newSVpv(module, strlen(module)) );
  DO_CALL("loadModule");
  int result = POPi;
  END_CALL;
#ifdef DEBUG
  fprintf(stderr, "=%d\n", result);
#endif
  return result != 0;
}

void EmbedPerl::join(const char *channel, const char *who)
{
#ifdef DEBUG
  fprintf(stderr, "EmbedPerl::join(%s, %s)\n", channel, who);
#endif
  PREPARE_CALL;
  XPUSHs( newSVpv(channel, strlen(channel)) );
  XPUSHs( newSVpv(who,     strlen(who))     );
  DO_CALL("join");
  END_CALL;
}

void EmbedPerl::nick(const char *who, const char *new_nick)
{
#ifdef DEBUG
  fprintf(stderr, "EmbedPerl::nick(%s, %s)\n", who, new_nick);
#endif
  PREPARE_CALL;
  XPUSHs( newSVpv(who,      strlen(who))      );
  XPUSHs( newSVpv(new_nick, strlen(new_nick)) );
  DO_CALL("nick");
  END_CALL;
}

void EmbedPerl::connected()
{
#ifdef DEBUG
  fprintf(stderr, "EmbedPerl::connected()\n");
#endif
  PREPARE_CALL;
  DO_CALL("connected");
  END_CALL;
}

/**
 * 'names' should be space separated!
 */
void EmbedPerl::namesReceived(const char *channel, const char *names)
{
#ifdef DEBUG
  fprintf(stderr, "EmbedPerl::namesReceived(%s, %s)\n", channel, names);
#endif
  PREPARE_CALL;
  XPUSHs( newSVpv(channel, strlen(channel)) );
  XPUSHs( newSVpv(names,   strlen(names))   );
  DO_CALL("namesReceived");
  END_CALL;
}

/**
 * Call this method every second
 */
void EmbedPerl::tick()
{
#ifdef DEBUG
  fprintf(stderr, "EmbedPerl::tick()\n");
#endif
  PREPARE_CALL;
  DO_CALL("tick");
  END_CALL;
}
