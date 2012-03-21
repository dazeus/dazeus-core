/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#include <QtCore/QSettings>
#include <fstream>
#include <sstream>
#include <assert.h>
#include "config.h"
#include "database.h"

// #define DEBUG

#define rw(x) QLatin1String(x)

/**
 * @brief Constructor
 *
 * This constructor sets default configuration. A new Config object is usable,
 * but will not contain any networks or servers.
 */
Config::Config()
: settings_( 0 )
, databaseConfig_( 0 )
{
}

/**
 * @brief Destructor
 */
Config::~Config()
{
  delete settings_;
}


/**
 * @brief Return database configuration.
 *
 * Currently, you *must* call networks() before calling databaseConfig().
 * This is considered a bug.
 */
const DatabaseConfig *Config::databaseConfig() const
{
  return databaseConfig_;
}

/**
 * @brief Returns configuration for a given group.
 *
 * Returns an empty list if no special configuration was found in the
 * configuration file. Currently, you *must* call networks() before calling
 * this method. This is considered a bug.
 */
const std::map<std::string,std::string> Config::groupConfig(std::string group) const
{
	std::map<std::string,std::string> configuration;
	assert(settings_);

	if(group.length() == 0)
		return configuration;

	bool found = false;
	foreach(const QString &g, settings_->childGroups()) {
		if(g.toStdString() == group) {
			found = true;
			break;
		}
	}

	if(!found)
		return configuration;

	settings_->beginGroup(QString::fromStdString(group));
	foreach(const QString &key, settings_->childKeys()) {
		configuration[key.toStdString()] = settings_->value(key).toString().toStdString();
	}
	settings_->endGroup();

	return configuration;
}

/**
 * @brief Returns the last error triggered by this class.
 *
 * If there was no error, returns an empty string.
 */
const std::string &Config::lastError()
{
  return error_;
}

/**
 * @brief Load configuration from file.
 *
 * This method attempts to load all contents from the given file to the Config
 * object, then returns whether that worked or not.
 */
bool Config::loadFromFile( std::string fileName )
{
  // TODO do this properly in reset()!
  oldNetworks_ = networks_;
  networks_.clear();
  error_.clear();
  delete settings_;
  settings_ = 0;

  // load!
  error_.clear();
  std::ifstream ifile(fileName.c_str());
  if(!ifile) {
    error_ = "Configuration file does not exist or is unreadable: " + fileName;
  }
  if( error_.length() == 0 )
  {
    settings_ = new QSettings(QString::fromStdString(fileName), QSettings::IniFormat);
    if( settings_->status() != QSettings::NoError )
    {
      error_ = "Could not read configuration file: " + fileName;
    }
  }

  if( error_.length() != 0 )
  {
    fprintf(stderr, "Error: %s\n", error_.c_str());
    delete settings_;
    settings_ = 0;
    return false;
  }

  return true;
}

