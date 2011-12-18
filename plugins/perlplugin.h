/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#ifndef PERLPLUGIN_H
#define PERLPLUGIN_H

#include "plugin.h"
#include "perl/embedperl.h"
#include <QtCore/QVariant>
#include <QtCore/QTimer>

class PerlPlugin : public Plugin
{
  Q_OBJECT

  public:
            PerlPlugin( PluginManager* );
  virtual  ~PerlPlugin();

  public slots:
    virtual void init();
    virtual void messageReceived( Network &net, const QString &origin, const QString &message,
                                  Irc::Buffer *buffer );
    virtual void numericMessageReceived( Network &net, const QString &origin, uint code,
                                     const QStringList &params,
                                     Irc::Buffer *buffer );
    virtual void connected( Network &net, const Server &serv );
    virtual void joined( Network &net, const QString &who, Irc::Buffer *channel );
    virtual void nickChanged( Network &net, const QString &origin, const QString &nick,
                          Irc::Buffer *buffer );


    // Do *not* call from the outside!
    void       emoteCallback  ( const char *network, const char *receiver, const char *body );
    void       privmsgCallback( const char *network, const char *receiver, const char *body );
    const char*getPropertyCallback(const char *network, const char *variable);
    void       setPropertyCallback(const char *network, const char *variable, const char *value);
    void       unsetPropertyCallback(const char *network, const char *variable);
    const char** getPropertyKeysCallback(const char *network, const char *ns, int *length);
    void       sendWhoisCallback(const char *who);
    void       joinCallback(const char *channel);
    void       partCallback(const char *channel);
    const char*getNickCallback();

  protected:
    EmbedPerl *getNetworkEmbed( Network &net );

  private slots:
    void       initNetwork(QString uniqueId);
    void       tick();

  private:
    EmbedPerl* ePerl;

    // TODO: write some code so that these can be cleaned up later:
    QByteArray propertyCopy_;
    const char **cKeys_;
    int          cKeysLength_;

    QTimer tickTimer_;
    QList<QString> uniqueIds_;
#warning TODO move this to a proper place
    QString in_whois;
    bool whois_identified;
    QString names_;
};

#endif
