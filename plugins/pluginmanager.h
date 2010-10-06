/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <QtCore/QObject>

class Config;
class Plugin;
class User;
class Network;
class Server;

namespace Irc {
  class Buffer;
};

class PluginManager : public QObject
{
  Q_OBJECT

  public:
            PluginManager();
           ~PluginManager();
    bool    isInitialized() const;

  public slots:
    void    setConfig( Config* );
    bool    initialize();

    void     welcomed( Network& );
    void     connected( Network&, const Server& );
    void     joined( const QString&, Irc::Buffer* );
    void     disconnected( Network& );
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

  private slots:
    void    reset();

  private:
    Config         *config_;
    QList<Plugin*>  plugins_;
    bool            initialized_;
};

#endif
