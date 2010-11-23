/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#include "statistics.h"

Statistics::Statistics(PluginManager *man)
: Plugin(man)
{}

Statistics::~Statistics() {}

void Statistics::init()
{
  // null QVariant::toInt() returns 0
  int numInits = get(QLatin1String("numinits")).toInt();
  ++numInits;
  set(Plugin::GlobalScope, QLatin1String("numinits"), numInits);
  const char *thingy = "th";
  if( numInits%100 == 11 ) ;
  else if( numInits%100 == 12 ) ;
  else if( numInits%100 == 13 ) ;
  else if( numInits%10 == 1 ) thingy = "st";
  else if( numInits%10 == 2 ) thingy = "nd";
  else if( numInits%10 == 3 ) thingy = "rd";

  qDebug() << "You're running this bot for the"
           << numInits << thingy << "time!";
}
