/**
 * Copyright (c) Sjors Gielen, 2011
 * See LICENSE for license.
 */

#define __STDC_LIMIT_MACROS
#include <stdint.h>
#include <jansson.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/uio.h>
#include <sys/select.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>
#include <poll.h>
#include <limits.h>

#include <string>
#include <sstream>

#include "plugincomm.h"
#include "network.h"
#include "config.h"
#include "server.h"
#include "config.h"
#include "dazeus.h"
#include "database.h"
#include "utils.h"

static std::string realpath(std::string path) {
	char newpath[PATH_MAX];
	realpath(path.c_str(), newpath);
	return std::string(newpath);
}

#define NOTBLOCKING(x) fcntl(x, F_SETFL, fcntl(x, F_GETFL) | O_NONBLOCK)

dazeus::PluginComm::PluginComm(Database *d, ConfigReader *c, DaZeus *bot)
: NetworkListener()
, tcpServers_()
, localServers_()
, commandQueue_()
, sockets_()
, database_(d)
, config_(c)
, dazeus_(bot)
{
}

dazeus::PluginComm::~PluginComm() {
	std::map<int,SocketInfo>::iterator it;
	for(it = sockets_.begin(); it != sockets_.end(); ++it) {
		close(it->first);
	}
	std::vector<int>::iterator it2;
	for(it2 = tcpServers_.begin(); it2 != tcpServers_.end(); ++it2) {
		close(*it2);
	}
	for(it2 = localServers_.begin(); it2 != localServers_.end(); ++it2) {
		close(*it2);
	}
}

void dazeus::PluginComm::run(int timeout_sec) {
	fd_set sockets, out_sockets;
	int highest = 0;
	struct timeval timeout;
	FD_ZERO(&sockets);
	FD_ZERO(&out_sockets);
	std::vector<int>::iterator it;
	for(it = tcpServers_.begin(); it != tcpServers_.end(); ++it) {
		if(*it > highest)
			highest = *it;
		FD_SET(*it, &sockets);
	}
	for(it = localServers_.begin(); it != localServers_.end(); ++it) {
		if(*it > highest)
			highest = *it;
		FD_SET(*it, &sockets);
	}
	std::map<int,SocketInfo>::iterator it2;
	for(it2 = sockets_.begin(); it2 != sockets_.end(); ++it2) {
		if(it2->first > highest)
			highest = it2->first;
		FD_SET(it2->first, &sockets);
	}
	// and add the IRC descriptors
	std::vector<Network*>::const_iterator nit;
	for(nit = dazeus_->networks().begin(); nit != dazeus_->networks().end(); ++nit) {
		if((*nit)->activeServer()) {
			int ircmaxfd = 0;
			(*nit)->addDescriptors(&sockets, &out_sockets, &ircmaxfd);
			if(ircmaxfd > highest)
				highest = ircmaxfd;
		}
	}
	timeout.tv_sec = timeout_sec;
	timeout.tv_usec = 0;
	int socks = select(highest + 1, &sockets, &out_sockets, NULL, &timeout);
	if(socks < 0) {
		if(errno != EINTR) {
			fprintf(stderr, "select() failed: %s\n", strerror(errno));
		}

		if(errno == EBADF) {
			// hopefully it was a regular plugin socket that went bad
			// then poll() will notice it
			poll();
		}
		return;
	}
	else if(socks == 0) {
		// No sockets fired, just check for network timeouts
		for(nit = dazeus_->networks().begin(); nit != dazeus_->networks().end(); ++nit) {
			if((*nit)->activeServer()) {
				(*nit)->checkTimeouts();
			}
		}
		return;
	}
	for(it = tcpServers_.begin(); it != tcpServers_.end(); ++it) {
		if(FD_ISSET(*it, &sockets)) {
			newTcpConnection();
			break;
		}
	}
	for(it = localServers_.begin(); it != localServers_.end(); ++it) {
		if(FD_ISSET(*it, &sockets)) {
			newLocalConnection();
			break;
		}
	}
	for(it2 = sockets_.begin(); it2 != sockets_.end(); ++it2) {
		if(FD_ISSET(it2->first, &sockets)) {
			poll();
			break;
		}
	}
	for(nit = dazeus_->networks().begin(); nit != dazeus_->networks().end(); ++nit) {
		if((*nit)->activeServer()) {
			(*nit)->processDescriptors(&sockets, &out_sockets);
			(*nit)->checkTimeouts();
		}
	}
}

