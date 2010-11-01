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

const char*
getProperty(const char *uniqueid, const char *variable)
  CODE:
    RETVAL = getPropertyEmbed(uniqueid, variable);
  OUTPUT:
    RETVAL

void
setProperty(const char *uniqueid, const char *variable, const char *value)
  CODE:
    setPropertyEmbed(uniqueid, variable, value);

void
unsetProperty(const char *uniqueid, const char *variable)
  CODE:
    unsetPropertyEmbed(uniqueid, variable);

const char*
getNick()
  CODE:
    RETVAL = getNickEmbed();
  OUTPUT:
    RETVAL
