/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#include <QtCore/QStringList>
#include <QtCore/QTimer>
#include <QtNetwork/QHostAddress>
#include <ircclient-qt/IrcBuffer>

#include "server.h"
#include "config.h"

QDebug operator<<(QDebug dbg, const Server &s)
{
  return operator<<(dbg, &s);
}

QDebug operator<<(QDebug dbg, const Server *s)
{
  dbg.nospace() << "Server[";
  if( s == 0 ) {
    dbg.nospace() << "0";
  } else {
    const ServerConfig *sc = s->config();
    dbg.nospace() << sc->host << ":" << sc->port;
  }
  dbg.nospace() << "]";

  return dbg.maybeSpace();
}

Server::Server()
: Irc::Session()
, config_( 0 )
, connectTimer_( 0 )
{
  connect( this, SIGNAL(      bufferAdded(Irc::Buffer*)),
           this, SLOT(    slotBufferAdded(Irc::Buffer*)));
  connect( this, SIGNAL(    bufferRemoved(Irc::Buffer*)),
           this, SLOT(  slotBufferRemoved(Irc::Buffer*)));

  setOptions( options() & ~Irc::Session::EchoMessages );
}


Server::~Server()
{
  delete connectTimer_;
}

void Server::action( QString destination, QString message )
{
  ctcpAction( destination, message );
}

const ServerConfig *Server::config() const
{
  return config_;
}

void Server::connectToServer()
{
  qDebug() << "Connecting to server" << this;
  Q_ASSERT( !config_->network->nickName.isEmpty() );
  Irc::Session::setNick(         config_->network->nickName );
  Irc::Session::setIdent(        config_->network->userName );
  Irc::Session::setRealName(     config_->network->fullName );
  Irc::Session::setPassword(     config_->network->password );
  Irc::Session::connectToServer( config_->host, config_->port );
}

void Server::ctcp( QString destination, QString message )
{
  ctcpRequest( destination, message );
}

void Server::disconnectFromServer( Network::DisconnectReason reason )
{
  QString reasonString;
  switch( reason )
  {
#define rw(x) QLatin1String(x)
  case Network::ShutdownReason:
    reasonString = rw("Shutting down");
    break;
  case Network::ConfigurationReloadReason:
    reasonString = rw("Reloading configuration");
    break;
  case Network::SwitchingServersReason:
    reasonString = rw("Switching servers");
    break;
  case Network::ErrorReason:
    reasonString = rw("Unknown error");
    break;
  case Network::AdminRequestReason:
    reasonString = rw("An admin asked me to disconnect");
    break;
  case Network::UnknownReason:
  default:
    reasonString = rw("See you around!");
#undef rw
  }

  quit( reasonString );
}


/**
 * @brief Create a Server from a given ServerConfig.
 */
Server *Server::fromServerConfig( const ServerConfig *c )
{
  Server *s = new Server;
  s->config_ = c;
  return s;
}


void Server::joinChannel( QString channel, const QString &key )
{
  join( channel, key );
}

void Server::leaveChannel( QString channel, const QString &reason )
{
  part( channel, reason );
}

QString Server::motd() const
{
  qWarning() << "MOTD cannot be retrieved.";
  return QString();
}


void Server::say( QString destination, QString message )
{
  QStringList lines = message.split(QRegExp(QLatin1String("[\\r\\n]+")), QString::SkipEmptyParts);
  foreach(const QString& line, lines)
    Irc::Session::message( destination, line );
}

void Server::slotBufferAdded( Irc::Buffer *buffer )
{
#define SERVER_SIGNAL_RELAY_1STR( slotname, signname ) \
  connect( buffer, SIGNAL( signname (const QString&) ), \
           this,   SLOT(   slotname (const QString&) ) );
#define SERVER_SIGNAL_RELAY_2STR( slotname, signname ) \
  connect( buffer, SIGNAL( signname (const QString&, const QString&) ), \
           this,   SLOT(   slotname (const QString&, const QString&) ) );
#define SERVER_SIGNAL_RELAY_3STR( slotname, signname ) \
  connect( buffer, SIGNAL( signname (const QString&, const QString&, const QString&) ), \
           this,   SLOT(   slotname (const QString&, const QString&, const QString&) ) );

  SERVER_SIGNAL_RELAY_1STR( slotMotdReceived, motdReceived );
  SERVER_SIGNAL_RELAY_1STR( slotJoined, joined );
  SERVER_SIGNAL_RELAY_2STR( slotParted, parted );
  SERVER_SIGNAL_RELAY_2STR( slotQuit, quit );
  SERVER_SIGNAL_RELAY_2STR( slotNickChanged, nickChanged );
  SERVER_SIGNAL_RELAY_3STR( slotModeChanged, modeChanged );
  SERVER_SIGNAL_RELAY_2STR( slotTopicChanged, topicChanged );
  SERVER_SIGNAL_RELAY_3STR( slotInvited, invited );
  SERVER_SIGNAL_RELAY_3STR( slotKicked, kicked );
  SERVER_SIGNAL_RELAY_2STR( slotMessageReceived, messageReceived );
  SERVER_SIGNAL_RELAY_2STR( slotNoticeReceived, noticeReceived );
  SERVER_SIGNAL_RELAY_2STR( slotCtcpRequestReceived, ctcpRequestReceived );
  SERVER_SIGNAL_RELAY_2STR( slotCtcpReplyReceived, ctcpReplyReceived );
  SERVER_SIGNAL_RELAY_2STR( slotCtcpActionReceived, ctcpActionReceived );

#undef SERVER_SIGNAL_RELAY_1STR
#undef SERVER_SIGNAL_RELAY_2STR
#undef SERVER_SIGNAL_RELAY_3STR

  connect( buffer, SIGNAL(     unknownMessageReceived( const QString&,
                                                       const QStringList& )),
           this,   SLOT(   slotUnknownMessageReceived( const QString&,
                                                       const QStringList& )));
  connect( buffer, SIGNAL(     numericMessageReceived( const QString&,
                                                 uint, const QStringList& )),
           this,   SLOT(   slotNumericMessageReceived( const QString&,
                                                 uint, const QStringList& )));
}

