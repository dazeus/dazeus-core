/**
 * Copyright (c) Sjors Gielen, 2013
 * See LICENSE for license.
 */

#include <iostream>
#include <dotconf.h>
#include <cassert>
#include "./config.h"
#include "utils.h"
#include "../contrib/libdazeus-irc/src/utils.h"
#include "server.h"
#include <limits>

enum section {
	S_ROOT,
	S_SOCKET,
	S_DATABASE,
	S_NETWORK,
	S_SERVER,
	S_PLUGIN
};

struct dazeus::ConfigReaderState {
	ConfigReaderState() : current_section(S_ROOT),
	global_progress(0), socket_progress(0), database_progress(0),
	network_progress(0), server_progress(0), plugin_progress(0) {}
	ConfigReaderState(ConfigReaderState const&);
	ConfigReaderState &operator=(ConfigReaderState const&);

	int current_section;
	std::vector<SocketConfig*> sockets;
	std::vector<NetworkConfig*> networks;
	std::vector<PluginConfig*> plugins;

	dazeus::GlobalConfig *global_progress;
	dazeus::SocketConfig *socket_progress;
	dazeus::DatabaseConfig *database_progress;
	dazeus::NetworkConfig *network_progress;
	dazeus::ServerConfig *server_progress;
	dazeus::PluginConfig *plugin_progress;
	std::string error;
};

dazeus::ConfigReader::ConfigReader(std::string file)
: global(0)
, database(0)
, file(file)
, state(new ConfigReaderState())
, is_read(false)
{}

dazeus::ConfigReader::~ConfigReader() {
	delete database;
	std::vector<NetworkConfig*>::const_iterator it;
	for(it = networks.begin(); it != networks.end(); ++it) {
		delete *it;
	}
	networks.clear();
	delete state;
}

static DOTCONF_CB(sect_open);
static DOTCONF_CB(sect_close);
static DOTCONF_CB(option);

static const configoption_t options[] = {
	{"<socket>", ARG_NONE, sect_open, NULL, CTX_ALL},
	{"</socket>", ARG_NONE, sect_close, NULL, CTX_ALL},
	{"<database>", ARG_NONE, sect_open, NULL, CTX_ALL},
	{"</database>", ARG_NONE, sect_close, NULL, CTX_ALL},
	{"<network", ARG_STR, sect_open, NULL, CTX_ALL},
	{"</network>", ARG_NONE, sect_close, NULL, CTX_ALL},
	{"<server>", ARG_NONE, sect_open, NULL, CTX_ALL},
	{"</server>", ARG_NONE, sect_close, NULL, CTX_ALL},
	{"<plugin", ARG_STR, sect_open, NULL, CTX_ALL},
	{"</plugin>", ARG_NONE, sect_close, NULL, CTX_ALL},
	{"nickname", ARG_RAW, option, NULL, CTX_ALL},
	{"username", ARG_RAW, option, NULL, CTX_ALL},
	{"fullname", ARG_RAW, option, NULL, CTX_ALL},
	{"plugindirectory", ARG_RAW, option, NULL, CTX_ALL},
	{"highlight", ARG_RAW, option, NULL, CTX_ALL},
	{"type", ARG_RAW, option, NULL, CTX_ALL},
	{"path", ARG_RAW, option, NULL, CTX_ALL},
	{"host", ARG_RAW, option, NULL, CTX_ALL},
	{"port", ARG_INT, option, NULL, CTX_ALL},
	{"password", ARG_RAW, option, NULL, CTX_ALL},
	{"database", ARG_RAW, option, NULL, CTX_ALL},
	{"options", ARG_RAW, option, NULL, CTX_ALL},
	{"autoconnect", ARG_RAW, option, NULL, CTX_ALL},
	{"priority", ARG_INT, option, NULL, CTX_ALL},
	{"ssl", ARG_RAW, option, NULL, CTX_ALL},
	{"executable", ARG_RAW, option, NULL, CTX_ALL},
	{"scope", ARG_RAW, option, NULL, CTX_ALL},
	{"parameters", ARG_RAW, option, NULL, CTX_ALL},
	{"var", ARG_RAW, option, NULL, CTX_ALL},
	LAST_OPTION
};

FUNC_ERRORHANDLER(error_handler);

static bool bool_is_true(std::string s) {
	s = strToLower(trim(s));
	return s == "true" || s == "yes" || s == "1";
}

