/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#include "config.h"
#include "database.h"

#include <QtCore/QFile>
#include <QtCore/QDebug>
#include <QtCore/QStringList>
#include <QtSql/QSqlDatabase>

// #define DEBUG

#define rw(x) QLatin1String(x)

/**
 * @brief Constructor
 *
 * This constructor sets default configuration. A new Config object is usable,
 * but will not contain any networks or servers.
 */
Config::Config()
: QObject()
, error_( QString() )
, settings_( 0 )
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
const QMap<QString,QVariant> Config::groupConfig(QString group) const
{
	QMap<QString,QVariant> configuration;
	Q_ASSERT(settings_);

	if(group.length() == 0)
		return configuration;

	bool found = false;
	foreach(const QString &g, settings_->childGroups()) {
		if(g == group) {
			found = true;
			break;
		}
	}

	if(!found)
		return configuration;

	settings_->beginGroup(group);
	foreach(const QString &key, settings_->childKeys()) {
		configuration[key] = settings_->value(key);
	}
	settings_->endGroup();

	return configuration;
}

/**
 * @brief Returns the last error triggered by this class.
 *
 * If there was no error, returns an empty string.
 */
const QString &Config::lastError()
{
  return error_;
}

/**
 * @brief Load configuration from file.
 *
 * This method attempts to load all contents from the given file to the Config
 * object, then returns whether that worked or not.
 */
bool Config::loadFromFile( QString fileName )
{
  // TODO do this properly in reset()!
  oldNetworks_ = networks_;
  networks_.clear();
  error_.clear();
  delete settings_;
  settings_ = 0;

  // load!
  error_.clear();
  if( !QFile::exists(fileName) )
  {
    error_ = rw("Configuration file does not exist: ") + fileName;
  }
  if( error_.isEmpty() )
  {
    settings_ = new QSettings(fileName, QSettings::IniFormat);
    if( settings_->status() != QSettings::NoError )
    {
      error_ = rw("Could not read configuration file: ") + fileName;
    }
  }

  if( !error_.isEmpty() )
  {
    qWarning() << "Error: " << error_;
    delete settings_;
    settings_ = 0;
    return false;
  }

  return true;
}

const QList<NetworkConfig*> &Config::networks()
{
  Q_ASSERT( settings_ != 0 );
  // TODO remove later
  if( networks_.count() > 0 )
    return networks_;

  // Database settings
  bool valid = true;
  settings_->beginGroup(rw("database"));
  DatabaseConfig *dbc = new DatabaseConfig;
  dbc->type     = settings_->value(rw("type")).toString();
  dbc->hostname = settings_->value(rw("hostname")).toString();
  QString dbRawPort = settings_->value(rw("port")).toString();
  dbc->port     = dbRawPort.isEmpty() ? 0 : dbRawPort.toUInt(&valid);
  dbc->username = settings_->value(rw("username")).toString();
  dbc->password = settings_->value(rw("password")).toString();
  dbc->database = settings_->value(rw("database")).toString();
  dbc->options  = settings_->value(rw("options")).toString();

  if(!valid) {
    qWarning() << "Database port is not a valid number: " << dbRawPort;
    qWarning() << "Assuming default port.";
    dbc->port = 0;
  }

  if( !QSqlDatabase::isDriverAvailable( Database::typeToQtPlugin(dbc->type) )) {
    qWarning() << "No Qt plugin loaded for database type " << dbc->type;
    qWarning() << "You will likely get database errors later.";
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
        qWarning() << "Max network name length is 50. " << networkName;
        networkName = networkName.left(50);
      }
#ifdef DEBUG
      qDebug() << "Network: " << networkName;
#endif
      if( servers.contains(networkName) )
      {
        qWarning() << "Warning: Two network blocks for " << networkName
                   << "exist in your configuration file."
                      " Behaviour is undefined.";
      }

      NetworkConfig *nc = new NetworkConfig;
      nc->name          = networkName;
      nc->displayName   = settings_->value(category + rw("/displayname"),
                                           networkName).toString();
      nc->autoConnect   = settings_->value(category + rw("/autoconnect"),
                                           false).toBool();
      nc->nickName      = settings_->value(category + rw("/nickname"),
                                           defaultNickname).toString();
      nc->userName      = settings_->value(category + rw("/username"),
                                           defaultUsername).toString();
      nc->fullName      = settings_->value(category + rw("/fullname"),
                                           defaultFullname).toString();
      nc->password      = settings_->value(category + rw("/password"),
                                           QString()).toString();

      networks.insert(networkName, nc);
    }
    else if( category.toLower().startsWith(rw("server")) )
    {
      QString networkName = category.mid(7).toLower();

      if( !networks.contains(networkName) )
      {
        qWarning() << "Warning: Server block for " << networkName
                   << "exists, but no network block found yet. Ignoring block.";
        continue;
      }

      ServerConfig *sc = new ServerConfig;
      sc->host         = settings_->value(category + rw("/host"),QString()).toString();
      sc->port         = settings_->value(category + rw("/port"),6667).toInt();
      sc->priority     = settings_->value(category + rw("/priority"), 5).toInt();
      sc->ssl          = settings_->value(category + rw("/ssl"), false).toBool();
      sc->network      = networks[networkName];
      networks[networkName]->servers.append( sc );

#ifdef DEBUG
      qDebug() << "Server for network: " << networkName;
#endif
    }
    else if( category.toLower() != rw("generic")
          && category.toLower() != rw("database")
          && category.toLower() != rw("sockets")
          && !category.toLower().startsWith(rw("plugin")) )
    {
      qWarning() << "Warning: Configuration category name not recognized: "
                 << category;
    }
  }

  QHashIterator<QString,NetworkConfig*> i(networks);
  while(i.hasNext())
  {
    i.next();
    if( i.value()->servers.count() == 0 )
    {
      qWarning() << "Warning: Network block for " << i.key()
                 << " exists, but no server block found. Ignoring block.";
      continue;
    }
  }

  networks_ = networks.values();

  return networks_;
}
