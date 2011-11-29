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
static EmbedPerl *ePerl = 0;

extern "C" {
void xs_init(pTHX);

/***** CALLBACKS *****/

void emoteEmbed(const char *uniqueid, const char *receiver, const char *message)
{
#ifdef DEBUG
  printf("***Callback*** Send emote to %s (on %s): %s\n", receiver, uniqueid, message);
#endif
  ePerl->emoteCallback( uniqueid, receiver, message, ePerl->data );
}

void privmsgEmbed(const char *uniqueid, const char *receiver, const char *message)
{
#ifdef DEBUG
  printf("***Callback*** Send privmsg to %s (on %s): %s\n", receiver, uniqueid, message);
#endif
  ePerl->privmsgCallback( uniqueid, receiver, message, ePerl->data );
}

const char *getPropertyEmbed(const char *uniqueid, const char *variable)
{
  return ePerl->getPropertyCallback( uniqueid, variable, ePerl->data );
}

void setPropertyEmbed(const char *uniqueid, const char *variable, const char *value)
{
  ePerl->setPropertyCallback( uniqueid, variable, value, ePerl->data );
}

void unsetPropertyEmbed(const char *uniqueid, const char *variable)
{
  ePerl->unsetPropertyCallback( uniqueid, variable, ePerl->data );
}

const char *getNickEmbed(const char *uniqueid)
{
  return ePerl->getNickCallback( ePerl->data );
}

void sendWhoisEmbed(const char *uniqueid, const char *who)
{
  ePerl->sendWhoisCallback( who, ePerl->data );
}

void joinEmbed(const char *uniqueid, const char *channel)
{
  ePerl->joinCallback( channel, ePerl->data );
}

void partEmbed(const char *uniqueid, const char *channel)
{
  ePerl->partCallback( channel, ePerl->data );
}

/***** END CALLBACKS ****/
}

EmbedPerl::EmbedPerl()
: emoteCallback(0)
, privmsgCallback(0)
, getPropertyCallback(0)
, setPropertyCallback(0)
, unsetPropertyCallback(0)
, sendWhoisCallback(0)
, joinCallback(0)
, partCallback(0)
, getNickCallback(0)
{
  assert(ePerl == 0);
  ePerl = this;
  PERL_SYS_INIT3(NULL, NULL, NULL);

  my_perl = perl_alloc();
  perl_construct(my_perl);

  char *argv[] = {"", "dazeus2_api_emulate.pl"};
  if(perl_parse(my_perl, xs_init, 1, argv, NULL) != 0
  || perl_run(my_perl) != 0)
  {
    fprintf(stderr, "DaZeus2 API emulation layer failed to parse, quitting.\n");
    perl_destruct(my_perl);
    perl_free(my_perl);
    my_perl = 0;
    PERL_SYS_TERM();
    return;
  }
}