void dazeus::PluginComm::init() {
	std::vector<SocketConfig*>::const_iterator it;

	for(it = config_->getSockets().begin(); it != config_->getSockets().end(); ++it) {
		SocketConfig *sc = *it;
		if(sc->type == "unix") {
			unlink(sc->path.c_str());
			int server = ::socket(AF_UNIX, SOCK_STREAM, 0);
			if(server <= 0) {
				fprintf(stderr, "(PluginComm) Failed to create socket at %s: %s\n", sc->path.c_str(), strerror(errno));
				continue;
			}
			NOTBLOCKING(server);
			struct sockaddr_un addr;
			addr.sun_family = AF_UNIX;
			strcpy(addr.sun_path, sc->path.c_str());
			if(bind(server, (struct sockaddr*) &addr, sizeof(struct sockaddr_un)) < 0) {
				close(server);
				fprintf(stderr, "(PluginComm) Failed to start listening for local connections on %s: %s\n", sc->path.c_str(), strerror(errno));
				continue;
			}
			if(listen(server, 5) < 0) {
				close(server);
				unlink(sc->path.c_str());
			}
			// Plugins run in a different working directory, so
			// make sure the socket path is an absolute path
			sc->path = realpath(sc->path);
			localServers_.push_back(server);
		} else if(sc->type == "tcp") {
			std::string portStr;
			{
				std::stringstream portStrSs;
				portStrSs << sc->port;
				portStr = portStrSs.str();
			}

			struct addrinfo *hints, *result;
			hints = (struct addrinfo*)calloc(1, sizeof(struct addrinfo));
			hints->ai_flags = 0;
			hints->ai_family = AF_INET;
			hints->ai_socktype = SOCK_STREAM;
			hints->ai_protocol = 0;

			int s = getaddrinfo(sc->host.c_str(), portStr.c_str(), hints, &result);
			free(hints);
			hints = 0;

			if(s != 0) {
				fprintf(stderr, "(PluginComm) Failed to resolve TCP address: %s\n", (char*)gai_strerror(s));
				continue;
			}

			int server = ::socket(AF_INET, SOCK_STREAM, 0);
			if(server <= 0) {
				fprintf(stderr, "(PluginComm) Failed to create socket: %s\n", strerror(errno));
				continue;
			}
			NOTBLOCKING(server);
			if(bind(server, result->ai_addr, result->ai_addrlen) < 0) {
				close(server);
				fprintf(stderr, "(PluginComm) Failed to start listening for TCP connections on %s:%d: %s\n",
					sc->host.c_str(), sc->port, strerror(errno));
			}
			if(listen(server, 5) < 0) {
				close(server);
			}

			// Now, if the IP is inaddr_any, change it to localhost
			if(result->ai_family == AF_INET) {
				sockaddr_in *addr = (sockaddr_in*)result->ai_addr;
				if(addr->sin_addr.s_addr == INADDR_ANY) {
					sc->host = "127.0.0.1";
				}
			}

			tcpServers_.push_back(server);
		} else {
			fprintf(stderr, "(PluginComm) Skipping socket: unknown type >%s<\n", sc->type.c_str());
		}
	}
}

void dazeus::PluginComm::newTcpConnection() {
	std::vector<int>::iterator it;
	for(it = tcpServers_.begin(); it != tcpServers_.end(); ++it) {
		while(1) {
			int sock = accept(*it, NULL, NULL);
			if(sock < 0) {
				if(errno != EWOULDBLOCK)
					fprintf(stderr, "Error on listening socket: %s\n", strerror(errno));
				break;
			}
			NOTBLOCKING(sock);
			sockets_[sock] = SocketInfo("tcp");
			assert(!sockets_[sock].didHandshake());
		}
	}
}

void dazeus::PluginComm::newLocalConnection() {
	std::vector<int>::iterator it;
	for(it = localServers_.begin(); it != localServers_.end(); ++it) {
		while(1) {
			int sock = accept(*it, NULL, NULL);
			if(sock < 0) {
				if(errno != EWOULDBLOCK)
					fprintf(stderr, "Error on listening socket: %s\n", strerror(errno));
				break;
			}
			NOTBLOCKING(sock);
			sockets_[sock] = SocketInfo("unix");
			assert(!sockets_[sock].didHandshake());
		}
	}
}

