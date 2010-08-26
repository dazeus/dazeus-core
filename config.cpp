/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#include "config.h"

/**
 * @brief Constructor
 *
 * This constructor sets default configuration. A new Config object is usable,
 * but will not contain any networks or servers.
 */
Config::Config()
: QObject()
, nickName_( "DaVinci" )
, error_( "" )
{
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
  oldNetworks_ = networks_;
  networks_.clear();
  error_ = "";

  emit beforeConfigReload();

  // load!
  if( 0 )
  {
    emit configReloadFailed();
    return false;
  }

  emit configReloaded();
  return true;
}

const QList<NetworkConfig*> &Config::networks()
{
  // TODO remove later
  if( networks_.count() > 0 )
    return networks_;
  ServerConfig *sc;
  NetworkConfig *nc;

  // Temporary network config for localhost
  nc = new NetworkConfig;
  nc->name = "localhost";
  nc->displayName = "Localhost";
  nc->autoConnect = true;
  nc->nickName = nickName_;
  sc = new ServerConfig;
  sc->host = "localhost";
  sc->port = 6667;
  sc->priority = 5;
  sc->ssl = false;
  sc->network = nc;
  nc->servers.append( sc );
  networks_.append( nc );

  // Temporary network config for Tweakers
  nc = new NetworkConfig;
  nc->name = "tweakers";
  nc->displayName = "Tweakers";
  nc->autoConnect = true;
  nc->nickName = nickName_;
  sc = new ServerConfig;
  sc->host = "irc.tweakers.net";
  sc->port = 6667;
  sc->priority = 5;
  sc->ssl = false;
  sc->network = nc;
  nc->servers.append( sc );
  networks_.append( nc );

  // Temporary network config for Freenode
  nc = new NetworkConfig;
  nc->name = "freenode";
  nc->displayName = "FweaNers";
  nc->autoConnect = false;
  nc->nickName = nickName_;
  sc = new ServerConfig;
  sc->host = "anthony.freenode.net";
  sc->port = 6667;
  sc->priority = 5;
  sc->ssl = false;
  sc->network = nc;
  nc->servers.append( sc );
  sc = new ServerConfig;
  sc->host = "wolfe.freenode.net";
  sc->port = 6667;
  sc->priority = 5;
  sc->ssl = false;
  sc->network = nc;
  nc->servers.append( sc );
  sc = new ServerConfig;
  sc->host = "2001:1418:13:1::25";
  sc->port = 6667;
  sc->priority = 10;
  sc->ssl = false;
  sc->network = nc;
  nc->servers.append( sc );
  networks_.append( nc );

  return networks_;
}
