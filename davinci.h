/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#ifndef DAVINCI_H
#define DAVINCI_H

#include <QtCore/QString>
#include <QtCore/QObject>

// TODO remove
#include "network.h"

class Config;
class PluginManager;
class Network;

class DaVinci : public QObject
{
  Q_OBJECT

  public:
             DaVinci( QString configFileName = QString() );
            ~DaVinci();
    void     setConfigFileName( QString fileName );
    QString  configFileName() const;
    bool     configLoaded() const;

  public slots:
    bool     loadConfig();
    bool     initPlugins();
    void     autoConnect();

  private slots:
    void     resetConfig();
    void     welcomed();
    void     connected();
    void     joined( const QString&, Irc::Buffer* );
    void     disconnected();
    void     motdReceived( const QString &motd, Irc::Buffer *buffer );
    void     parted( const QString &origin, const QString &message,
                     Irc::Buffer *buffer );
    void     quit(   const QString &origin, const QString &message,
                     Irc::Buffer *buffer );
    void     nickChanged( const QString &origin, const QString &nick,
                          Irc::Buffer *buffer );
    void     modeChanged( const QString &origin, const QString &mode,
                          const QString &args, Irc::Buffer *buffer );
    void     topicChanged( const QString &origin, const QString &topic,
                           Irc::Buffer *buffer );
    void     invited( const QString &origin, const QString &receiver,
                      const QString &channel, Irc::Buffer *buffer );
    void     kicked( const QString &origin, const QString &nick,
                     const QString &message, Irc::Buffer *buffer );
    void     messageReceived( const QString &origin, const QString &message,
                              Irc::Buffer *buffer );
    void     noticeReceived( const QString &origin, const QString &notice,
                             Irc::Buffer *buffer );
    void     ctcpRequestReceived(const QString &origin, const QString &request,
                                 Irc::Buffer *buffer );
    void     ctcpReplyReceived( const QString &origin, const QString &reply,
                                Irc::Buffer *buffer );
    void     ctcpActionReceived( const QString &origin, const QString &action,
                                 Irc::Buffer *buffer );
    void     numericMessageReceived( const QString &origin, uint code,
                                     const QStringList &params,
                                     Irc::Buffer *buffer );
    void     unknownMessageReceived( const QString &origin,
                                     const QStringList &params,
                                     Irc::Buffer *buffer );

  private:
    Config          *config_;
    QString          configFileName_;
    PluginManager   *pluginManager_;
    QList<Network*>  networks_;
};

#endif