void dazeus::PluginComm::poll() {
	std::vector<int> toRemove;
	std::map<int,SocketInfo>::iterator it;
	for(it = sockets_.begin(); it != sockets_.end(); ++it) {
		int dev = it->first;
		SocketInfo info = it->second;

		// check if it's in error state
		struct pollfd fds[1];
		fds[0].fd = dev;
		fds[0].events = fds[0].revents = 0;
		if(::poll(fds, 1, 0) == -1) {
			perror("Failed to check socket error state");
			close(dev);
			toRemove.push_back(dev);
			continue;
		} else if(fds[0].revents & POLLERR || fds[0].revents & POLLHUP) {
			close(dev);
			toRemove.push_back(dev);
			continue;
		}

		bool appended = false;
		while(1) {
			char *readahead = (char*)malloc(512);
			ssize_t r = read(dev, readahead, 512);
			if(r == 0) {
				// end-of-file
				close(dev);
				toRemove.push_back(dev);
				break;
			} else if(r < 0) {
				if(errno != EWOULDBLOCK) {
					fprintf(stderr, "Socket error: %s\n", strerror(errno));
					close(dev);
					toRemove.push_back(dev);
				}
				free(readahead);
				break;
			}
			appended = true;
			info.readahead.append(readahead, r);
			free(readahead);
		}
		if(appended) {
			// try reading as much commands as we can
			bool parsedPacket;
			do {
				parsedPacket = false;
				if(info.waitingSize == 0) {
					std::stringstream s;
					std::string waitingSize;
					unsigned int j;
					for(j = 0; j < info.readahead.size(); ++j) {
						if(isdigit(info.readahead[j])) {
							s << info.readahead[j];
						} else if(info.readahead[j] == '{') {
							int waitingSize;
							s >> waitingSize;
							assert(s);
							info.waitingSize = waitingSize;
							break;
						}
					}
					// remove everything to j from the readahead buffer
					if(info.waitingSize != 0)
						info.readahead = info.readahead.substr(j);
				}
				if(info.waitingSize != 0) {
					if((signed)info.readahead.length() >= info.waitingSize) {
						std::string packet = info.readahead.substr(0, info.waitingSize);
						info.readahead = info.readahead.substr(info.waitingSize);
						handle(dev, packet, info);
						info.waitingSize = 0;
						parsedPacket = true;
					}
				}
			} while(parsedPacket);
		}
		it->second = info;
	}
	std::vector<int>::iterator toRemoveIt;
	for(toRemoveIt = toRemove.begin(); toRemoveIt != toRemove.end(); ++toRemoveIt) {
		sockets_.erase(*toRemoveIt);
	}
}

void dazeus::PluginComm::dispatch(const std::string &event, const std::vector<std::string> &parameters) {
	std::map<int,SocketInfo>::iterator it;
	for(it = sockets_.begin(); it != sockets_.end(); ++it) {
		SocketInfo &info = it->second;
		if(info.isSubscribed(event)) {
			info.dispatch(it->first, event, parameters);
		}
	}
}

void dazeus::PluginComm::flushCommandQueue(const std::string &nick, bool identified) {
	std::vector<Command*>::iterator cit;
	for(cit = commandQueue_.begin(); cit != commandQueue_.end(); ++cit) {
		Command *cmd = *cit;
		// If there is at least one plugin that does sender checking,
		// and the sender is not already known as identified, a whois
		// check is necessary to send it to those plugins.
		// If the whois check was done and it failed (nick is set and
		// identified is false), send it to all other relevant plugins
		// anyway.
		// Check if we have all needed information
		bool whoisRequired = false;
		std::map<int,SocketInfo>::iterator it;
		for(it = sockets_.begin(); it != sockets_.end(); ++it) {
			SocketInfo &info = it->second;
			if(info.commandMightNeedWhois(cmd->command)
			&& !cmd->network.isIdentified(cmd->origin)) {
				whoisRequired = true;
			}
		}

		// If the whois check was not done yet, do it now, handle this
		// command when the identified command comes in
		if(whoisRequired && nick != cmd->origin) {
			if(!cmd->whoisSent) {
				cmd->network.sendWhois(cmd->origin);
				cmd->whoisSent = true;
			}
			continue;
		}

		// We either don't require whois for this command, or the
		// whois status of the sender is known at this point (either
		// saved in the network, or in the 'identified' variable).
		assert(!whoisRequired || cmd->network.isIdentified(cmd->origin)
		       || nick == cmd->origin);

		std::vector<std::string> parameters;
		parameters.push_back(cmd->network.networkName());
		parameters.push_back(cmd->origin);
		parameters.push_back(cmd->channel);
		parameters.push_back(cmd->command);
		parameters.push_back(cmd->fullArgs);
		for(std::vector<std::string>::const_iterator itt = cmd->args.begin(); itt != cmd->args.end(); ++itt) {
			parameters.push_back(*itt);
		}

		for(it = sockets_.begin(); it != sockets_.end(); ++it) {
			SocketInfo &info = it->second;
			if(!info.isSubscribedToCommand(cmd->command, cmd->channel, cmd->origin,
				cmd->network.isIdentified(cmd->origin) || identified, cmd->network)) {
				continue;
			}

			// Receiver, sender, network matched; dispatch command to this plugin
			info.dispatch(it->first, "COMMAND", parameters);
		}

		cit = commandQueue_.erase(cit);
		cit--;
		delete cmd;
	}
}

