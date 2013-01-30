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
#include <sstream>

// #define DEBUG
#define VERBOSE

/**
 * @brief Constructor.
 *
 * @param configFileName optional path to the configuration file. Configuration
 * will not be loaded automatically, use loadConfig() for that.
 */
dazeus::DaZeus::DaZeus( std::string configFileName )
: config_( 0 )
, configFileName_( configFileName )
, plugins_( 0 )
, database_( 0 )
, networks_()
{
}


/**
 * @brief Destructor.
 */
dazeus::DaZeus::~DaZeus()
{
  std::vector<Network*>::iterator it;
  for(it = networks_.begin(); it != networks_.end(); ++it)
  {
    (*it)->disconnectFromNetwork( Network::ShutdownReason );
    delete *it;
  }
  networks_.clear();

  delete plugins_;
  delete config_;
}


/**
 * @brief Connect to all networks marked "autoconnect".
 * 
 * Warning: This method is usually called outside the event loop, just after
 * initialisation.
 */
void dazeus::DaZeus::autoConnect()
{
#ifdef DEBUG
  fprintf(stderr, "dazeus::DaZeus::autoConnect() called: looking for networks to connect to\n");
#endif
  std::vector<Network*>::iterator it;
  for(it = networks_.begin(); it != networks_.end(); ++it)
  {
    Network *n = *it;
    if( n->autoConnectEnabled() )
    {
#ifdef DEBUG
      fprintf(stderr, "Connecting to %s (autoconnect is enabled)\n", Network::toString(n).c_str());
#endif
      n->connectToNetwork();
    }
#ifdef DEBUG
    else
    {
      fprintf(stderr, "Not connecting to %s, autoconnect is disabled.\n", Network::toString(n).c_str());
    }
#endif
  }
}


std::string dazeus::DaZeus::configFileName() const {
	return configFileName_;
}

/**
 * @brief Returns whether the configuration is loaded.
 */
bool dazeus::DaZeus::configLoaded() const
{
  return config_->isRead();
}


/**
 * @brief Start connecting to the database.
 *
 * Does nothing if already connected.
 */
bool dazeus::DaZeus::connectDatabase()
{
  const DatabaseConfig *dbc = config_->getDatabaseConfig();
  if(dbc == 0) {
    fprintf(stderr, "Database configuration is absent, cannot continue.\n");
    return false;
  }
  database_ = new Database(dbc->hostname, dbc->port, dbc->database, dbc->username, dbc->password);

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
dazeus::Database *dazeus::DaZeus::database() const
{
  return database_;
}



/**
 * @brief Initialises plugins from the configuration file.
 */
bool dazeus::DaZeus::initPlugins()
{
  assert(plugins_ != 0);
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
bool dazeus::DaZeus::loadConfig()
{
  assert( configFileName_.length() != 0 );

  if( config_ != 0 ) {
    delete config_;
  }

  config_ = new ConfigReader(configFileName_);

  if( !config_ )
    return false;

  config_->read();

  const std::vector<NetworkConfig*> &networks = config_->getNetworks();

  if(!connectDatabase()) {
    delete config_;
    config_ = 0;
    return false;
  }

  if(plugins_)
    delete plugins_;
  plugins_ = new PluginComm( database_, config_, this );

  std::vector<NetworkConfig*>::const_iterator it;
  for(it = networks.begin(); it != networks.end(); ++it)
  {
    Network *net = new Network( *it );
    net->addListener(plugins_);
    if( net == 0 ) {
      delete config_;
      config_ = 0;
      return false;
    }

    networks_.push_back( net );
  }

  // Pretty number of initialisations viewer -- and also an immediate database
  // check.
  std::stringstream numInitsStr;
  numInitsStr << database_->property("dazeus.numinits");
  int numInits;
  numInitsStr >> numInits;
  if(!numInitsStr)
    numInits = 0;
  ++numInits;
  numInitsStr.str(""); numInitsStr.clear();
  numInitsStr << numInits;
  database_->setProperty("dazeus.numinits", numInitsStr.str());
  const char *suffix = "th";
  if(numInits%100 == 11 ) ;
  else if(numInits%100 == 12) ;
  else if(numInits%100 == 13) ;
  else if(numInits%10 == 1) suffix = "st";
  else if(numInits%10 == 2) suffix = "nd";
  else if(numInits%10 == 3) suffix = "rd";
  printf("Initialising DaZeus for the %d%s time!\n", numInits, suffix);
  return true;
}

void dazeus::DaZeus::run()
{
	while(1) {
		plugins_->run();
	}
}

void dazeus::DaZeus::setConfigFileName(std::string filename) {
	configFileName_ = filename;
}
