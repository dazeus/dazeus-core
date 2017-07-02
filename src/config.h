/**
 * Copyright (c) Sjors Gielen, 2013
 * See LICENSE for license.
 */

#ifndef _DAZEUS_CONFIG_H
#define _DAZEUS_CONFIG_H

#include "network.h"
#include "db/database.h"
#include "mstd/optional.hpp"
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <sstream>
#include <assert.h>

namespace dazeus {

struct GlobalConfig {
	GlobalConfig()
	: default_nickname("DaZeus")
	, default_username("DaZeus")
	, default_fullname("DaZeus IRC bot")
	, plugindirectory("plugins")
	, highlight("}") {}

	std::string default_nickname;
	std::string default_username;
	std::string default_fullname;
	std::string plugindirectory;
	std::string highlight;
};

struct PluginConfig {
	PluginConfig(std::string n) : name(n), per_network(false) {}
	std::string name;
	std::string path;
	std::string executable;
	bool per_network;
	std::string parameters;
	std::map<std::string, std::string> config;
};

struct SocketConfig {
	SocketConfig() : port(0) {}
	std::string toString() const {
		if(type == "unix") {
			return "unix:" + path;
		} else if(type == "tcp") {
			std::stringstream ss;
			ss << "tcp:" << host << ":" << port;
			return ss.str();
		}
		assert(!"Unknown type in SocketConfig::toString()");
		return "";
	}
	std::string type;
	std::string host;
	uint16_t port;
	std::string path;
};

class ConfigReader;
typedef std::shared_ptr<ConfigReader> ConfigReaderPtr;

class ConfigReader {
	std::vector<NetworkConfig> networks;
	std::vector<PluginConfig> plugins;
	std::vector<SocketConfig> sockets;
	mstd::optional<GlobalConfig> global;
	mstd::optional<db::DatabaseConfig> database;
	bool is_read;

public:
	struct exception : public std::runtime_error {
		exception(std::string e) : std::runtime_error(e) {}
	};

	ConfigReader() : is_read(false) {}

	bool isRead() const { return is_read; }
	// Re-reads all configuration. Throws ConfigReader::exception if something failed.
	void read(std::string file);

	const std::vector<NetworkConfig> &getNetworks() const { return networks; }
	const std::vector<PluginConfig> &getPlugins() const { return plugins; }
	// socket configs may be changed when creating one (but this should change)
	std::vector<SocketConfig> &getSockets() { return sockets; }
	const SocketConfig &getPluginSocket() const {
		if(sockets.size() == 0) {
			throw std::runtime_error("No sockets defined, cannot run plugins");
		}
		return sockets.at(0);
	}
	const GlobalConfig &getGlobalConfig() const { return *global; }
	const db::DatabaseConfig &getDatabaseConfig() const { return *database; }
};

}

#endif