void dazeus::PluginComm::messageReceived( const std::string &origin, const std::string &message,
                  const std::string &receiver, Network *n ) {
	assert(n != 0);

	GlobalConfig *c = config_->getGlobalConfig();

	std::string payload;
	if( startsWith(message, n->nick() + ":", true)
	 || startsWith(message, n->nick() + ",", true)) {
		payload = trim(message.substr(n->nick().length() + 1));
	} else if(startsWith(message, c->highlight, true)) {
		payload = trim(message.substr(c->highlight.length()));
	}

	if(payload.length() != 0) {
		// parse arguments from this string
		bool inQuoteArg = false;
		bool inEscape = false;
		bool hasCommand = false;
		std::vector<std::string> args;
		std::string stringBuilder;
		std::string fullArgs;
		// loop through characters
		for(unsigned i = 0; i < payload.length(); ++i) {
			if(hasCommand)
				fullArgs += payload.at(i);
			if(inEscape) {
				inEscape = false;
				stringBuilder += payload.at(i);
			} else if(payload.at(i) == '\\') {
				inEscape = true;
			} else if(!inQuoteArg && payload.at(i) == ' ') {
				// finish this word
				args.push_back(stringBuilder);
				stringBuilder.clear();
				hasCommand = true;
			} else if(payload.at(i) == '"') {
				inQuoteArg = !inQuoteArg;
			} else {
				stringBuilder += payload.at(i);
			}
		}
		if(stringBuilder.length() != 0)
			args.push_back(stringBuilder);

		if(args.size() > 0) {
			const std::string command = args.front();
			args.erase(args.begin());

			Command *cmd = new Command(*n);
			cmd->origin = origin;
			cmd->channel = receiver;
			cmd->command = command;
			cmd->fullArgs = trim(fullArgs);
			cmd->args = args;
			commandQueue_.push_back(cmd);
			flushCommandQueue();
		}
	}
	std::vector<std::string> args;
	args << n->networkName() << origin << receiver << message << (n->isIdentified(origin) ? "true" : "false");
	dispatch("PRIVMSG", args);
}

