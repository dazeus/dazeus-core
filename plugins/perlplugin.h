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
    virtual void welcomed( Network &net );
    virtual void connected( Network &net, const Server &serv );
    virtual void joinedChannel( Network &net, const QString &who, Irc::Buffer *b );
    virtual void leftChannel( Network &net, const QString &who, const QString &leaveMessage,
                              Irc::Buffer *b );

    // Do *not* call from the outside!
    void       emoteCallback  ( const char *network, const char *receiver, const char *body );
    void       privmsgCallback( const char *network, const char *receiver, const char *body );

  protected slots:
    virtual QHash<QString, VariableScope> variables();
    EmbedPerl *getNetworkEmbed( Network &net );

  private:
    QHash<QString,EmbedPerl*> ePerl;
};

#endif
