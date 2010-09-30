#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"
#include "perl_callbacks.h"

#define PL_unitcheckav 0

MODULE = DaZeus2		PACKAGE = DaZeus2

PROTOTYPES: ENABLE

void
emote(const char *network, const char *receiver, const char *message)
  CODE:
    emoteEmbed(network, receiver, message);

void
privmsg(const char *network, const char *receiver, const char *message)
  CODE:
    privmsgEmbed(network, receiver, message);