void dazeus::PluginComm::ircEvent(const std::string &event, const std::string &origin, const std::vector<std::string> &params, Network *n) {
	assert(n != 0);
	std::vector<std::string> args;
	args << n->networkName();
#define MIN(a) if(params.size() < a) { fprintf(stderr, "Too few parameters for event %s (%lu)\n", event.c_str(), params.size()); return; }
	if(event == "PRIVMSG") {
		MIN(2);
		messageReceived(origin, params[1], params[0], n);
	} else if(event == "NOTICE") {
		MIN(2);
		args << origin << params[0] << params[1] << (n->isIdentified(origin) ? "true" : "false");
		dispatch("NOTICE", args);
	} else if(event == "MODE" || event == "UMODE") {
		MIN(1);
		args << origin << params;
		dispatch("MODE", args);
	} else if(event == "NICK") {
		MIN(1);
		args << origin << params[0];
		dispatch("NICK", args);
	} else if(event == "JOIN") {
		MIN(1);
		args << origin << params[0];
		dispatch("JOIN", args);
	} else if(event == "PART") {
		MIN(1);
		std::string message;
		if(params.size() == 2)
			message = params[1];
		args << origin << params[0] << message;
		dispatch("PART", args);
	} else if(event == "KICK") {
		MIN(2);
		std::string nick = params[1];
		std::string message;
		if(params.size() == 3)
			message = params[2];
		args << origin << params[0] << nick << message;
		dispatch("KICK", args);
	} else if(event == "INVITE") {
		MIN(2);
		std::string channel  = params[1];
		args << origin << channel;
		dispatch("INVITE", args);
	} else if(event == "QUIT") {
		std::string message;
		if(params.size() == 1)
			message = params[0];
		args << origin << message;
		dispatch("QUIT", args);
	} else if(event == "TOPIC") {
		MIN(1);
		std::string topic;
		if(params.size() > 1)
			topic = params[1];
		args << origin << params[0] << topic;
		dispatch("TOPIC", args);
	} else if(event == "CONNECT") {
		dispatch("CONNECT", args);
	} else if(event == "DISCONNECT") {
		dispatch("DISCONNECT", args);
	} else if(event == "CTCP_REQ" || event == "CTCP") {
		MIN(1);
		// TODO: libircclient does not seem to tell us where the ctcp
		// request was sent (user or channel), so just assume it was
		// sent to our nick
		std::string to = n->nick();
		args << origin << to << params[0];
		dispatch("CTCP", args);
	} else if(event == "CTCP_REP") {
		MIN(1);
		// TODO: see above
		std::string to = n->nick();
		args << origin << to << params[0];
		dispatch("CTCP_REP", args);
	} else if(event == "CTCP_ACTION" || event == "ACTION") {
		MIN(1);
		std::string message;
		if(params.size() >= 2)
			message = params[1];
		args << origin << params[0] << message;
		dispatch("ACTION", args);
	} else if(event == "WHOIS") {
		MIN(2);
		args << origin << params[0] << params[1];
		dispatch("WHOIS", args);
		flushCommandQueue(params[0], params[1] == "true");
	} else if(event == "NAMES") {
		MIN(2);
		args << origin << params;
		dispatch("NAMES", args);
	} else if(event == "NUMERIC") {
		MIN(1);
		args << origin << params[0] << params;
		dispatch("NUMERIC", args);
	} else if(event == "ACTION_ME" || event == "CTCP_ME" || event == "PRIVMSG_ME") {
		MIN(2);
		args << origin << params;
		dispatch(event, args);
	} else if(event == "PONG") {
		args << origin << params;
		dispatch(event, args);
	} else {
		fprintf(stderr, "Unknown event: \"%s\" \"%s\" \"%s\"\n", Network::toString(n).c_str(), event.c_str(), origin.c_str());
		args << origin << params[0] << event << params;
		dispatch("UNKNOWN", args);
	}
#undef MIN
}

