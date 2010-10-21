/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#include "network.h"
#include "server.h"
#include "config.h"
#include "user.h"

#include <ircclient-qt/IrcBuffer>

QHash<QString, Network*> Network::networks_;

QDebug operator<<(QDebug dbg, const Network &n)
{
  return operator<<( dbg, &n );
}

QDebug operator<<(QDebug dbg, const Network *n)
{
  const NetworkConfig *nc = n->config();
  dbg.nospace() << "Network[" << nc->displayName << ","
                              << n->activeServer() << "]";

  return dbg.maybeSpace();
}

/**
 * @brief Constructor.
 */
Network::Network( const QString &name )
: QObject()
, activeServer_(0)
, config_(0)
, me_(0)
{
  Q_ASSERT( !networks_.contains( name ) );
  networks_.insert( name, this );
}


/**
 * @brief Destructor.
 */
Network::~Network()
{
  disconnectFromNetwork();
  Q_ASSERT( networks_.contains( config_->name ) && getNetwork( config_->name ) == this );
  networks_.remove( config_->name );
}


/**
 * Returns the active server in this network, or 0 if there is none.
 */
const Server *Network::activeServer() const
{
  return activeServer_;
}


/**
 * @brief Whether this network is marked for autoconnection.
 */
bool Network::autoConnectEnabled() const
{
  return config_->autoConnect;
}

void Network::action( QString destination, QString message )
{
  if( !activeServer_ )
    return;
  return activeServer_->action( destination, message );
}

bool serverPriorityLessThan( const ServerConfig *c1, const ServerConfig *c2 )
{
  // TODO: also take in consideration some "error factor" or "last error given"
  // so DaVinci doesn't try to reconnect to a failing server
  return c1->priority < c2->priority;
}

const NetworkConfig *Network::config() const
{
  return config_;
}

void Network::connectToNetwork( bool reconnect )
{
  if( !reconnect && activeServer_ )
    return;

  // Check if there *is* a server to use
  if( servers().count() == 0 )
  {
    qWarning() << "Trying to connect to network" << config_->displayName
               << "but there are no servers to connect to!";
    return;
  }

  // Find the best server to use

  // First, sort the list by priority
  QList<ServerConfig*> sortedServers = servers();
  qSort( sortedServers.begin(), sortedServers.end(), serverPriorityLessThan );

  // Then, take the first one and create a Server around it
  ServerConfig *best = sortedServers[0];

  // And set it as the active server, and connect.
  connectToServer( best, true );
}


void Network::connectToServer( ServerConfig *server, bool reconnect )
{
  if( !reconnect && activeServer_ )
    return;

  if( activeServer_ )
  {
    disconnect( activeServer_, 0, 0, 0 );
    activeServer_->disconnectFromServer( SwitchingServersReason );
    activeServer_->deleteLater();
  }

  activeServer_ = Server::fromServerConfig( server );
  #define RELAY_SIGN(x) connect(activeServer_, SIGNAL(x), this, SIGNAL(x));
  RELAY_SIGN( connected() );
  RELAY_SIGN( disconnected() );
  RELAY_SIGN( welcomed() );
  RELAY_SIGN( motdReceived( const QString&, Irc::Buffer* ) );
  RELAY_SIGN( joined( const QString&, Irc::Buffer* ) );
  RELAY_SIGN( parted( const QString&, const QString&, Irc::Buffer* ) );
  RELAY_SIGN( quit(   const QString&, const QString&, Irc::Buffer* ) );
  RELAY_SIGN( nickChanged( const QString&, const QString&, Irc::Buffer* ) );
  RELAY_SIGN( modeChanged( const QString&, const QString&, const QString &, Irc::Buffer* ) );
  RELAY_SIGN( topicChanged( const QString&, const QString&, Irc::Buffer* ) );
  RELAY_SIGN( invited( const QString&, const QString&, const QString &, Irc::Buffer* ) );
  RELAY_SIGN( kicked( const QString&, const QString&, const QString &, Irc::Buffer* ) );
  RELAY_SIGN( messageReceived( const QString&, const QString&, Irc::Buffer* ) );
  RELAY_SIGN( noticeReceived( const QString&, const QString&, Irc::Buffer* ) );
  RELAY_SIGN( ctcpRequestReceived( const QString&, const QString&, Irc::Buffer* ) );
  RELAY_SIGN( ctcpReplyReceived( const QString&, const QString&, Irc::Buffer* ) );
  RELAY_SIGN( ctcpActionReceived( const QString&, const QString&, Irc::Buffer* ) );
  RELAY_SIGN( numericMessageReceived( const QString&, uint, const QStringList&, Irc::Buffer* ) );
  RELAY_SIGN( unknownMessageReceived( const QString&, const QStringList&, Irc::Buffer* ) );
  #undef RELAY_SIGN

  connect( activeServer_, SIGNAL(      disconnected() ),
           this,          SLOT(       cleanupServer() ) );
  connect( activeServer_, SIGNAL( connectionTimeout() ),
           this,          SLOT(       cleanupServer() ) );
  connect( activeServer_, SIGNAL(         destroyed() ),
           this,          SLOT(       cleanupServer() ) );
  activeServer_->connectToServer();
}


void Network::cleanupServer()
{
  // TODO: auto-reconnect if not shutting down
  Q_ASSERT( sender() == activeServer_ );
  disconnect( activeServer_, 0, 0, 0 );
  activeServer_ = 0;
}


void Network::ctcp( QString destination, QString message )
{
  if( !activeServer_ )
    return;
  return activeServer_->ctcp( destination, message );
}


/**
 * @brief Disconnect from the network.
 */
void Network::disconnectFromNetwork( DisconnectReason reason )
{
  if( activeServer_ == 0 )
    return;

  disconnect( activeServer_, 0, 0, 0 );
  activeServer_->disconnectFromServer( reason );
  activeServer_->deleteLater();
  activeServer_ = 0;
}


/**
 * @brief Get the Network belonging to this Buffer.
 */
Network *Network::fromBuffer( Irc::Buffer *b )
{
  // One of the Servers in one of the Networks in network_, is an
  // Irc::Session that has created this buffer.
  foreach( Network *n, networks_.values() )
  {
    if( n->activeServer() == qobject_cast<Server*>( b->session() ) )
    {
      return n;
    }
  }
  return 0;
}


/**
 * @brief Create a Network from a given NetworkConfig.
 */
Network *Network::fromNetworkConfig( const NetworkConfig *c )
{
  if( networks_.contains( c->name ) )
    return networks_.value( c->name );

  Network *n = new Network( c->name );
  n->config_ = c;
  return n;
}


/**
 * @brief Retrieve the Network with given name.
 */
Network *Network::getNetwork( const QString &name )
{
  if( !networks_.contains( name ) )
    return 0;
  return networks_.value( name );
}

void Network::joinChannel( QString channel )
{
  if( !activeServer_ )
    return;
  return activeServer_->joinChannel( channel );
}


void Network::leaveChannel( QString channel )
{
  if( !activeServer_ )
    return;
  return activeServer_->leaveChannel( channel );
}


void Network::say( QString destination, QString message )
{
  if( !activeServer_ )
    return;
  return activeServer_->say( destination, message );
}


const QList<ServerConfig*> &Network::servers() const
{
  return config_->servers;
}


User *Network::user()
{
  if( me_ == 0 )
  {
    me_ = new User( config_->nickName, config_->nickName, "__local__", this );
  }

  return me_;
}
