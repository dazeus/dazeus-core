/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#include "dazeus.h"
#include "database.h"
#include "network.h"
#include "config.h"
#include "plugincomm.h"
#include <cassert>

// #define DEBUG
#define VERBOSE

/**
 * @brief Constructor.
 *
 * Initialises the object, and loads configuration if a path was given.
 * @param configFileName optional path to the configuration file.
 */
DaZeus::DaZeus( std::string configFileName )
: config_( 0 )
, configFileName_( configFileName )
, plugins_( 0 )
, database_( 0 )
{
  if( configFileName_.length() != 0 )
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
  printf("Initialising DaZeus for the %d%s time!\n", numInits, suffix);
}


/**
 * @brief Destructor.
 */
DaZeus::~DaZeus()
{
  foreach( Network *n, networks_ )
  {
    n->disconnectFromNetwork( Network::ShutdownReason );
    delete n;
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
  fprintf(stderr, "DaZeus::autoConnect() called: looking for networks to connect to\n");
#endif
  foreach( Network *n, networks_ )
  {
    if( n->autoConnectEnabled() )
    {
#ifdef DEBUG
      // TODO: toString
      fprintf(stderr, "Connecting to %p (autoconnect is enabled)\n", n);
#endif
      n->connectToNetwork();
    }
#ifdef DEBUG
    else
    {
      // TODO: toString
      fprintf(stderr, "Not connecting to %p, autoconnect is disabled.\n", n);
    }
#endif
  }
}


std::string DaZeus::configFileName() const {
	return configFileName_;
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
  database_ = new Database(dbc->hostname, dbc->port);

  if( !database_->open() )
  {
    fprintf(stderr, "Could not connect to database: %s\n", database_->lastError().c_str());
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

  assert( configFileName_.length() != 0 );

  if( config_ == 0 )
    config_ = new Config();

  config_->loadFromFile( configFileName_ );

  if( !config_ )
    return false;

  const std::list<NetworkConfig*> &networks = config_->networks();

  if(!connectDatabase())
    return false;

  plugins_ = new PluginComm( database_, config_, this );

  foreach( NetworkConfig *netconf, networks )
  {
    Network *net = new Network( netconf, plugins_ );
    if( net == 0 ) {
      resetConfig();
      return false;
    }

    networks_.push_back( net );
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

void DaZeus::run()
{
	while(1) {
		plugins_->run();
	}
}

void DaZeus::setConfigFileName(std::string filename) {
	configFileName_ = filename;
}
