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

AV*
getPropertyKeys(const char *uniqueid, const char *ns)
  CODE:
    int length = 0;
    const char **keys = getPropertyKeysEmbed(uniqueid, ns, &length);
    RETVAL = newAV();
    int i;
    for(i = 0; i < length; ++i) {
      SV *val = newSVpv(keys[i], 0);
      av_push(RETVAL, val);
    }
  OUTPUT:
    RETVAL

void
sendWhois(const char *uniqueid, const char *who)
  CODE:
    sendWhoisEmbed(uniqueid, who);

void
join(const char *uniqueid, const char *channel)
  CODE:
    joinEmbed(uniqueid,channel);

void
part(const char *uniqueid, const char *channel)
  CODE:
    partEmbed(uniqueid,channel);

const char*
getNick(const char *uniqueid)
  CODE:
    RETVAL = getNickEmbed(uniqueid);
  OUTPUT:
    RETVAL
