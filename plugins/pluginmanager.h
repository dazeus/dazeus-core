/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <QtCore/QObject>
#include "plugin.h"

class Config;
class User;
class Network;
class Server;
class Database;

namespace Irc {
  class Buffer;
};

struct Context {
  QString network;
  QString receiver;
  QString sender;
};

QDebug operator<<(QDebug, const Context *);

class PluginManager : public QObject
{
  Q_OBJECT

  public:
            PluginManager( Database *db );
           ~PluginManager();
    bool    isInitialized() const;
    Database *database() const;
    const Context *context() const;
    void     set( Plugin::VariableScope s, const QString &name, const QVariant &value );
    QVariant get( const QString &name, Plugin::VariableScope *s = NULL ) const;

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
    void    setContext(QString network, QString receiver = "", QString sender = "");
    void    clearContext();

  private:
    Config         *config_;
    Database       *database_;
    Context        *context_;
    QList<Plugin*>  plugins_;
    bool            initialized_;
};

#endif
