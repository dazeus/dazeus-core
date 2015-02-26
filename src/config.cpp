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

namespace dazeus {
struct ConfigReaderState {
	ConfigReaderState() : current_section(S_ROOT) {}

	int current_section;
	std::vector<SocketConfig> sockets;
	std::vector<NetworkConfig> networks;
	std::vector<PluginConfig> plugins;

	boost::optional<dazeus::GlobalConfig> global_progress;
	boost::optional<dazeus::SocketConfig> socket_progress;
	boost::optional<dazeus::DatabaseConfig> database_progress;
	boost::optional<dazeus::NetworkConfig> network_progress;
	boost::optional<dazeus::ServerConfig> server_progress;
	boost::optional<dazeus::PluginConfig> plugin_progress;
	std::string error;
};
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
	{"sslverify", ARG_RAW, option, NULL, CTX_ALL},
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

void dazeus::ConfigReader::read(std::string file) {
	if(is_read) return;

	std::shared_ptr<ConfigReaderState> state = std::make_shared<ConfigReaderState>();

	configfile_t *configfile = dotconf_create(
		const_cast<char*>(file.c_str()), options,
		state.get(), CASE_INSENSITIVE);
	if(!configfile) {
		throw exception(state->error = "Error opening config file.");
	}

	configfile->errorhandler = (dotconf_errorhandler_t) error_handler;

	// Initialise global config in progress. Other fields will be
	// initialised when a section is started, global starts now.
	state->global_progress.reset(GlobalConfig());
	if(dotconf_command_loop(configfile) == 0 || state->error.length() > 0) {
		dotconf_cleanup(configfile);
		if(state->error.size() == 0)
			state->error = "Error reading config file.";
		throw exception(state->error);
	}
	dotconf_cleanup(configfile);

	if(!state->database_progress) {
		throw exception("No Database block defined in config file.");
	}

	assert(!state->socket_progress);
	assert(!state->network_progress);
	assert(!state->server_progress);
	assert(!state->plugin_progress);

	sockets = state->sockets;
	networks = state->networks;
	plugins = state->plugins;
	global = state->global_progress;
	database = state->database_progress;

	is_read = true;
}

FUNC_ERRORHANDLER(error_handler)
{
	(void)type;
	(void)dc_errno;

	dazeus::ConfigReaderState *s = (dazeus::ConfigReaderState*)configfile->context;
	if(s->error.length() == 0) {
		s->error = msg;
	} else {
		s->error.append("\n" + std::string(msg));
	}
	return 1;
}

static DOTCONF_CB(sect_open)
{
	(void)ctx;

	dazeus::ConfigReaderState *s = (dazeus::ConfigReaderState*)cmd->context;
	std::string name(cmd->name);

	switch(s->current_section) {
	case S_ROOT:
		assert(s->global_progress);
		assert(!s->socket_progress);
		assert(!s->network_progress);
		assert(!s->server_progress);
		if(name == "<socket>") {
			s->current_section = S_SOCKET;
			s->socket_progress.reset(dazeus::SocketConfig());
		} else if(name == "<database>") {
			if(s->database_progress) {
				return "More than one Database block defined in configuration file.";
			}
			s->current_section = S_DATABASE;
			s->database_progress.reset(dazeus::DatabaseConfig());
		} else if(name == "<network") {
			std::string networkname = cmd->data.str;
			networkname.resize(networkname.length() - 1);
			s->current_section = S_NETWORK;
			s->network_progress.reset(dazeus::NetworkConfig());
			s->network_progress->name = networkname;
			s->network_progress->displayName = networkname;
			s->network_progress->nickName = s->global_progress->default_nickname;
			s->network_progress->userName = s->global_progress->default_username;
			s->network_progress->fullName = s->global_progress->default_fullname;
		} else if(name == "<plugin") {
			std::string pluginname = cmd->data.str;
			pluginname.resize(pluginname.length() - 1);
			if(pluginname.length() == 0) {
				return "All plugins must have a name in their <Plugin> tag.";
			}
			s->current_section = S_PLUGIN;
			s->plugin_progress.reset(dazeus::PluginConfig(pluginname));
		} else {
			return "Logic error";
		}
		break;
	case S_NETWORK:
		assert(s->network_progress);
		assert(!s->server_progress);
		if(name == "<server>") {
			s->current_section = S_SERVER;
			s->server_progress.reset(dazeus::ServerConfig());
			s->server_progress->host = "";
		}
		break;
	default:
		return "Logic error";
	}
	return NULL;
}

static DOTCONF_CB(sect_close)
{
	(void)ctx;

	dazeus::ConfigReaderState *s = (dazeus::ConfigReaderState*)cmd->context;

	std::string name(cmd->name);
	switch(s->current_section) {
	// we can never have a section end in the root context
	case S_ROOT: return "Logic error";
	case S_SOCKET:
		if(name == "</socket>") {
			s->sockets.push_back(*s->socket_progress);
			s->socket_progress.reset();
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
			s->networks.push_back(*s->network_progress);
			s->network_progress.reset();
			s->current_section = S_ROOT;
		} else {
			return "Logic error";
		}
		break;
	case S_SERVER:
		if(name == "</server>") {
			s->network_progress->servers.push_back(*s->server_progress);
			s->server_progress.reset();
			s->current_section = S_NETWORK;
		} else {
			return "Logic error";
		}
		break;
	case S_PLUGIN:
		if(name == "</plugin>") {
			s->plugins.push_back(*s->plugin_progress);
			s->plugin_progress.reset();
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
	(void)ctx;

	dazeus::ConfigReaderState *s = (dazeus::ConfigReaderState*)cmd->context;
	std::string name(cmd->name);
	switch(s->current_section) {
	case S_ROOT: {
		dazeus::GlobalConfig &g = *s->global_progress;
		if(name == "nickname") {
			g.default_nickname = trim(cmd->data.str);
		} else if(name == "username") {
			g.default_username = trim(cmd->data.str);
		} else if(name == "fullname") {
			g.default_fullname = trim(cmd->data.str);
		} else if(name == "plugindirectory") {
			g.plugindirectory = trim(cmd->data.str);
		} else if(name == "highlight") {
			g.highlight = trim(cmd->data.str);
		} else {
			s->error = "Invalid option name in root context: " + name;
			return "Configuration file contains errors";
		}
		break;
	}
	case S_SOCKET: {
		dazeus::SocketConfig &sc = *s->socket_progress;
		if(name == "type") {
			sc.type = trim(cmd->data.str);
		} else if(name == "path") {
			sc.path = trim(cmd->data.str);
		} else if(name == "host") {
			sc.host = trim(cmd->data.str);
		} else if(name == "port") {
			if(cmd->data.value > std::numeric_limits<uint16_t>::max() || cmd->data.value < 0) {
				return "Invalid value for 'port'";
			}
			sc.port = cmd->data.value;
		} else {
			s->error = "Invalid option name in socket context: " + name;
			return "Configuration file contains errors";
		}
		break;
	}
	case S_DATABASE: {
		dazeus::DatabaseConfig &dc = *s->database_progress;
		if(name == "type") {
			dc.type = trim(cmd->data.str);
		} else if(name == "host") {
			dc.hostname = trim(cmd->data.str);
		} else if(name == "port") {
			dc.port = cmd->data.value;
		} else if(name == "username") {
			dc.username = trim(cmd->data.str);
		} else if(name == "password") {
			dc.password = trim(cmd->data.str);
		} else if(name == "database") {
			dc.database = trim(cmd->data.str);
		} else if(name == "options") {
			dc.options = trim(cmd->data.str);
		} else {
			s->error = "Invalid option name in database context: " + name;
			return "Configuration file contains errors";
		}
		break;
	}
	case S_NETWORK: {
		boost::optional<dazeus::NetworkConfig> &nc = s->network_progress;
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
		boost::optional<dazeus::ServerConfig> &sc = s->server_progress;
		assert(sc);
		if(name == "host") {
			sc->host = trim(cmd->data.str);
		} else if(name == "port") {
			sc->port = cmd->data.value;
		} else if(name == "priority") {
			sc->priority = cmd->data.value;
		} else if(name == "ssl") {
			sc->ssl = bool_is_true(cmd->data.str);
		} else if(name == "sslverify") {
			sc->ssl_verify = bool_is_true(cmd->data.str);
		} else {
			s->error = "Invalid option name in server context: " + name;
			return "Configuration file contains errors";
		}
		break;
	}
	case S_PLUGIN: {
		dazeus::PluginConfig &pc = *s->plugin_progress;
		if(name == "path") {
			pc.path = trim(cmd->data.str);
		} else if(name == "executable") {
			pc.executable = trim(cmd->data.str);
		} else if(name == "scope") {
			std::string scope = strToLower(trim(cmd->data.str));
			if(scope == "network") {
				pc.per_network = true;
			} else if(scope != "global") {
				s->error = "Invalid value for Scope for plugin " + pc.name;
				return "Configuration file contains errors";
			}
		} else if(name == "parameters") {
			pc.parameters = trim(cmd->data.str);
		} else if(name == "var") {
			if(cmd->arg_count != 2) {
				s->error = "Invalid amount of parameters to Var in plugin context";
				return "Configuration file contains errors";
			}
			pc.config[trim(cmd->data.list[0])] = trim(cmd->data.list[1]);
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
