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
  const ServerConfig *sc = s->config();
  dbg.nospace() << "Server[" << sc->host << ":" << sc->port << "]";

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
  Irc::Session::setNick( config_->network->nickName );
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
  case Network::ShutdownReason:
    reasonString = "Shutting down";
    break;
  case Network::ConfigurationReloadReason:
    reasonString = "Reloading configuration";
    break;
  case Network::SwitchingServersReason:
    reasonString = "Switching servers";
    break;
  case Network::ErrorReason:
    reasonString = "Unknown error";
    break;
  case Network::AdminRequestReason:
    reasonString = "An admin asked me to disconnect";
    break;
  case Network::UnknownReason:
  default:
    reasonString = "See you around!";
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
  return "";
}


void Server::say( QString destination, QString message )
{
  Irc::Session::message( destination, message );
}

void Server::slotBufferAdded( Irc::Buffer *buffer )
{
  qDebug() << "Buffer added: " << buffer
           << buffer->receiver()
           << buffer->topic()
           << buffer->names()
           << "is server buffer?" << ( buffer == defaultBuffer() || defaultBuffer() == 0 );
  if( buffer == defaultBuffer() || defaultBuffer() == 0 )
  {
    // A server buffer has been created!
    emit authenticated();
  }

//#define SERVER_SIGN_RELAY_1STR( slotname, signname )
  //connect( buffer, SIGNAL( signname (
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