const std::list<NetworkConfig*> &Config::networks()
{
  assert( settings_ != 0 );
  // TODO remove later
  if( networks_.size() > 0 )
    return networks_;

  // Database settings
  settings_->beginGroup(rw("database"));
  DatabaseConfig *dbc = new DatabaseConfig;
  dbc->type     = settings_->value(rw("type")).toString().toStdString();
  dbc->hostname = settings_->value(rw("hostname")).toString().toStdString();
  std::string dbRawPort = settings_->value(rw("port")).toString().toStdString();
  std::stringstream portStr;
  portStr << dbRawPort;
  portStr >> dbc->port;
  dbc->username = settings_->value(rw("username")).toString().toStdString();
  dbc->password = settings_->value(rw("password")).toString().toStdString();
  dbc->database = settings_->value(rw("database")).toString().toStdString();
  dbc->options  = settings_->value(rw("options")).toString().toStdString();

  if(!portStr) {
    fprintf(stderr, "Database port is not a valid numer: %s\n", dbRawPort.c_str());
    fprintf(stderr, "Assuming default port.\n");
    dbc->port = 0;
  }

  if( dbc->type != "mongo") {
    fprintf(stderr, "No support loaded for database type %s\n", dbc->type.c_str());
    fprintf(stderr, "Currently, only mongo is supported.\n");
  }

  delete databaseConfig_;
  databaseConfig_ = dbc;
  settings_->endGroup();

  QString defaultNickname = settings_->value(rw("nickname")).toString();
  QString defaultUsername = settings_->value(rw("username")).toString();
  QString defaultFullname = settings_->value(rw("fullname")).toString();

  QHash<QString,NetworkConfig*> networks;
  QMultiHash<QString,ServerConfig*> servers;
  foreach( const QString &category, settings_->childGroups() )
  {
    if( category.toLower().startsWith(rw("network")) )
    {
      QString networkName = category.mid(8).toLower();
      if( networkName.length() > 50 )
      {
        fprintf(stderr, "Max network name length is 50. %s\n", networkName.toUtf8().constData());
        networkName = networkName.left(50);
      }
#ifdef DEBUG
      fprintf(stderr, "Network: %s\n", networkName.toUtf8().constData());
#endif
      if( servers.contains(networkName) )
      {
        fprintf(stderr, "Warning: Two network blocks for network %s exist in your configuration file. Behaviour is undefined.\n", networkName.toUtf8().constData());
      }

      NetworkConfig *nc = new NetworkConfig;
      nc->name          = networkName.toStdString();
      nc->displayName   = settings_->value(category + rw("/displayname"),
                                           networkName).toString().toStdString();
      nc->autoConnect   = settings_->value(category + rw("/autoconnect"),
                                           false).toBool();
      nc->nickName      = settings_->value(category + rw("/nickname"),
                                           defaultNickname).toString().toStdString();
      nc->userName      = settings_->value(category + rw("/username"),
                                           defaultUsername).toString().toStdString();
      nc->fullName      = settings_->value(category + rw("/fullname"),
                                           defaultFullname).toString().toStdString();
      nc->password      = settings_->value(category + rw("/password"),
                                           QString()).toString().toStdString();

      networks.insert(networkName, nc);
    }
    else if( category.toLower().startsWith(rw("server")) )
    {
      QString networkName = category.mid(7).toLower();

      if( !networks.contains(networkName) )
      {
        fprintf(stderr, "Warning: Server block for network %s exists, but no network block found yet. Ignoring block.", networkName.toUtf8().constData());
        continue;
      }

      ServerConfig *sc = new ServerConfig;
      sc->host         = settings_->value(category + rw("/host"),QString()).toString().toStdString();
      sc->port         = settings_->value(category + rw("/port"),6667).toInt();
      sc->priority     = settings_->value(category + rw("/priority"), 5).toInt();
      sc->ssl          = settings_->value(category + rw("/ssl"), false).toBool();
      sc->network      = networks[networkName];
      networks[networkName]->servers.push_back( sc );

#ifdef DEBUG
      fprintf(stderr, "Server for network: %s\n", networkName.toUtf8().constData());
#endif
    }
    else if( category.toLower() != rw("generic")
          && category.toLower() != rw("database")
          && category.toLower() != rw("sockets")
          && !category.toLower().startsWith(rw("plugin")) )
    {
      fprintf(stderr, "Warning: Configuration category name not recognized: %s\n", category.toUtf8().constData());
    }
  }

  QHashIterator<QString,NetworkConfig*> i(networks);
  while(i.hasNext())
  {
    i.next();
    if( i.value()->servers.size() == 0 )
    {
      fprintf(stderr, "Warning: Network block for %s exists, but no server block found. Ignoring block.\n", i.key().toUtf8().constData());
      continue;
    }
  }

  networks_.clear();
  foreach(NetworkConfig *v, networks.values()) {
    networks_.push_back(v);
  }

  return networks_;
}
