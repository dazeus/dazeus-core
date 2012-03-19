/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <vector>
#include <string>
#include <map>
#include <list>

/**
 * These structs are only for configuration as it is in the configuration
 * file. This configuration cannot be changed via the runtime interface of
 * the IRC bot. Things like autojoining channels are not in here, because
 * that's handled by admins during runtime... :)
 */
struct ServerConfig;

struct NetworkConfig {
  std::string name;
  std::string displayName;
  std::string nickName;
  std::string userName;
  std::string fullName;
  std::string password;
  std::vector<ServerConfig*> servers;
  bool autoConnect;
};

struct ServerConfig {
  std::string host;
  uint16_t port;
  uint8_t priority;
  NetworkConfig *network;
  bool ssl;
};

struct DatabaseConfig {
  std::string type;
  std::string hostname;
  uint16_t port;
  std::string username;
  std::string password;
  std::string database;
  std::string options;
};

class QSettings;

class Config
{
  public:
        Config();
       ~Config();
  bool  loadFromFile( std::string fileName );

  const std::list<NetworkConfig*> &networks();
  const std::string               &lastError();
  const DatabaseConfig            *databaseConfig() const;
  const std::map<std::string,std::string> groupConfig(std::string plugin) const;

  private:
  std::list<NetworkConfig*> oldNetworks_;
  std::list<NetworkConfig*> networks_;
  std::string               error_;
  QSettings            *settings_;
  DatabaseConfig       *databaseConfig_;
};

#endif
