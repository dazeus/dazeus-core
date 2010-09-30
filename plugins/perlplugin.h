/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#ifndef PERLPLUGIN_H
#define PERLPLUGIN_H

#include "plugin.h"
#include "perl/embedperl.h"
#include <QtCore/QVariant>

class PerlPlugin : public Plugin
{
  Q_OBJECT

  public:
            PerlPlugin();
  virtual  ~PerlPlugin();

  public slots:
    virtual void init();
    virtual void welcomed( Network &net, const Server &serv );
    virtual void connected( Network &net, const Server &serv );
    virtual void joinedChannel( const QString &who, Irc::Buffer *b );
    virtual void leftChannel( const QString &who, const QString &leaveMessage,
                              Irc::Buffer *b );

  protected slots:
    virtual QHash<QString, VariableScope> variables();

  private:
    EmbedPerl *ePerl;
};

#endif