EmbedPerl::~EmbedPerl()
{
#ifdef DEBUG
  fprintf(stderr, "EmbedPerl destructor\n");
#endif
  perl_destruct(my_perl);
  perl_free(my_perl);
  PERL_SYS_TERM();
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

void EmbedPerl::init(const char *uniqueid)
{
#ifdef DEBUG
  fprintf(stderr, "EmbedPerl::init()\n");
#endif
  PREPARE_CALL;
  XPUSHs(sv_2mortal(newSVpv(uniqueid, strlen(uniqueid)) ));
  DO_CALL( "init" );
  END_CALL;
}

void EmbedPerl::message(const char *uniqueid, const char *from, const char *to, const char *msg)
{
#ifdef DEBUG
  fprintf(stderr, "EmbedPerl::message(%s,%s,%s)\n", from, to, msg);
#endif
  PREPARE_CALL;
  XPUSHs(sv_2mortal(newSVpv(uniqueid, strlen(uniqueid)) ));
  XPUSHs(sv_2mortal(newSVpv(from, strlen(from)) ));
  XPUSHs(sv_2mortal(newSVpv(to,   strlen(to))   ));
  XPUSHs(sv_2mortal(newSVpv(msg,  strlen(msg))  ));
  XPUSHs(sv_2mortal(newSVpv(msg,  strlen(msg))  ));
  DO_CALL("message");
  int result = POPi;
  END_CALL;
#ifdef DEBUG
  fprintf(stderr, "... message(...)=%d\n", result);
#endif
}

void EmbedPerl::whois(const char *uniqueid, char const *nick, int isIdentified)
{
#ifdef DEBUG
  fprintf(stderr, "EmbedPerl::whois(%s,%d)\n", nick, isIdentified);
#endif
  PREPARE_CALL;
  XPUSHs(sv_2mortal(newSVpv(uniqueid, strlen(uniqueid)) ));
  XPUSHs(sv_2mortal(newSVpv(nick, strlen(nick)) ));
  XPUSHs(sv_2mortal(newSVpv(isIdentified == 1 ? "1" : "0", strlen("1"))));
  DO_CALL( "whois" );
  int result = POPi;
  END_CALL;
#ifdef DEBUG
  fprintf(stderr, ".. whois(...)=%d\n", result);
#endif
}

bool EmbedPerl::loadModule(const char *uniqueid, const char *module)
{
#ifdef DEBUG
  fprintf(stderr, "EmbedPerl::loadModule(%s)", module);
#endif
  PREPARE_CALL;
  XPUSHs(sv_2mortal(newSVpv(uniqueid, strlen(uniqueid)) ));
  XPUSHs(sv_2mortal(newSVpv(module, strlen(module)) ));
  DO_CALL("loadModule");
  int result = POPi;
  END_CALL;
#ifdef DEBUG
  fprintf(stderr, "=%d\n", result);
#endif
  return result != 0;
}

void EmbedPerl::join(const char *uniqueid, const char *channel, const char *who)
{
#ifdef DEBUG
  fprintf(stderr, "EmbedPerl::join(%s, %s)\n", channel, who);
#endif
  PREPARE_CALL;
  XPUSHs(sv_2mortal(newSVpv(uniqueid, strlen(uniqueid)) ));
  XPUSHs(sv_2mortal(newSVpv(channel, strlen(channel)) ));
  XPUSHs(sv_2mortal(newSVpv(who,     strlen(who))     ));
  DO_CALL("join");
  END_CALL;
}

void EmbedPerl::nick(const char *uniqueid, const char *who, const char *new_nick)
{
#ifdef DEBUG
  fprintf(stderr, "EmbedPerl::nick(%s, %s)\n", who, new_nick);
#endif
  PREPARE_CALL;
  XPUSHs(sv_2mortal(newSVpv(uniqueid, strlen(uniqueid)) ));
  XPUSHs(sv_2mortal(newSVpv(who,      strlen(who))      ));
  XPUSHs(sv_2mortal(newSVpv(new_nick, strlen(new_nick)) ));
  DO_CALL("nick");
  END_CALL;
}

void EmbedPerl::connected(const char *uniqueid)
{
#ifdef DEBUG
  fprintf(stderr, "EmbedPerl::connected()\n");
#endif
  PREPARE_CALL;
  XPUSHs(sv_2mortal(newSVpv(uniqueid, strlen(uniqueid)) ));
  DO_CALL("connected");
  END_CALL;
}

/**
 * 'names' should be space separated!
 */
void EmbedPerl::namesReceived(const char *uniqueid, const char *channel, const char *names)
{
#ifdef DEBUG
  fprintf(stderr, "EmbedPerl::namesReceived(%s, %s)\n", channel, names);
#endif
  PREPARE_CALL;
  XPUSHs(sv_2mortal(newSVpv(uniqueid, strlen(uniqueid)) ));
  XPUSHs(sv_2mortal(newSVpv(channel, strlen(channel)) ));
  XPUSHs(sv_2mortal(newSVpv(names,   strlen(names))   ));
  DO_CALL("namesReceived");
  END_CALL;
}

/**
 * Call this method every second
 */
void EmbedPerl::tick(const char *uniqueid)
{
#ifdef DEBUG
  fprintf(stderr, "EmbedPerl::tick()\n");
#endif
  PREPARE_CALL;
  XPUSHs(sv_2mortal(newSVpv(uniqueid, strlen(uniqueid)) ));
  DO_CALL("tick");
  END_CALL;
}
