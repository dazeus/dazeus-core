/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#ifndef DAZEUS_H
#define DAZEUS_H

#include <vector>
#include <string>
#include <memory>

namespace dazeus {

class ConfigReader;
typedef std::shared_ptr<ConfigReader> ConfigReaderPtr;
class PluginComm;
class Network;
class Database;
class PluginMonitor;

class DaZeus
{
  public:
             DaZeus( std::string configFileName = std::string() );
            ~DaZeus();
    void     setConfigFileName( std::string fileName );
    std::string configFileName() const;
    bool     configLoaded() const;

    Database *database() const;
    const std::vector<Network*> &networks() const { return networks_; }

    void     run();
    bool     loadConfig();
    bool     initPlugins();
    void     autoConnect();
    bool     connectDatabase();
    void     stop();
    void     sigchild();

  private:
    // explicitly disable copy constructor
    DaZeus(const DaZeus&);
    void operator=(const DaZeus&);

    ConfigReaderPtr  config_;
    std::string      configFileName_;
    PluginComm      *plugins_;
    PluginMonitor   *plugin_monitor_;
    Database        *database_;
    std::vector<Network*>  networks_;
    bool             running_;
};

}

#endif
