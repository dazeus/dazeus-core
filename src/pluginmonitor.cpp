/**
 * Copyright (c) Sjors Gielen, 2013
 * See LICENSE for license.
 */

#include "pluginmonitor.h"
#include "utils.h"
#include "config.h"
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <assert.h>
#include <iostream>
#include <errno.h>
#include <string.h>

#define PLUGIN_EXIT_VALUE_CHDIR -7
#define PLUGIN_EXIT_VALUE_EXEC -8

// after an hour, consider a failure as a stand-alone fault, don't link it
// to earlier failures anymore
#define PLUGIN_RUNTIME_RESET_FAILURE 3600

// maximum time to wait between plugin restarts
#define PLUGIN_RUNTIME_MAX_WAIT_TIME 300


namespace dazeus {
	struct PluginState {
		PluginState(const PluginConfig &c, std::string network)
		: config(c), will_autostart(true), pid(0), network(network)
		, num_failures(0), last_start(0) {
			assert(config.per_network || network.length() == 0);
			assert(!config.per_network || network.length() > 0);
		}
		~PluginState() {}

		PluginConfig config;
		bool will_autostart;
		pid_t pid;
		std::string network;
		int num_failures;
		time_t last_start;
	};
}

bool directory_exists(std::string dir) {
	struct stat buf;
	int res = stat(dir.c_str(), &buf);
	if(res == -1) return false;
	return (buf.st_mode & S_IFDIR) == S_IFDIR;
}

bool file_exists(std::string file) {
	struct stat buf;
	int res = stat(file.c_str(), &buf);
	if(res == -1) return false;
	return (buf.st_mode & S_IFREG) == S_IFREG;
}

bool executable_exists(std::string file) {
	return access(file.c_str(), X_OK) == 0;
}

dazeus::PluginMonitor::PluginMonitor(ConfigReaderPtr config)
: pluginDirectory_(config->getGlobalConfig().plugindirectory)
, config_(config)
, should_run_(1)
{
}

void dazeus::PluginMonitor::configReloaded() {
	std::vector<std::string> plugins_seen;

	const std::vector<PluginConfig> &plugins = config_->getPlugins();
	std::vector<PluginConfig>::const_iterator it;
	for(it = plugins.begin(); it != plugins.end(); ++it) {
		const PluginConfig &config = *it;
		assert(config.name.length() > 0);

		if(config.per_network) {
			const std::vector<NetworkConfig> &networks = config_->getNetworks();
			std::vector<NetworkConfig>::const_iterator nit;
			for(nit = networks.begin(); nit != networks.end(); ++nit) {
				std::string state_name = config.name + "!" + nit->name;

				if(contains(plugins_seen, state_name)) {
					throw std::runtime_error("Multiple plugins exist with name " + state_name);
				}
				plugins_seen.push_back(state_name);

				auto sit = state_.find(state_name);
				if(sit == state_.end()) {
					PluginState *s = new PluginState(config, nit->name);
					state_[state_name] = s;
				} else {
					state_[state_name]->num_failures = 0;
					state_[state_name]->will_autostart = true;
					state_[state_name]->config = config;
				}
			}
		} else {
			if(contains(plugins_seen, config.name)) {
				throw std::runtime_error("Multiple plugins exist with name " + config.name);
			}
			plugins_seen.push_back(config.name);

			auto sit = state_.find(config.name);
			if(sit == state_.end()) {
				std::cout << "Starting " << config.name << std::endl;
				PluginState *s = new PluginState(config, "");
				state_[config.name] = s;
			} else {
				state_[config.name]->num_failures = 0;
				state_[config.name]->will_autostart = true;
				state_[config.name]->config = config;
			}
		}
	}

	// kill all plugins that we didn't have in this run
	std::map<std::string, PluginState*> plugins_to_kill;
	for(auto it = state_.begin(); it != state_.end(); ++it) {
		bool plugin_is_active = it->second->pid > 0 || it->second->will_autostart;
		if(!contains(plugins_seen, it->first) && plugin_is_active) {
			std::cerr << "Marking to kill " << it->first << std::endl;
			std::cerr << "Current PID = " << it->second->pid << " / Will autostart = " << it->second->will_autostart << std::endl;
			plugins_to_kill[it->first] = it->second;
			it->second->will_autostart = false;
		}
	}

	kill_plugins(plugins_to_kill);
	should_run_ = 1;
	runOnce();
}

dazeus::PluginMonitor::~PluginMonitor() {
	kill_plugins(state_);
	for(auto it = state_.begin(); it != state_.end(); ++it) {
		delete it->second;
	}
}

