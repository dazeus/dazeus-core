/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#ifndef DAZEUS_H
#define DAZEUS_H

#include <vector>
#include <string>
#include <memory>
#include <map>

namespace dazeus {
namespace db {
  class Database;
}

class ConfigReader;
typedef std::shared_ptr<ConfigReader> ConfigReaderPtr;
class PluginComm;
class Network;
class PluginMonitor;

class DaZeus
{
  public:
             DaZeus( std::string configFileName = std::string() );
            ~DaZeus();
    void     setConfigFileName( std::string fileName );
    std::string configFileName() const;
    bool     configLoaded() const;

    db::Database *database() const;
    const std::map<std::string, Network*> &networks() const { return networks_; }

    void     run();
    void     reloadConfig() { config_reload_pending_ = true; }
    void     stop();
    void     sigchild();

  private:
    // explicitly disable copy constructor
    DaZeus(const DaZeus&);
    void operator=(const DaZeus&);

    bool     loadConfig();
    bool     connectDatabase();

    ConfigReaderPtr  config_;
    std::string      configFileName_;
    PluginComm      *plugins_;
    PluginMonitor   *plugin_monitor_;
    db::Database    *database_;
    std::map<std::string, Network*>  networks_;
    bool             running_;
    bool config_reload_pending_;
};

}

#endif
