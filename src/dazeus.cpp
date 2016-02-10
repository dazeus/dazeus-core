/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#include "dazeus.h"
#include "db/database.h"
#include "db/factory.h"
#include "network.h"
#include "config.h"
#include "plugincomm.h"
#include "pluginmonitor.h"
#include <cassert>
#include <sstream>
#include <iostream>

using dazeus::db::DatabaseConfig;
using dazeus::db::Database;
using dazeus::db::Factory;

// #define DEBUG
#define VERBOSE

/**
 * @brief Constructor.
 *
 * @param configFileName Path to the configuration file.
 */
dazeus::DaZeus::DaZeus( std::string configFileName )
: config_(std::make_shared<ConfigReader>())
, configFileName_( configFileName )
, plugins_( 0 )
, plugin_monitor_( 0 )
, database_( 0 )
, networks_()
, running_(false)
, config_reload_pending_(true)
{
}


/**
 * @brief Destructor.
 */
dazeus::DaZeus::~DaZeus()
{
  for(auto it = networks_.begin(); it != networks_.end(); ++it)
  {
    it->second->disconnectFromNetwork( Network::ShutdownReason );
    delete it->second;
  }
  networks_.clear();

  delete plugin_monitor_;
  delete plugins_;
  delete database_;
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
 * @brief (Re-)connect to the database.
 */
bool dazeus::DaZeus::connectDatabase()
{
  assert(config_->isRead());
  if(database_) {
    delete database_;
  }
  try {
    database_ = Factory::createDb(config_->getDatabaseConfig());
  } catch (dazeus::db::exception &e) {
    std::cout << e.what() << std::endl;
    return false;
  }

  try {
    database_->open();
  } catch(const dazeus::db::exception &e) {
    std::cout << "Could not connect to database: " << e.what() << std::endl;
    return false;
  }
	return true;
}

/**
 * @brief Return the database.
 */
Database *dazeus::DaZeus::database() const
{
  return database_;
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
  assert(config_);

  try {
    config_->read(configFileName_);
  } catch(ConfigReader::exception &e) {
    std::cerr << "Failed to read configuration: " << e.what() << std::endl;
    return false;
  }
  assert(config_->isRead());

  if(!connectDatabase()) {
    return false;
  }

  if(plugins_) {
    plugins_->setDatabase(database_);
    // TODO: update socketconfig in PluginComm without breaking existing sockets
  } else {
    plugins_ = new PluginComm( database_, config_, this );
    plugins_->init();
  }

  if(plugin_monitor_) {
    plugin_monitor_->configReloaded();
  } else {
    plugin_monitor_ = new PluginMonitor(config_);
  }

  const std::vector<NetworkConfig> &networks = config_->getNetworks();
  for(auto it = networks.begin(); it != networks.end(); ++it)
  {
    std::string name = it->name;
    auto network = networks_.find(name);
    if(network == networks_.end()) {
      Network *net = new Network(*it);
      net->addListener(plugins_);
      networks_[name] = net;

      if(net->autoConnectEnabled()) {
        net->connectToNetwork();
      }
    } else {
      network->second->resetConfig(*it);
    }
  }

  // Disconnect from networks that are gone.
  for(auto nit = networks_.begin(); nit != networks_.end();) {
    std::string name = nit->first;
    bool found = false;
    for(auto it = networks.begin(); it != networks.end(); ++it) {
      if(it->name == name) {
        found = true;
        break;
      }
    }
    if(!found) {
      Network *net = nit->second;
      networks_.erase(nit++);
      net->disconnectFromNetwork(Network::ConfigurationReloadReason);
      delete net;
    } else {
      ++nit;
    }
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

void dazeus::DaZeus::sigchild()
{
	plugin_monitor_->sigchild();
}

void dazeus::DaZeus::stop()
{
	running_ = false;
}

void dazeus::DaZeus::run()
{
	running_ = true;
	while(running_) {
		if(config_reload_pending_) {
			config_reload_pending_ = false;
			loadConfig();
		}
		plugin_monitor_->runOnce();
		// The only non-socket processing in DaZeus is done by the
		// plugin monitor. It works using signals (primarily SIGCHLD),
		// which already interrupt select(). However, its timing in
		// re-starting plugins has a granularity of 5 seconds, so if it
		// is waiting to restart a plugin, we will decrease our timeout
		// length to once every second. If it is in a normal state, we
		// can use any granularity we want.
		plugins_->run(plugin_monitor_->shouldRun() ? 1 : 30);
	}
}

void dazeus::DaZeus::setConfigFileName(std::string filename) {
	configFileName_ = filename;
}
