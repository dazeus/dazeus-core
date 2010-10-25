/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#ifndef STATISTICS_H
#define STATISTICS_H

#include "plugin.h"
#include <QtCore/QVariant>

class Statistics : public Plugin
{
  Q_OBJECT

  public:
            Statistics(PluginManager*);
  virtual  ~Statistics();

  public slots:
    virtual void init();
};

#endif
