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
            PerlPlugin( PluginManager* );
  virtual  ~PerlPlugin();

  public slots:
    virtual void init();
    virtual void welcomed( Network &net );
    virtual void connected( Network &net, const Server &serv );
    virtual void joined( Network &net, const QString &who, Irc::Buffer *b );
    virtual void parted( Network &net, const QString &who, const QString &leaveMessage,
                              Irc::Buffer *b );
    virtual void messageReceived( Network &net, const QString &origin, const QString &message,
                                  Irc::Buffer *buffer );

    // Do *not* call from the outside!
    void       emoteCallback  ( const char *network, const char *receiver, const char *body );
    void       privmsgCallback( const char *network, const char *receiver, const char *body );
    const char*getPropertyCallback(const char *network, const char *variable);
    void       setPropertyCallback(const char *network, const char *variable, const char *value);
    void       unsetPropertyCallback(const char *network, const char *variable);
    void       sendWhoisCallback(const char *who);
    void       joinCallback(const char *channel);
    void       partCallback(const char *channel);
    const char*getNickCallback();

  protected slots:
    virtual QHash<QString, VariableScope> variables();
    EmbedPerl *getNetworkEmbed( Network &net );

  private:
    QHash<QString,EmbedPerl*> ePerl;
};

#endif
