/**
 * Copyright (c) Sjors Gielen, 2011
 * See LICENSE for license.
 */

#ifndef KARMAPLUGIN_H
#define KARMAPLUGIN_H

#include "plugin.h"
#include <QtCore/QVariant>

class KarmaPlugin : public Plugin
{
  Q_OBJECT

  public:
            KarmaPlugin(PluginManager *man);
  virtual  ~KarmaPlugin();

  public slots:
    virtual void init();
    virtual void messageReceived( Network &net, const QString &origin, const QString &message,
                              Irc::Buffer *buffer );

  private:
    void modifyKarma(QString object, bool increase);
};

#endif