void dazeus::PluginMonitor::kill_plugins(std::map<std::string, PluginState*> &plugins) {
	// Try a graceful stop of all plugins...
	bool pluginsStopped = false;
	for(auto it = plugins.begin(); it != plugins.end(); ++it) {
		if(it->second->pid != 0) {
			stop_plugin(it->second, false);
			pluginsStopped = true;
		}
	}

	if(pluginsStopped) {
		sleep(2);
		runOnce();
	}

	pluginsStopped = false;
	for(auto it = plugins.begin(); it != plugins.end(); ++it) {
		if(it->second->pid != 0) {
			stop_plugin(it->second, true);
			pluginsStopped = true;
		}
	}

	if(pluginsStopped) {
		sleep(2);
		runOnce();
	}

	for(auto it = plugins.begin(); it != plugins.end(); ++it) {
		if(it->second->pid != 0) {
			std::cerr << "In PluginMonitor::kill_plugins(), failed to stop plugin " << it->second->config.name << std::endl;
		}
	}
}

void dazeus::PluginMonitor::stop_plugin(PluginState *state, bool hard) {
	assert(state != NULL);
	assert(state->pid != 0);
	const PluginConfig &config = state->config;

	int signal = hard ? SIGKILL : SIGINT;

	if(kill(state->pid, signal) < 0) {
		std::cerr << "Failed to kill plugin " << config.name << ": " << strerror(errno) << std::endl;
	}
	// If succesfully killed, runOnce() will set the pid to 0
}

void dazeus::PluginMonitor::plugin_failed(PluginState *state, bool permanent) {
	if(permanent) {
		state->will_autostart = false;
	} else {
		state->num_failures++;
	}
}

bool dazeus::PluginMonitor::start_plugin(PluginState *state) {
	assert(state != NULL);
	assert(state->pid == 0);
	const PluginConfig &config = state->config;

	state->last_start = time(NULL);

	// Configuration loading
	std::string path = config.path;
	if(path.length() == 0) {
		// If not set, defaults to the name of the plugin
		path = config.name;
	}
	if(path[0] != '/') {
		// If relative, counts from the Plugins directory as set in
		// dazeus.conf.
		path = pluginDirectory_ + "/" + path;
	}

	std::string executable = config.executable;
	if(executable.length() == 0) {
		// If not set, defaults to the name of the plugin.
		executable = config.name;
	}

	std::vector<std::string> arguments;
	std::string current_arg;
	bool in_quotes = false;
	bool in_doublequotes = false;
	bool in_escape = false;
	for(unsigned i = 0; i < config.parameters.length(); ++i) {
		char c = config.parameters[i];
		if(in_escape) {
			current_arg += c;
			in_escape = false;
		}
		else if(in_quotes || in_doublequotes) {
			assert(!(in_quotes && in_doublequotes));
			if((in_quotes && c == '\'') || (in_doublequotes && c == '"')) {
				in_quotes = in_doublequotes = false;
				arguments.push_back(current_arg);
				current_arg.clear();
			} else {
				current_arg += c;
			}
		}
		else if(c == '\'') {
			in_quotes = true;
		}
		else if(c == '"') {
			in_doublequotes = true;
		}
		else if(isspace(c)) {
			if(current_arg.length() > 0) {
				arguments.push_back(current_arg);
				current_arg.clear();
			}
		}
		else if(c == '%') {
			if(i + 1 == config.parameters.length()) {
				current_arg += '%';
			} else {
				char d = config.parameters[i+1];
				switch(d) {
				case 's': current_arg += config_->getPluginSocket().toString(); ++i; break;
				case 'n': current_arg += state->network; ++i; break;
				default:  current_arg += '%'; break;
				}
			}
		}
		else {
			current_arg += c;
		}
	}
	if(current_arg.length() > 0) {
		arguments.push_back(current_arg);
	}

	// Sanity checking (to decrease the chance of errors in the child)
	if(!directory_exists(path)) {
		std::cerr << "Failed to run plugin " << config.name << ": its directory does not exist" << std::endl;
		plugin_failed(state);
		return false;
	}

	std::string full_executable = executable;
	if(full_executable[0] != '/') {
		// relative path
		full_executable = path + '/' + executable;
	}
	if(!file_exists(full_executable)) {
		std::cerr << "Failed to run plugin " << config.name << ": its executable does not exist" << std::endl;
		plugin_failed(state);
		return false;
	} else if(!executable_exists(full_executable)) {
		std::cerr << "Failed to run plugin " << config.name << ": its executable is not executable" << std::endl;
		plugin_failed(state);
		return false;
	}

	pid_t res = fork_plugin(path, arguments, executable);
	if(res == -1) {
		std::cerr << "Failed to run plugin " << config.name << ": " << strerror(errno) << std::endl;
		plugin_failed(state);
		return false;
	}

	// we are the parent, plugin is running
	state->pid = res;
	std::cout << "Plugin " << config.name << " started, PID " << state->pid << std::endl;
	return true;
}

