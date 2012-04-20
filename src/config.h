/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <vector>
#include <string>
#include <map>
#include <stdint.h>

/**
 * These structs are only for configuration as it is in the configuration
 * file. This configuration cannot be changed via the runtime interface of
 * the IRC bot. Things like autojoining channels are not in here, because
 * that's handled by admins during runtime... :)
 */

class ConfigPrivate;
struct DatabaseConfig;
struct NetworkConfig;
struct ServerConfig;

class Config
{
  public:
        Config();
       ~Config();
  bool  loadFromFile( std::string fileName );
  void  reset();
  void  addAdditionalSockets(const std::vector<std::string>&);

  const std::vector<NetworkConfig*> &networks();
  const std::string               &lastError();
  const DatabaseConfig            *databaseConfig() const;
  const std::map<std::string,std::string> groupConfig(std::string plugin) const;
  const std::vector<std::string>         &sockets() const;

  private:
  // explicitly disable copy constructor
  Config(const Config&);
  void operator=(const Config&);

  std::vector<NetworkConfig*> oldNetworks_;
  std::vector<NetworkConfig*> networks_;
  std::vector<std::string>    sockets_;
  std::vector<std::string>    additionalSockets_;
  std::string                 error_;
  ConfigPrivate              *settings_;
  DatabaseConfig             *databaseConfig_;
};

#endif
