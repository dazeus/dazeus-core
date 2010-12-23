/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#ifndef NETWORK_H
#define NETWORK_H

#include <QtCore/QObject>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QHash>

#include "user.h"

class Server;
class Network;

struct ServerConfig;
struct NetworkConfig;

QDebug operator<<(QDebug, const Network &);
QDebug operator<<(QDebug, const Network *);

namespace Irc
{
  class Buffer;
};

class Network : public QObject
{
  Q_OBJECT

  public:
                   ~Network();

    static Network *fromNetworkConfig( const NetworkConfig *c );
    static Network *getNetwork( const QString &name );
    static Network *fromBuffer( Irc::Buffer *b );

    enum DisconnectReason {
      UnknownReason,
      ShutdownReason,
      ConfigurationReloadReason,
      SwitchingServersReason,
      ErrorReason,
      AdminRequestReason,
    };

    bool                        autoConnectEnabled() const;
    const QList<ServerConfig*> &servers() const;
    User                       *user();
    const Server               *activeServer() const;
    const NetworkConfig        *config() const;
    int                         serverUndesirability( const ServerConfig *sc ) const;

  public slots:
    void connectToNetwork( bool reconnect = false );
    void disconnectFromNetwork( DisconnectReason reason = UnknownReason );
    void joinChannel( QString channel );
    void leaveChannel( QString channel );
    void say( QString destination, QString message );
    void action( QString destination, QString message );
    void ctcp( QString destination, QString message );
    void sendWhois( QString destination );
    void flagUndesirableServer( const ServerConfig *sc );
    void serverIsActuallyOkay( const ServerConfig *sc );

  signals:
    void connected();
    void disconnected();
    void welcomed();
    void motdReceived( const QString &motd, Irc::Buffer *buffer );
    void joined( const QString &origin, Irc::Buffer *buffer );
    void parted( const QString &origin, const QString &message, Irc::Buffer *buffer );
    void quit(   const QString &origin, const QString &message, Irc::Buffer *buffer );
    void nickChanged( const QString &origin, const QString &nick, Irc::Buffer *buffer );
    void modeChanged( const QString &origin, const QString &mode, const QString &args, Irc::Buffer *buffer );
    void topicChanged( const QString &origin, const QString &topic, Irc::Buffer *buffer );
    void invited( const QString &origin, const QString &receiver, const QString &channel, Irc::Buffer *buffer );
    void kicked( const QString &origin, const QString &nick, const QString &message, Irc::Buffer *buffer );
    void messageReceived( const QString &origin, const QString &message, Irc::Buffer *buffer );
    void noticeReceived( const QString &origin, const QString &notice, Irc::Buffer *buffer );
    void ctcpRequestReceived( const QString &origin, const QString &request, Irc::Buffer *buffer );
    void ctcpReplyReceived( const QString &origin, const QString &reply, Irc::Buffer *buffer );
    void ctcpActionReceived( const QString &origin, const QString &action, Irc::Buffer *buffer );
    void numericMessageReceived( const QString &origin, uint code, const QStringList &params, Irc::Buffer *buffer );
    void unknownMessageReceived( const QString &origin, const QStringList &params, Irc::Buffer *buffer );

  private:
    void connectToServer( ServerConfig *conf, bool reconnect );
    bool disconnect( const QObject *sender, const char *signal,
      const QObject *receiver, const char *method )
    { return QObject::disconnect( sender, signal, receiver, method ); }
    bool connect( const QObject *sender, const char *signal,
      const QObject *receiver, const char *method,
      Qt::ConnectionType type = Qt::AutoConnection )
    { return QObject::connect( sender, signal, receiver, method, type ); }
 
                          Network( const QString &name );
    Server               *activeServer_;
    const NetworkConfig  *config_;
    static QHash<QString,Network*> networks_;
    QHash<const ServerConfig*,int> undesirables_;
    User                 *me_;

  private slots:
    void onFailedConnection();
    void serverIsActuallyOkay();

};

#endif