void dazeus::ConfigReader::read() {
	if(is_read) return;

	configfile_t *configfile = dotconf_create(
		const_cast<char*>(file.c_str()), options,
		this, CASE_INSENSITIVE);
	if(!configfile) {
		throw exception(state->error = "Error opening config file.");
	}

	configfile->errorhandler = (dotconf_errorhandler_t) error_handler;

	// Initialise global config in progress. Other fields will be
	// initialised when a section is started, global starts now.
	state->global_progress = new GlobalConfig();
	if(dotconf_command_loop(configfile) == 0) {
		dotconf_cleanup(configfile);
		if(state->error.size() == 0)
			state->error = "Error reading config file.";
		throw exception(state->error);
	}

	if(state->database_progress == 0) {
		throw exception("No Database block defined in config file.");
	}

	assert(state->socket_progress == 0);
	assert(state->network_progress == 0);
	assert(state->server_progress == 0);
	assert(state->plugin_progress == 0);

	sockets = state->sockets;
	networks = state->networks;
	plugins = state->plugins;
	global = state->global_progress;
	database = state->database_progress;

	state->sockets.clear();
	state->networks.clear();
	state->plugins.clear();

	dotconf_cleanup(configfile);
	is_read = true;
}

FUNC_ERRORHANDLER(error_handler)
{
	dazeus::ConfigReaderState *s = ((dazeus::ConfigReader*)configfile->context)->_state();
	if(s->error.length() == 0) {
		s->error = msg;
	} else {
		s->error.append("\n" + std::string(msg));
	}
	return 1;
}

static DOTCONF_CB(sect_open)
{
	dazeus::ConfigReaderState *s = ((dazeus::ConfigReader*)cmd->context)->_state();
	std::string name(cmd->name);

	switch(s->current_section) {
	case S_ROOT:
		assert(s->global_progress != NULL);
		assert(s->socket_progress == NULL);
		assert(s->network_progress == NULL);
		assert(s->server_progress == NULL);
		if(name == "<socket>") {
			s->current_section = S_SOCKET;
			s->socket_progress = new dazeus::SocketConfig;
		} else if(name == "<database>") {
			if(s->database_progress != NULL) {
				return "More than one Database block defined in configuration file.";
			}
			s->current_section = S_DATABASE;
			s->database_progress = new dazeus::DatabaseConfig;
		} else if(name == "<network") {
			std::string networkname = cmd->data.str;
			networkname.resize(networkname.length() - 1);
			s->current_section = S_NETWORK;
			std::string default_nick = s->global_progress->default_nickname;
			std::string default_user = s->global_progress->default_username;
			std::string default_full = s->global_progress->default_fullname;
			s->network_progress = new dazeus::NetworkConfig(networkname, networkname,
				default_nick, default_user, default_full);
		} else if(name == "<plugin") {
			std::string pluginname = cmd->data.str;
			pluginname.resize(pluginname.length() - 1);
			s->current_section = S_PLUGIN;
			s->plugin_progress = new dazeus::PluginConfig(pluginname);
		} else {
			return "Logic error";
		}
		break;
	case S_NETWORK:
		assert(s->network_progress != NULL);
		assert(s->server_progress == NULL);
		if(name == "<server>") {
			s->current_section = S_SERVER;
			s->server_progress = new dazeus::ServerConfig("", s->network_progress);
		}
		break;
	default:
		return "Logic error";
	}
	return NULL;
}

