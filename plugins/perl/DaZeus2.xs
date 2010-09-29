#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"
#include "const-c.h"

#define PL_unitcheckav 0

MODULE = DaZeus2		PACKAGE = DaZeus2

PROTOTYPES: ENABLE

void
callBack()
  CODE:
    callbackEmbed("Hello, world!\n");

void
emote(const char *receiver, const char *message)
  CODE:
    emoteEmbed(receiver, message);

void
privmsg(const char *receiver, const char *message)
  CODE:
    privmsgEmbed(receiver, message);
