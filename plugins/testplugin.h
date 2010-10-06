/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#ifndef TESTPLUGIN_H
#define TESTPLUGIN_H

#include "plugin.h"
#include <QtCore/QVariant>

class TestPlugin : public Plugin
{
  Q_OBJECT

  public:
            TestPlugin();
  virtual  ~TestPlugin();

  public slots:
    virtual void init();
    virtual void welcomed( Network &net );
    virtual void connected( Network &net, const Server &serv );
    virtual void joined( Network &net, const QString &who, Irc::Buffer *b );
    virtual void parted( Network &net, const QString &who, const QString &leaveMessage,
                              Irc::Buffer *b );

  protected slots:
    virtual QHash<QString, VariableScope> variables();
};

#endif