pid_t dazeus::PluginMonitor::fork_plugin(const std::string path, const std::vector<std::string> arguments, const std::string executable) {
	pid_t res = fork();
	if(res != 0) {
		return res;
	}

	// we are the child; chdir() and execve()
	// path executable per_network parameters
	if(chdir(path.c_str()) < 0) {
		exit(PLUGIN_EXIT_VALUE_CHDIR);
	}

	// prepare *char[] arguments
	char **child_argv = (char**)malloc((2 + arguments.size()) * sizeof(char*));
	child_argv[0] = strdup(executable.c_str());
	for(unsigned i = 0; i < arguments.size(); ++i) {
		child_argv[i + 1] = strdup(arguments[i].c_str());
	}
	child_argv[arguments.size() + 1] = NULL;

	// execv() takes a path; it never queries PATH, even if the first parameter
	// does not contain any slashes.
	execv(executable.c_str(), child_argv);
	exit(PLUGIN_EXIT_VALUE_EXEC);
}

void dazeus::PluginMonitor::runOnce() {
	if(!should_run_) {
		return;
	}

	// First, block CHLD signals while we process childs
	sigset_t signalblock;
	sigemptyset(&signalblock);
	sigaddset(&signalblock, SIGCHLD);
	sigprocmask(SIG_BLOCK, &signalblock, NULL);

	// Process died childs
	pid_t child;
	int child_status;
	errno = 0;
	while((child = wait4(-1, &child_status, WNOHANG, NULL))) {
		if(child == -1 && errno == ECHILD) {
			// no child processes
			break;
		} else if(child == -1) {
			std::cerr << "wait4() returned an error: " << strerror(errno) << std::endl;
			break;
		}

		PluginState *state = NULL;
		for(auto it = state_.begin(); it != state_.end(); ++it) {
			if(it->second->pid == child) {
				state = it->second;
				break;
			}
		}
		if(state == NULL) {
			std::cerr << "wait4() returned a PID that is not ours: " << child << std::endl;
			continue;
		}

		if(WIFEXITED(child_status)) {
			std::cerr << "Plugin " << state->config.name << " exited with code " << WEXITSTATUS(child_status) << std::endl;
		} else if(WIFSIGNALED(child_status)) {
			std::string coredumped = WCOREDUMP(child_status) ? " (core dumped)" : "";
			std::cerr << "Plugin " << state->config.name << " killed by signal " << WTERMSIG(child_status) << coredumped << std::endl;
		} else {
			std::cerr << "Plugin signaled but did not quit? Ignoring..." << std::endl;
			continue;
		}

		state->pid = 0;
		if(state->last_start + PLUGIN_RUNTIME_RESET_FAILURE < time(NULL)) {
			// more than PLUGIN_RUNTIME_RESET_FAILURE seconds have passed
			// since the last start attempt; assume the last start was succesful
			// up to now and reset num_failures
			state->num_failures = 0;
		}
		state->num_failures++;
	}

	// Process plugins that should start now
	bool waiting_plugin = false;
	for(auto it = state_.begin(); it != state_.end(); ++it) {
		PluginState  *state  = it->second;
		assert(state != NULL);

		if(!state->will_autostart) {
			// never start it anyway
			continue;
		}

		if(state->pid != 0) {
			// it's running fine
			continue;
		}

		int wait_seconds = 0;
		if(state->num_failures > 0) {
			wait_seconds = 5 * (1 << (state->num_failures - 1));
		}
		if(wait_seconds > PLUGIN_RUNTIME_MAX_WAIT_TIME) {
			wait_seconds = PLUGIN_RUNTIME_MAX_WAIT_TIME;
		}

		if(state->last_start + wait_seconds > time(NULL)) {
			// The new starting point hasn't passed yet, don't
			// re-start the plugin yet
			waiting_plugin = true;
			continue;
		}

		// See if we can auto-run it
		if(!start_plugin(state)) {
			waiting_plugin = true;
		}
	}

	// Only run again if there is a plugin waiting to be started again
	should_run_ = waiting_plugin;

	// Allow CHLD signals to interrupt us again
	sigprocmask(SIG_UNBLOCK, &signalblock, NULL);
}