void Server::slotBufferRemoved( Irc::Buffer *buffer )
{
  disconnect( buffer );
}

#define SERVER_SLOT_RELAY_1STR( slotname, signname ) \
void Server::slotname ( const QString &str ) { \
  QObject *s = sender(); \
  Q_ASSERT( s != 0 ); \
  Irc::Buffer *buf = qobject_cast<Irc::Buffer*>( s ); \
  Q_ASSERT( buf != 0 ); \
  emit signname ( str, buf ); \
}

#define SERVER_SLOT_RELAY_2STR( slotname, signname ) \
void Server::slotname ( const QString &str, const QString &str2 ) { \
  QObject *s = sender(); \
  Q_ASSERT( s != 0 ); \
  Irc::Buffer *buf = qobject_cast<Irc::Buffer*>( s ); \
  Q_ASSERT( buf != 0 ); \
  emit signname ( str, str2, buf ); \
}

#define SERVER_SLOT_RELAY_3STR( slotname, signname ) \
void Server::slotname (const QString &str, const QString &str2, const QString &str3) \
{ \
  QObject *s = sender(); \
  Q_ASSERT( s != 0 ); \
  Irc::Buffer *buf = qobject_cast<Irc::Buffer*>( s ); \
  Q_ASSERT( buf != 0 ); \
  emit signname ( str, str2, str3, buf ); \
}

SERVER_SLOT_RELAY_1STR( slotMotdReceived, motdReceived );
SERVER_SLOT_RELAY_1STR( slotJoined, joined );
SERVER_SLOT_RELAY_2STR( slotParted, parted );
SERVER_SLOT_RELAY_2STR( slotQuit, quit );
SERVER_SLOT_RELAY_2STR( slotNickChanged, nickChanged );
SERVER_SLOT_RELAY_3STR( slotModeChanged, modeChanged );
SERVER_SLOT_RELAY_2STR( slotTopicChanged, topicChanged );
SERVER_SLOT_RELAY_3STR( slotInvited, invited );
SERVER_SLOT_RELAY_3STR( slotKicked, kicked );
SERVER_SLOT_RELAY_2STR( slotMessageReceived, messageReceived );
SERVER_SLOT_RELAY_2STR( slotNoticeReceived, noticeReceived );
SERVER_SLOT_RELAY_2STR( slotCtcpRequestReceived, ctcpRequestReceived );
SERVER_SLOT_RELAY_2STR( slotCtcpReplyReceived, ctcpReplyReceived );
SERVER_SLOT_RELAY_2STR( slotCtcpActionReceived, ctcpActionReceived );

#undef SERVER_SLOT_RELAY_1STR
#undef SERVER_SLOT_RELAY_2STR
#undef SERVER_SLOT_RELAY_3STR

void Server::slotNumericMessageReceived( const QString &str, uint code,
                                 const QStringList &list )
{
  QObject *s = sender();
  Q_ASSERT( s != 0 );
  Irc::Buffer *buf = qobject_cast<Irc::Buffer*>( s );
  Q_ASSERT( buf != 0 );
  emit numericMessageReceived( str, code, list, buf );
}

void Server::slotUnknownMessageReceived( const QString &str, const QStringList &list )
{
  QObject *s = sender();
  Q_ASSERT( s != 0 );
  Irc::Buffer *buf = qobject_cast<Irc::Buffer*>( s );
  Q_ASSERT( buf != 0 );
  emit unknownMessageReceived( str, list, buf );
}
