/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#include <fstream>
#include <sstream>
#include <assert.h>
#include <libjson.h>
#include "config.h"
#include "utils.h"
#include <network.h>
#include <server.h>
#include "database.h"
#include "../contrib/libdazeus-irc/src/utils.h"

// #define DEBUG

class ConfigPrivate {
public:
	ConfigPrivate(JSONNode s) : s_(s) {}
	JSONNode &s() { return s_; }
private:
	JSONNode s_;
};

#define S settings_->s()

/**
 * @brief Constructor
 *
 * This constructor sets default configuration. A new Config object is usable,
 * but will not contain any networks or servers.
 */
Config::Config()
: oldNetworks_()
, networks_()
, sockets_()
, additionalSockets_()
, error_()
, settings_( 0 )
, databaseConfig_( 0 )
{
}

/**
 * @brief Destructor
 */
Config::~Config()
{
	reset();
}


void Config::addAdditionalSockets(const std::vector<std::string> &a)
{
	std::vector<std::string>::const_iterator sock_it;
	for(sock_it = a.begin(); sock_it != a.end(); ++sock_it) {
		additionalSockets_.push_back(*sock_it);
		sockets_.push_back(*sock_it);
	}
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

	JSONNode::const_iterator it = S.find_nocase(libjson::to_json_string(group));
	if(it == S.end())
		return configuration;

	for(JSONNode::const_iterator nit = it->begin(); nit != it->end(); ++nit) {
		configuration[strToLower(libjson::to_std_string(nit->name()))] = libjson::to_std_string(nit->as_string()).c_str();
	}

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

void Config::reset()
{
	networks_.clear();
	sockets_.clear();
	error_.clear();
	delete settings_;
	settings_ = 0;
	delete databaseConfig_;
	databaseConfig_ = 0;
}

/**
 * @brief Load configuration from file.
 *
 * This method attempts to load all contents from the given file to the Config
 * object, then returns whether that worked or not.
 */
bool Config::loadFromFile( std::string fileName )
{
	reset();
	std::ifstream ifile(fileName.c_str());
	if(!ifile) {
		error_ = "Configuration file does not exist or is unreadable: " + fileName;
		return false;
	}

	std::stringstream configstr;
	configstr << ifile.rdbuf();
	std::string configjson = configstr.str();

	JSONNode config;
	try {
		config = libjson::parse(libjson::to_json_string(configjson));
	} catch(std::invalid_argument e) {
		error_ = std::string("Invalid json in configuration file: ") + e.what();
		return false;
	}

	settings_ = new ConfigPrivate(config);

	JSONNode::const_iterator fit = S.begin(), it = S.begin();
	std::stringstream stream;
	std::string placeholder;
	int ival;
	bool ierror;
#define LOADCONFIG(stor, fld) fit = it->find_nocase(fld); if(fit != it->end()) stor = libjson::to_std_string(fit->as_string())

	/** Load general configuration */
	std::string defaultNickname = "DaZeus";
	std::string defaultUsername = "dazeus";
	std::string defaultFullname = "DaZeus";
	it = S.find_nocase("general");
	if(it != S.end()) {
		LOADCONFIG(defaultNickname, "nickname");
		LOADCONFIG(defaultUsername, "username");
		LOADCONFIG(defaultFullname, "fullname");
	}

	/** Load sockets */
	it = S.find_nocase("sockets");
	if(it != S.end()) {
		for(fit = it->begin(); fit != it->end(); ++fit) {
			sockets_.push_back(libjson::to_std_string(fit->as_string()));
		}
	}
	// Add additional sockets as defined
	std::vector<std::string>::const_iterator sock_it;
	for(sock_it = additionalSockets_.begin(); sock_it != additionalSockets_.end(); ++sock_it) {
		sockets_.push_back(*sock_it);
	}
	if(sockets_.size() == 0) {
		fprintf(stderr, "Warning: No sockets defined in configuration file. Plugins won't be able to connect.\n");
	}

	/** Load database configuration */
	it = S.find_nocase("database");
	databaseConfig_ = new DatabaseConfig;
	databaseConfig_->type = "mongo";
	databaseConfig_->hostname = "localhost";
	databaseConfig_->port = 27017;
	databaseConfig_->username = "";
	databaseConfig_->password = "";
	databaseConfig_->database = "dazeus";
	databaseConfig_->options = "";
	if(it != S.end()) {
		LOADCONFIG(databaseConfig_->hostname, "host");
		std::string dbRawPort;
		LOADCONFIG(dbRawPort,     "port");
		std::stringstream portStr;
		if(dbRawPort.length() > 0) {
			portStr << dbRawPort;
			portStr >> databaseConfig_->port;
		}
		LOADCONFIG(databaseConfig_->username, "username");
		LOADCONFIG(databaseConfig_->password, "password");
		LOADCONFIG(databaseConfig_->database, "database");

		if(!portStr) {
			fprintf(stderr, "Database port is not a valid numer: %s\n", dbRawPort.c_str());
			fprintf(stderr, "Assuming default port.\n");
			databaseConfig_->port = 27017;
		}
	}

	/** Load networks */
	std::map<std::string, NetworkConfig*> networks;
	JSONNode::const_iterator nit = S.find("networks");
	if(nit != S.end()) {
		for(it = nit->begin(); it != nit->end(); ++it) {
			NetworkConfig *nc = new NetworkConfig;
			nc->displayName = "Unnamed network";
			nc->nickName = defaultNickname;
			nc->userName = defaultUsername;
			nc->fullName = defaultFullname;
			nc->password = "";
			nc->autoConnect = false;

			LOADCONFIG(nc->displayName, "name");
			nc->name = strToIdentifier(nc->displayName);
			if(networks.find(nc->name) != networks.end()) {
				fprintf(stderr, "Error: Two networks exist with the same identifier, skipping.\n");
				return false;
			}
			LOADCONFIG(nc->nickName, "nickname");
			LOADCONFIG(nc->userName, "username");
			LOADCONFIG(nc->fullName, "fullname");
			LOADCONFIG(nc->password, "password");
			std::string autoconnect;
			LOADCONFIG(autoconnect, "autoconnect");
			nc->autoConnect = autoconnect == "true";
			JSONNode::const_iterator ssit = it->find_nocase("servers");
			if(ssit != S.end()) {
#define LOADCONFIG_S(stor, fld) fit = sit->find_nocase(fld); if(fit != sit->end()) stor = libjson::to_std_string(fit->as_string())
#define LOADINT_S(stor, fld) LOADCONFIG_S(placeholder, fld); \
	if(placeholder.length() == 0) ierror = false; \
	else { stream << placeholder; stream >> ival; ierror = !stream; if(!ierror) stor = ival; stream.str(""); stream.clear(); }
				for(JSONNode::const_iterator sit = ssit->begin(); sit != ssit->end(); ++sit) {
					ServerConfig *sc = new ServerConfig;
					sc->host = "";
					sc->port = 6667;
					sc->priority = 100;
					sc->network = nc;
					sc->ssl = false;

					LOADCONFIG_S(sc->host, "host");
					LOADINT_S(sc->port, "port");
					if(ierror) fprintf(stderr, "Invalid port string: %s\n", placeholder.c_str());
					LOADINT_S(sc->priority, "priority");
					if(ierror) fprintf(stderr, "Invalid priority string: %s\n", placeholder.c_str());
					std::string ssl;
					LOADCONFIG_S(ssl, "ssl");
					if(ssl == "true") sc->ssl = true;

					nc->servers.push_back(sc);
				}
#undef LOADINT_S
#undef LOADCONFIG_S
			}
			if(nc->servers.size() == 0) {
				fprintf(stderr, "Warning: No servers defined for network '%s' in configuration file.\n", nc->displayName.c_str());
			}
			networks[nc->name] = nc;
		}
	}
#undef LOADCONFIG

	if(networks.size() == 0) {
		fprintf(stderr, "Warning: No networks defined in configuration file.\n");
	}

	assert(networks_.size() == 0);
	std::map<std::string, NetworkConfig*>::const_iterator netit;
	for(netit = networks.begin(); netit != networks.end(); ++netit) {
		networks_.push_back(netit->second);
	}

	return true;
}

const std::vector<NetworkConfig*> &Config::networks()
{
	return networks_;
}

const std::vector<std::string> &Config::sockets() const
{
	return sockets_;
}
