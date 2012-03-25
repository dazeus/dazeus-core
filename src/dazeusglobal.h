/*
 * Copyright (C) 2010, Sjors Gielen <dazjorz@dazjorz.com>
 * See LICENSE for license.
 */

#ifndef DAZEUSGLOBAL_H
#define DAZEUSGLOBAL_H

// 2.0-beta1 is not numeric. When a numeric is required, "2.0-beta1"
// comes before "2.0" but way after "1.whatever". Therefore, "1.9" is used
// for 2.0-beta, so beta1 is "1.9.1".
#define DAZEUS_VERSION_STR "2.0-beta1"
#define DAZEUS_VERSION 0x010901

#define DAZEUS_DEFAULT_CONFIGFILE "dazeus.conf"

#endif
