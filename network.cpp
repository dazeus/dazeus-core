/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#include "network.h"
#include "server.h"
#include "config.h"
#include "user.h"

QHash<QString, Network*> Network::networks_;

QDebug operator<<(QDebug dbg, const Network &n)
{
  return operator<<( dbg, &n );
}

QDebug operator<<(QDebug dbg, const Network *n)
{
  dbg.nospace() << "Network[";
  if( n == 0 ) {
    dbg.nospace() << "0";
  } else {
    const NetworkConfig *nc = n->config();
    dbg.nospace() << nc->displayName << "," << n->activeServer();
  }
  dbg.nospace() << "]";

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
  return activeServer_->ctcpAction( destination, message );
}

/**
 * @brief Chooses one server or the other based on the priority and failure
 *        rate.
 * Failure rate is recorded in the Network object that the ServerConfig object
 * links to via the NetworkConfig, if there is such a NetworkConfig object.
 * You must give two servers of the same Network.
 */
bool serverNicenessLessThan( const ServerConfig *c1, const ServerConfig *c2 )
{
  int var1 = c1->priority;
  int var2 = c2->priority;

  Network *n;

  // if no network is configured for either, we're done
  if( c1->network == 0 || c2->network == 0 )
    goto ready;

  // otherwise, look at server undesirability (failure rate) too
  n = Network::fromNetworkConfig( c1->network );
  Q_ASSERT( n == Network::fromNetworkConfig( c2->network ) );

  var1 -= n->serverUndesirability( c1 );
  var2 -= n->serverUndesirability( c2 );

ready:
  // If the priorities are equal, randomise the order.
  if( var1 == var2 )
    return (qrand() % 2) == 0 ? -1 : 1;
  return var1 < var2;
}

const NetworkConfig *Network::config() const
{
  return config_;
}

void Network::connectToNetwork( bool reconnect )
{
  if( !reconnect && activeServer_ )
    return;

  qDebug() << "Connecting to network: " << this;

  // Check if there *is* a server to use
  if( servers().count() == 0 )
  {
    qWarning() << "Trying to connect to network" << config_->displayName
               << "but there are no servers to connect to!";
    return;
  }

  // Find the best server to use

  // First, sort the list by priority and earlier failures
  QList<ServerConfig*> sortedServers = servers();
  qSort( sortedServers.begin(), sortedServers.end(), serverNicenessLessThan );

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
           this,          SLOT(  onFailedConnection() ) );
  connect( activeServer_, SIGNAL( connectionTimeout() ),
           this,          SLOT(  onFailedConnection() ) );
  connect( activeServer_, SIGNAL(         destroyed() ),
           this,          SLOT(  onFailedConnection() ) );
  connect( activeServer_, SIGNAL(          welcomed() ),
           this,          SLOT(serverIsActuallyOkay() ) );
  activeServer_->connectToServer();
}


void Network::onFailedConnection()
{
  Q_ASSERT( sender() == activeServer_ );

  qDebug() << "Connection failed on " << this;

  // Flag old server as undesirable
  flagUndesirableServer( activeServer_->config() );
  disconnect( activeServer_, 0, 0, 0 );
  activeServer_ = 0;

  connectToNetwork();
}


void Network::ctcp( QString destination, QString message )
{
  if( !activeServer_ )
    return;
  return activeServer_->ctcpAction( destination, message );
}


/**
 * @brief Disconnect from the network.
 */
void Network::disconnectFromNetwork( DisconnectReason reason )
{
  if( activeServer_ == 0 )
    return;

  // Disconnect the activeServer first, so its disconnected() signal will not
  // trigger the reconnection in this class
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
  return activeServer_->join( channel );
}


void Network::leaveChannel( QString channel )
{
  if( !activeServer_ )
    return;
  activeServer_->part( channel );
}


void Network::say( QString destination, QString message )
{
  if( !activeServer_ )
    return;
  return activeServer_->message( destination, message );
}


void Network::sendWhois( QString destination )
{
  activeServer_->whois(destination);
}

const QList<ServerConfig*> &Network::servers() const
{
  return config_->servers;
}


User *Network::user()
{
  if( me_ == 0 )
  {
    me_ = new User( config_->nickName, config_->nickName,
                    QLatin1String("__local__"), this );
  }

  return me_;
}



QString Network::networkName() const
{
  return config()->name;
}



int Network::serverUndesirability( const ServerConfig *sc ) const
{
  return undesirables_.contains(sc) ? undesirables_.value(sc) : 0;
}

void Network::flagUndesirableServer( const ServerConfig *sc )
{
  if( undesirables_.contains(sc) )
    undesirables_.insert( sc, undesirables_.value(sc) + 1 );
  undesirables_.insert( sc, 0 );
}

void Network::serverIsActuallyOkay()
{
  Q_ASSERT( activeServer_ != 0 );
  serverIsActuallyOkay( activeServer_->config() );
}

void Network::serverIsActuallyOkay( const ServerConfig *sc )
{
  undesirables_.remove(sc);
}
