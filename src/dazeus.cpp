/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#include <QtCore/QTimer>

#include "dazeus.h"
#include "database.h"
#include "network.h"
#include "config.h"
#include "plugincomm.h"

// #define DEBUG
#define VERBOSE

/**
 * @brief Constructor.
 *
 * Initialises the object, and loads configuration if a path was given.
 * @param configFileName optional path to the configuration file.
 */
DaZeus::DaZeus( QString configFileName )
: QObject()
, config_( 0 )
, configFileName_( configFileName )
, plugins_( 0 )
, database_( 0 )
{
  if( !configFileName_.isEmpty() )
    loadConfig();

  // Pretty number of initialisations viewer -- and also an immediate database
  // check.
  int numInits = database_->property("dazeus.numinits").toInt();
  ++numInits;
  database_->setProperty("dazeus.numinits", numInits);
  const char *suffix = "th";
  if(numInits%100 == 11 ) ;
  else if(numInits%100 == 12) ;
  else if(numInits%100 == 13) ;
  else if(numInits%10 == 1) suffix = "st";
  else if(numInits%10 == 2) suffix = "nd";
  else if(numInits%10 == 3) suffix = "rd";
  qDebug() << "Initialising DaZeus for the " << numInits << suffix << " time!";
}


/**
 * @brief Destructor.
 */
DaZeus::~DaZeus()
{
  foreach( Network *n, networks_ )
  {
    n->disconnectFromNetwork( Network::ShutdownReason );
    n->deleteLater();
  }
  networks_.clear();

  resetConfig();
  delete plugins_;
}


/**
 * @brief Connect to all networks marked "autoconnect".
 * 
 * Warning: This method is usually called outside the event loop, just after
 * initialisation.
 */
void DaZeus::autoConnect()
{
#ifdef DEBUG
  qDebug() << "DaZeus::autoConnect() called: looking for networks to connect to";
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
bool DaZeus::configLoaded() const
{
  return config_ != 0;
}


/**
 * @brief Start connecting to the database.
 *
 * Does nothing if already connected.
 */
bool DaZeus::connectDatabase()
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
Database *DaZeus::database() const
{
  return database_;
}



/**
 * @brief Initialises plugins from the configuration file.
 */
bool DaZeus::initPlugins()
{
  plugins_->init();

  return true;
}


/**
 * @brief (Re-)loads configuration from the configuration file.
 *
 * If loading failed, this method returns false.
 * Note: this method is automatically called when setConfigFileName is called,
 * and by the constructor.
 */
bool DaZeus::loadConfig()
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

  plugins_ = new PluginComm( database_, config_, this );

  foreach( NetworkConfig *netconf, networks )
  {
    Network *net = Network::fromNetworkConfig( netconf );
    if( net == 0 ) {
      resetConfig();
      return false;
    }

#define RELAY_NET_SIGN(sign) \
    connect( net,            SIGNAL( sign ),   \
             plugins_,    SLOT(   sign ) );

    RELAY_NET_SIGN( motdReceived( const QString&, Irc::Buffer* ));
    RELAY_NET_SIGN( invited( const QString&, const QString&,
                          const QString&, Irc::Buffer* ) );
    RELAY_NET_SIGN( kicked( const QString&, const QString&,
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
    RELAY_NET_SIGN( ircEvent( const QString&, const QString&,
                          const QStringList&, Irc::Buffer*) );
    RELAY_NET_SIGN( whoisReceived( const QString&, const QString&,
                          bool, Irc::Buffer* ) );
    RELAY_NET_SIGN( namesReceived( const QString&, const QString&,
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
void DaZeus::resetConfig()
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