static DOTCONF_CB(sect_close)
{
	dazeus::ConfigReaderState *s = ((dazeus::ConfigReader*)cmd->context)->_state();

	std::string name(cmd->name);
	switch(s->current_section) {
	// we can never have a section end in the root context
	case S_ROOT: return "Logic error";
	case S_SOCKET:
		if(name == "</socket>") {
			s->sockets.push_back(s->socket_progress);
			s->socket_progress = 0;
			s->current_section = S_ROOT;
		} else {
			return "Logic error";
		}
		break;
	case S_DATABASE:
		if(name == "</database>") {
			s->current_section = S_ROOT;
		} else {
			return "Logic error";
		}
		break;
	case S_NETWORK:
		if(name == "</network>") {
			s->networks.push_back(s->network_progress);
			s->network_progress = 0;
			s->current_section = S_ROOT;
		} else {
			return "Logic error";
		}
		break;
	case S_SERVER:
		if(name == "</server>") {
			s->network_progress->servers.push_back(s->server_progress);
			s->server_progress = 0;
			s->current_section = S_NETWORK;
		} else {
			return "Logic error";
		}
		break;
	case S_PLUGIN:
		if(name == "</plugin>") {
			s->plugins.push_back(s->plugin_progress);
			s->plugin_progress = 0;
			s->current_section = S_ROOT;
		} else {
			return "Logic error";
		}
		break;
	default:
		return "Logic error";
	}

	return NULL;
}
static DOTCONF_CB(option)
{
	dazeus::ConfigReaderState *s = ((dazeus::ConfigReader*)cmd->context)->_state();
	std::string name(cmd->name);
	switch(s->current_section) {
	case S_ROOT: {
		dazeus::GlobalConfig *g = s->global_progress;
		assert(g);
		if(name == "nickname") {
			g->default_nickname = trim(cmd->data.str);
		} else if(name == "username") {
			g->default_username = trim(cmd->data.str);
		} else if(name == "fullname") {
			g->default_fullname = trim(cmd->data.str);
		} else if(name == "plugindirectory") {
			g->plugindirectory = trim(cmd->data.str);
		} else if(name == "highlight") {
			g->highlight = trim(cmd->data.str);
		} else {
			s->error = "Invalid option name in root context: " + name;
			return "Configuration file contains errors";
		}
		break;
	}
	case S_SOCKET: {
		dazeus::SocketConfig *sc = s->socket_progress;
		assert(sc);
		if(name == "type") {
			sc->type = trim(cmd->data.str);
		} else if(name == "path") {
			sc->path = trim(cmd->data.str);
		} else if(name == "host") {
			sc->host = trim(cmd->data.str);
		} else if(name == "port") {
			if(cmd->data.value > std::numeric_limits<uint16_t>::max() || cmd->data.value < 0) {
				return "Invalid value for 'port'";
			}
			sc->port = cmd->data.value;
		} else {
			s->error = "Invalid option name in socket context: " + name;
			return "Configuration file contains errors";
		}
		break;
	}
	case S_DATABASE: {
		dazeus::DatabaseConfig *dc = s->database_progress;
		assert(dc);
		if(name == "type") {
			dc->type = trim(cmd->data.str);
		} else if(name == "host") {
			dc->hostname = trim(cmd->data.str);
		} else if(name == "port") {
			dc->port = cmd->data.value;
		} else if(name == "username") {
			dc->username = trim(cmd->data.str);
		} else if(name == "password") {
			dc->password = trim(cmd->data.str);
		} else if(name == "database") {
			dc->database = trim(cmd->data.str);
		} else if(name == "options") {
			dc->options = trim(cmd->data.str);
		} else {
			s->error = "Invalid option name in database context: " + name;
			return "Configuration file contains errors";
		}
		break;
	}
	case S_NETWORK: {
		dazeus::NetworkConfig *nc = s->network_progress;
		assert(nc);
		if(name == "autoconnect") {
			nc->autoConnect = bool_is_true(cmd->data.str);
		} else if(name == "nickname") {
			nc->nickName = trim(cmd->data.str);
		} else if(name == "username") {
			nc->userName = trim(cmd->data.str);
		} else if(name == "fullname") {
			nc->fullName = trim(cmd->data.str);
		} else if(name == "password") {
			nc->password = trim(cmd->data.str);
		} else {
			s->error = "Invalid option name in network context: " + name;
			return "Configuration file contains errors";
		}
		break;
	}
	case S_SERVER: {
		dazeus::ServerConfig *sc = s->server_progress;
		assert(sc);
		if(name == "host") {
			sc->host = trim(cmd->data.str);
		} else if(name == "port") {
			sc->port = cmd->data.value;
		} else if(name == "priority") {
			sc->priority = cmd->data.value;
		} else if(name == "ssl") {
			sc->ssl = bool_is_true(cmd->data.str);
		} else {
			s->error = "Invalid option name in server context: " + name;
			return "Configuration file contains errors";
		}
		break;
	}
	case S_PLUGIN: {
		dazeus::PluginConfig *pc = s->plugin_progress;
		assert(pc);
		if(name == "path") {
			pc->path = trim(cmd->data.str);
		} else if(name == "executable") {
			pc->executable = trim(cmd->data.str);
		} else if(name == "scope") {
			pc->scope = trim(cmd->data.str);
		} else if(name == "parameters") {
			pc->parameters = trim(cmd->data.str);
		} else if(name == "var") {
			if(cmd->arg_count != 2) {
				s->error = "Invalid amount of parameters to Var in plugin context";
				return "Configuration file contains errors";
			}
			pc->config[trim(cmd->data.list[0])] = trim(cmd->data.list[1]);
		} else {
			s->error = "Invalid option name in plugin context: " + name;
			return "Configuration file contains errors";
		}
		break;
	}
	default:
		return "Logic error";
	}
	return NULL;
}