void dazeus::PluginComm::handle(int dev, const std::string &line, SocketInfo &info) {
	const std::vector<Network*> &networks = dazeus_->networks();
	std::vector<Network*>::const_iterator nit;

	json_error_t error;
	json_t *n = json_loads(line.c_str(), 0, &error);
	if(!n) {
		fprintf(stderr, "Got incorrect JSON, ignoring: %s\n", error.text);
		return;
	}

	std::vector<std::string> params;
	std::vector<std::string> scope;
	std::string action;

	json_t *jParams = json_object_get(n, "params");
	if(jParams) {
		if(!json_is_array(jParams)) {
			fprintf(stderr, "Got params, but of the wrong type, ignoring\n");
		} else {
			for(unsigned i = 0; i < json_array_size(jParams); ++i) {
				json_t *v = json_array_get(jParams, i);
				std::stringstream value;
				switch(json_typeof(v)) {
				case JSON_STRING: value << json_string_value(v); break;
				case JSON_INTEGER: value << json_integer_value(v); break;
				case JSON_REAL: value << json_real_value(v); break;
				case JSON_TRUE: value << "true"; break;
				case JSON_FALSE: value << "false"; break;
				default: std::cerr << "Param " << i << " is of wrong type, ignoring\n";
				}
				params.push_back(value.str());
			}
		}
	}

	json_t *jScope = json_object_get(n, "scope");
	if(jScope) {
		if(!json_is_array(jScope)) {
			fprintf(stderr, "Got scope, but of the wrong type, ignoring\n");
		} else {
			for(unsigned int i = 0; i < json_array_size(jScope); ++i) {
				json_t *v = json_array_get(jScope, i);
				scope.push_back(json_is_string(v) ? json_string_value(v) : "");
			}
		}
	}

	json_t *jAction = json_object_get(n, "get");
	if(!jAction)
		jAction = json_object_get(n, "do");
	if(jAction) {
		if(!json_is_string(jAction)) {
			fprintf(stderr, "Got action, but of the wrong type, ignoring\n");
		} else {
			action = json_string_value(jAction);
		}
	}

	json_t *response = json_object();
	// REQUEST WITHOUT A NETWORK
	if(action == "networks") {
		json_object_set_new(response, "got", json_string("networks"));
		json_object_set_new(response, "success", json_true());
		json_t *nets = json_array();
		for(nit = networks.begin(); nit != networks.end(); ++nit) {
			json_array_append_new(nets, json_string((*nit)->networkName().c_str()));
		}
		json_object_set_new(response, "networks", nets);
	} else if(action == "handshake") {
		json_object_set_new(response, "did", json_string("handshake"));
		if(info.didHandshake()) {
			json_object_set_new(response, "success", json_false());
			json_object_set_new(response, "error", json_string("Already did handshake"));
		} else if(params.size() < 4) {
			json_object_set_new(response, "success", json_false());
			json_object_set_new(response, "error", json_string("Missing parameters"));
		} else if(params[2] != "1") {
			json_object_set_new(response, "succes", json_false());
			json_object_set_new(response, "error", json_string("Protocol version must be '1'"));
		} else {
			json_object_set_new(response, "success", json_true());
			info.plugin_name = params[0];
			info.plugin_version = params[1];
			std::stringstream version(params[2]);
			version >> info.protocol_version;
			info.config_group = params[3];
			std::cout << "Plugin handshake: " << info.plugin_name << " v"
			          << info.plugin_version << " (protocol version "
			          << info.protocol_version << ", config group "
			          << info.config_group << ")" << std::endl;
		}
	// REQUESTS ON A NETWORK
	} else if(action == "channels" || action == "whois" || action == "join" || action == "part"
	       || action == "nick") {
		if(action == "channels" || action == "nick") {
			json_object_set_new(response, "got", json_string(action.c_str()));
		} else {
			json_object_set_new(response, "did", json_string(action.c_str()));
		}
		std::string network = "";
		if(params.size() > 0) {
			network = params[0];
		}
		json_object_set_new(response, "network", json_string(network.c_str()));
		Network *net = 0;
		for(nit = networks.begin(); nit != networks.end(); ++nit) {
			if((*nit)->networkName() == network) {
				net = *nit; break;
			}
		}
		if(net == 0) {
			json_object_set_new(response, "success", json_false());
			json_object_set_new(response, "error", json_string("Not on that network"));
		} else {
			if(action == "channels") {
				json_object_set_new(response, "success", json_true());
				json_t *chans = json_array();
				const std::vector<std::string> &channels = net->joinedChannels();
				for(unsigned i = 0; i < channels.size(); ++i) {
					json_array_append_new(chans, json_string(channels[i].c_str()));
				}
				json_object_set_new(response, "channels", chans);
			} else if(action == "whois" || action == "join" || action == "part") {
				if(params.size() < 2) {
					json_object_set_new(response, "success", json_false());
					json_object_set_new(response, "error", json_string("Missing parameters"));
				} else {
					json_object_set_new(response, "success", json_true());
					if(action == "whois") {
						net->sendWhois(params[1]);
					} else if(action == "join") {
						net->joinChannel(params[1]);
					} else if(action == "part") {
						net->leaveChannel(params[1]);
					} else {
						assert(false);
						return;
					}
				}
			} else if(action == "nick") {
				json_object_set_new(response, "success", json_true());
				json_object_set_new(response, "nick", json_string(net->nick().c_str()));
			} else {
				assert(false);
				return;
			}
		}
	// REQUESTS ON A CHANNEL
	} else if(action == "message" || action == "action" || action == "names") {
		json_object_set_new(response, "did", json_string(action.c_str()));
		if(params.size() < 2 || (action != "names" && params.size() < 3)) {
			fprintf(stderr, "Wrong parameter size for message, skipping.\n");
			return;
		}
		std::string network = params[0];
		std::string receiver = params[1];
		std::string message;
		if(action != "names") {
			message = params[2];
		}
		json_object_set_new(response, "network", json_string(network.c_str()));
		if(action == "names") {
			json_object_set_new(response, "channel", json_string(receiver.c_str()));
		} else {
			json_object_set_new(response, "receiver", json_string(receiver.c_str()));
			json_object_set_new(response, "message", json_string(message.c_str()));
		}

		bool netfound = false;
		for(nit = networks.begin(); nit != networks.end(); ++nit) {
			Network *n = *nit;
			if(n->networkName() == network) {
				netfound = true;
				if(receiver.substr(0, 1) != "#" || contains_ci(n->joinedChannels(), strToLower(receiver))) {
					json_object_set_new(response, "success", json_true());
					if(action == "names") {
						n->names(receiver);
					} else if(action == "message") {
						n->say(receiver, message);
					} else {
						n->action(receiver, message);
					}
				} else {
					fprintf(stderr, "Request for communication to network %s receiver %s, but not in that channel, dropping\n",
						network.c_str(), receiver.c_str());
					json_object_set_new(response, "success", json_false());
					json_object_set_new(response, "error", json_string("Not in that channel"));
				}
				break;
			}
		}
		if(!netfound) {
			fprintf(stderr, "Request for communication to network %s, but that network isn't joined, dropping\n", network.c_str());
			json_object_set_new(response, "success", json_false());
			json_object_set_new(response, "error", json_string("Not on that network"));
		}
	// REQUESTS ON DAZEUS ITSELF
	} else if(action == "subscribe") {
		json_object_set_new(response, "did", json_string("subscribe"));
		json_object_set_new(response, "success", json_true());
		int added = 0;
		std::vector<std::string>::const_iterator pit;
		for(pit = params.begin(); pit != params.end(); ++pit) {
			if(info.subscribe(*pit))
				++added;
		}
		json_object_set_new(response, "added", json_integer(added));
	} else if(action == "unsubscribe") {
		json_object_set_new(response, "did", json_string("unsubscribe"));
		json_object_set_new(response, "success", json_true());
		int removed = 0;
		std::vector<std::string>::const_iterator pit;
		for(pit = params.begin(); pit != params.end(); ++pit) {
			if(info.unsubscribe(*pit))
				++removed;
		}
		json_object_set_new(response, "removed", json_integer(removed));
	} else if(action == "command") {
		json_object_set_new(response, "did", json_string("command"));
		// {"do":"command", "params":["cmdname"]}
		// {"do":"command", "params":["cmdname", "network"]}
		// {"do":"command", "params":["cmdname", "network", true, "sender"]}
		// {"do":"command", "params":["cmdname", "network", false, "receiver"]}
		if(params.size() == 0) {
			json_object_set_new(response, "success", json_false());
			json_object_set_new(response, "error", json_string("Missing parameters"));
		} else {
			std::string commandName = params.front();
			params.erase(params.begin());
			RequirementInfo *req = 0;
			Network *net = 0;
			if(params.size() > 0) {
				for(nit = networks.begin(); nit != networks.end(); ++nit) {
					if((*nit)->networkName() == params.at(0)) {
						net = *nit; break;
					}
				}
			}
			if(params.size() == 0) {
				// Add it as a global command
				req = new RequirementInfo();
			} else if(params.size() == 1) {
				// Network requirement
				req = new RequirementInfo(net);
			} else if(params.size() == 3) {
				// Network and sender/receiver requirement
				bool isSender = params.at(1) == "true";
				req = new RequirementInfo(net, params.at(2), isSender);
			} else {
				json_object_set_new(response, "success", json_false());
				json_object_set_new(response, "error", json_string("Wrong number of parameters"));
			}
			if(req != NULL) {
				info.subscribeToCommand(commandName, req);
				json_object_set_new(response, "success", json_true());
			}
		}
	} else if(action == "property") {
		json_object_set_new(response, "did", json_string("property"));

		std::string network, receiver, sender;
		switch(scope.size()) {
		// fall-throughs
		case 3: sender   = scope[2];
		case 2: receiver = scope[1];
		case 1: network  = scope[0];
		default: break;
		}

		if(params.size() < 2) {
			json_object_set_new(response, "success", json_false());
			json_object_set_new(response, "error", json_string("Missing parameters"));
		} else if(params[0] == "get") {
			std::string value = database_->property(params[1], network, receiver, sender);
			json_object_set_new(response, "success", json_true());
			json_object_set_new(response, "variable", json_string(params[1].c_str()));
			if(value.length() > 0) {
				json_object_set_new(response, "value", json_string(value.c_str()));
			}
		} else if(params[0] == "set") {
			if(params.size() < 3) {
				json_object_set_new(response, "success", json_false());
				json_object_set_new(response, "error", json_string("Missing parameters"));
			} else {
				database_->setProperty(params[1], params[2], network, receiver, sender);
				json_object_set_new(response, "success", json_true());
			}
		} else if(params[0] == "unset") {
			if(params.size() < 2) {
				json_object_set_new(response, "success", json_false());
				json_object_set_new(response, "error", json_string("Missing parameters"));
			} else {
				database_->setProperty(params[1], std::string(), network, receiver, sender);
				json_object_set_new(response, "success", json_true());
			}
		} else if(params[0] == "keys") {
			std::vector<std::string> pKeys = database_->propertyKeys(params[1], network, receiver, sender);
			json_t *keys = json_array();
			std::vector<std::string>::iterator kit;
			for(kit = pKeys.begin(); kit != pKeys.end(); ++kit) {
				json_array_append_new(keys, json_string(kit->c_str()));
			}
			json_object_set_new(response, "keys", keys);
			json_object_set_new(response, "success", json_true());
		} else {
			json_object_set_new(response, "success", json_false());
			json_object_set_new(response, "error", json_string("Did not understand request"));
		}
	} else if(action == "config") {
		json_object_set_new(response, "got", json_string("config"));
		if(params.size() != 2) {
			json_object_set_new(response, "success", json_false());
			json_object_set_new(response, "error", json_string("Missing parameters"));
		} else {
			std::string configtype = params[0];
			std::string configvar  = params[1];

			if(configtype == "plugin" && !info.didHandshake()) {
				json_object_set_new(response, "success", json_false());
				json_object_set_new(response, "error", json_string("Need to do handshake for retrieving plugin configuration"));
			} else if(configtype == "plugin") {
				json_object_set_new(response, "success", json_true());
				json_object_set_new(response, "group", json_string(configtype.c_str()));
				json_object_set_new(response, "variable", json_string(configvar.c_str()));

				// Get the right plugin config
				const std::vector<PluginConfig*> &plugins = config_->getPlugins();
				for(std::vector<PluginConfig*>::const_iterator cit = plugins.begin(); cit != plugins.end(); ++cit) {
					PluginConfig *pc = *cit;
					if(pc->name == info.config_group) {
						std::map<std::string,std::string>::iterator configIt = pc->config.find(configvar);
						if(configIt != pc->config.end()) {
							json_object_set_new(response, "value", json_string(configIt->second.c_str()));
							break;
						}
					}
				}
			} else if(configtype == "core") {
				GlobalConfig *global = config_->getGlobalConfig();
				json_object_set_new(response, "success", json_true());
				json_object_set_new(response, "group", json_string(configtype.c_str()));
				json_object_set_new(response, "variable", json_string(configvar.c_str()));
				if(configvar == "nickname") {
					json_object_set_new(response, "value", json_string(global->default_nickname.c_str()));
				} else if(configvar == "username") {
					json_object_set_new(response, "value", json_string(global->default_username.c_str()));
				} else if(configvar == "fullname") {
					json_object_set_new(response, "value", json_string(global->default_fullname.c_str()));
				} else if(configvar == "highlight") {
					json_object_set_new(response, "value", json_string(global->highlight.c_str()));
				}
			} else {
				json_object_set_new(response, "success", json_false());
				json_object_set_new(response, "error", json_string("Unrecognised config group"));
			}
		}
	} else {
		json_object_set_new(response, "success", json_false());
		json_object_set_new(response, "error", json_string("Did not understand request"));
	}

	char *raw_json = json_dumps(response, 0);
	std::string jsonMsg = raw_json;
	free(raw_json);
	std::stringstream mstr;
	mstr << jsonMsg.length();
	mstr << jsonMsg;
	mstr << "\n";
	std::string finalResponse = mstr.str();
	if(write(dev, finalResponse.c_str(), finalResponse.length()) != (unsigned)finalResponse.length()) {
		fprintf(stderr, "Failed to write correct number of JSON bytes to client socket.\n");
		close(dev);
	}

	json_decref(n);
	json_decref(response);
}
