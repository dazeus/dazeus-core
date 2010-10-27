/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#include <QtCore/QTimer>

#include "davinci.h"
#include "database.h"
#include "network.h"
#include "config.h"
#include "plugins/pluginmanager.h"

// #define DEBUG

/**
 * @brief Constructor.
 *
 * Initialises the object, and loads configuration if a path was given.
 * @param configFileName optional path to the configuration file.
 */
DaVinci::DaVinci( QString configFileName )
: QObject()
, config_( 0 )
, configFileName_( configFileName )
, pluginManager_( 0 )
, database_( 0 )
{
  if( !configFileName_.isEmpty() )
    loadConfig();
}


/**
 * @brief Destructor.
 */
DaVinci::~DaVinci()
{
  foreach( Network *n, networks_ )
  {
    n->disconnectFromNetwork( Network::ShutdownReason );
    n->deleteLater();
  }
  networks_.clear();

  resetConfig();
  delete pluginManager_;
}


/**
 * @brief Connect to all networks marked "autoconnect".
 * 
 * Warning: This method is usually called outside the event loop, just after
 * initialisation.
 */
void DaVinci::autoConnect()
{
#ifdef DEBUG
  qDebug() << "DaVinci::autoConnect() called: looking for networks to connect to";
#endif
  foreach( Network *n, networks_ )
  {
    if( n->autoConnectEnabled() )
    {
#ifdef DEBUG
      qDebug() << "Connecting to " << n << " (autoconnect is enabled)";
#endif
      n->connectToNetwork();
    }
#ifdef DEBUG
    else
    {
      qDebug() << "Not connecting to " << n << ", autoconnect is disabled.";
    }
#endif
  }
}


/**
 * @brief Returns whether the configuration is loaded.
 */
bool DaVinci::configLoaded() const
{
  return config_ != 0;
}


/**
 * @brief Start connecting to the database.
 *
 * Does nothing if already connected.
 */
bool DaVinci::connectDatabase()
{
  const DatabaseConfig *dbc = config_->databaseConfig();
  database_ = Database::fromConfig(dbc);

  if( !database_->open() )
  {
    qWarning() << "Could not connect to database: " << database_->lastError();
    return false;
  }

  // create it if it doesn't exist yet
  if( !database_->createTable() )
  {
    qWarning() << "Could not create table: " << database_->lastError();
    return false;
  }

  return true;
}

/**
 * @brief Return the database.
 */
Database *DaVinci::database() const
{
  return database_;
}

/**
 * @brief We received the IRC 'welcomed' message from a server.
 *
 * Give a signal through to the plugin manager.
 */
void DaVinci::welcomed()
{
  Network *n = qobject_cast<Network*>(sender());
  Q_ASSERT( n != 0 );
  pluginManager_->welcomed( *n );
}



/**
 * @brief The application connected.
 *
 * Give a signal through to the plugin manager.
 */
void DaVinci::connected()
{
  Network *n = qobject_cast<Network*>(sender());
  Q_ASSERT( n != 0 );
  const Server *s = n->activeServer();
  Q_ASSERT( s != 0 );
  pluginManager_->connected( *n, *s );
}



/**
 * @brief The application disconnected.
 *
 * Give a signal through to the plugin manager.
 */
void DaVinci::disconnected()
{
  Network *n = qobject_cast<Network*>(sender());
  Q_ASSERT( n != 0 );
  pluginManager_->disconnected( *n );
}



/**
 * @brief Initialises plugins from the configuration file.
 */
bool DaVinci::initPlugins()
{
  if( pluginManager_->isInitialized() )
    return true;

  pluginManager_->setConfig( config_ );
  if( !pluginManager_->initialize() ) {
    delete pluginManager_;
    return false;
  }

  return true;
}


/**
 * @brief (Re-)loads configuration from the configuration file.
 *
 * If loading failed, this method returns false.
 * Note: this method is automatically called when setConfigFileName is called,
 * and by the constructor.
 */
bool DaVinci::loadConfig()
{
  // Clean up this object.
  resetConfig();

  Q_ASSERT( !configFileName_.isEmpty() );

  if( config_ == 0 )
    config_ = new Config();

  config_->loadFromFile( configFileName_ );

  if( !config_ )
    return false;

  const QList<NetworkConfig*> &networks = config_->networks();

  if(!connectDatabase())
    return false;
  
  pluginManager_ = new PluginManager( database_ );

  foreach( NetworkConfig *netconf, networks )
  {
    Network *net = Network::fromNetworkConfig( netconf );
    if( net == 0 ) {
      resetConfig();
      return false;
    }

    connect( net,  SIGNAL(    connected() ),
             this, SLOT(      connected() ) );
    connect( net,  SIGNAL( disconnected() ),
             this, SLOT(   disconnected() ) );
    connect( net,  SIGNAL(     welcomed() ),
             this, SLOT(       welcomed() ) );

#define RELAY_NET_SIGN(sign) \
    connect( net,            SIGNAL( sign ), \
             pluginManager_, SLOT(   sign ) )

    RELAY_NET_SIGN( motdReceived( const QString&, Irc::Buffer* ));
    RELAY_NET_SIGN( joined( const QString&, Irc::Buffer* ) );
    RELAY_NET_SIGN( parted( const QString&, const QString&,
                          Irc::Buffer* ) );
    RELAY_NET_SIGN( quit(   const QString&, const QString&,
                          Irc::Buffer* ) );
    RELAY_NET_SIGN( nickChanged( const QString&, const QString&,
                          Irc::Buffer* ) );
    RELAY_NET_SIGN( modeChanged( const QString&, const QString&,
                          const QString&, Irc::Buffer* ) );
    RELAY_NET_SIGN( topicChanged( const QString&, const QString&,
                          Irc::Buffer* ) );
    RELAY_NET_SIGN( invited( const QString&, const QString&,
                          const QString&, Irc::Buffer* ) );
    RELAY_NET_SIGN( kicked( const QString&, const QString&,
                          const QString&, Irc::Buffer* ) );
    RELAY_NET_SIGN( messageReceived( const QString&,
                          const QString&, Irc::Buffer* ) );
    RELAY_NET_SIGN( noticeReceived( const QString&,
                          const QString&, Irc::Buffer* ) );
    RELAY_NET_SIGN( ctcpRequestReceived( const QString&,
                          const QString&, Irc::Buffer* ) );
    RELAY_NET_SIGN( ctcpReplyReceived( const QString&,
                          const QString&, Irc::Buffer* ) );
    RELAY_NET_SIGN( ctcpActionReceived( const QString&,
                          const QString&, Irc::Buffer* ) );
    RELAY_NET_SIGN( numericMessageReceived( const QString&, uint,
                          const QStringList&, Irc::Buffer* ) );
    RELAY_NET_SIGN( unknownMessageReceived( const QString&,
                          const QStringList&, Irc::Buffer* ) );

#undef RELAY_NET_SIGN

    networks_.append( net );
  }

  return true;
}

/**
 * @brief Resets everything from the configuration of this object.
 *
 * Destroys the config_ object if it's already there, resets networks, servers,
 * et cetera.
 * This method will not clear the configFileName_. It is always called when
 * loadConfig() is called.
 */
void DaVinci::resetConfig()
{
  //delete config_;
  //config_ = 0;

  //foreach( Network *net, networks_ )
  //{
    //net->disconnect( Network::ConfigurationReloadReason );
    //net->deleteLater();
  //}
  //networks_.clear();
}
