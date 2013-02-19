/**
 * Copyright (c) Sjors Gielen, 2013
 * See LICENSE for license.
 */

#ifndef PLUGINMONITOR_H
#define PLUGINMONITOR_H

#include <signal.h>
#include <string>
#include <vector>

namespace dazeus {

struct PluginState;
struct PluginConfig;
struct NetworkConfig;

/**
 * @class PluginMonitor
 * @brief An init-style plugin runner and monitor.
 *
 * This class takes several PluginConfig instantiations and runs those that
 * can be run, and watches their state. If any of them quits, it re-runs them
 * init-style: if respawning fails a few times in a short period, it places
 * the plugin on incremental hold, meaning that it will continuously try to
 * restart the plugin with longer intervals.
 *
 * Plugins don't need to be run through PluginMonitor to connect to DaZeus:
 * connecting directly to the socket is always possible. The added value is
 * that PluginMonitor will auto-start the plugins and keep them running.
 */
class PluginMonitor
{
  public:
          PluginMonitor(std::string socket, std::string pluginDirectory,
              const std::vector<PluginConfig*> &plugins,
              const std::vector<NetworkConfig*> &networks);
         ~PluginMonitor();

    bool  shouldRun() { return should_run_; }
    void  runOnce();
    void  sigchild() { should_run_ = 1; }

  private:
    // explicitly disable copy constructor
    PluginMonitor(const PluginMonitor&);
    void operator=(const PluginMonitor&);

    bool start_plugin(PluginState *state);
    static pid_t fork_plugin(const std::string path, const std::vector<std::string> arguments, const std::string executable);
    static void stop_plugin(PluginState *state, bool hard);
    static void plugin_failed(PluginState *state, bool permanent = false);

    std::string pluginDirectory_;
    std::string socket_;
    std::vector<PluginState*> state_;
    volatile sig_atomic_t should_run_;
};

}

#endif
