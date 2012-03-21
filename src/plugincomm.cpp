/**
 * Copyright (c) Sjors Gielen, 2011
 * See LICENSE for license.
 */

#define __STDC_LIMIT_MACROS
#include <stdint.h>
#include <libjson.h>
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

#define NOTBLOCKING(x) fcntl(x, F_SETFL, fcntl(x, F_GETFL) | O_NONBLOCK)

PluginComm::PluginComm(Database *d, Config *c, DaZeus *bot)
: database_(d)
, config_(c)
, dazeus_(bot)
{
}

PluginComm::~PluginComm() {
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

void PluginComm::run() {
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
		Server *active = (*nit)->activeServer();
		if(active) {
			int ircmaxfd = 0;
			active->addDescriptors(&sockets, &out_sockets, &ircmaxfd);
			if(ircmaxfd > highest)
				highest = ircmaxfd;
		}
	}
	// TODO dynamic?
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	int socks = select(highest + 1, &sockets, &out_sockets, NULL, &timeout);
	if(socks < 0) {
		fprintf(stderr, "select() failed: %s\n", strerror(errno));
		return;
	}
	else if(socks == 0) {
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
		Server *active = (*nit)->activeServer();
		if(active) {
			active->processDescriptors(&sockets, &out_sockets);
		}
	}
}

void PluginComm::init() {
	std::stringstream socketStr;
	std::map<std::string,std::string> sockets = config_->groupConfig("sockets");

	std::map<std::string,std::string>::iterator sockIt = sockets.find("sockets");
	if(sockIt == sockets.end()) {
		fprintf(stderr, "Warning: Configuration file contains no sockets to open.\n");
		return;
	}

	socketStr << sockIt->second;
	int numSockets;
	socketStr >> numSockets;
	if(!socketStr) {
		fprintf(stderr, "Could not read how many sockets to read, aborting.\n");
		abort();
	}
	for(int i = 1; i <= numSockets; ++i) {
		socketStr.clear();
		socketStr.str(std::string());
		socketStr << "socket" << i;
		sockIt = sockets.find(socketStr.str());
		if(sockIt == sockets.end()) {
			fprintf(stderr, "Warning: Socket %d declared, but not defined, skipping\n", i);
			continue;
		}
		std::string socket = sockIt->second;
		size_t colon = socket.find(':');
		if(colon == std::string::npos) {
			fprintf(stderr, "(PluginComm) Did not understand socket option %d: %s\n", i, socket.c_str());
			continue;
		}
		std::string socketType = socket.substr(0, colon);
		socket = socket.substr(colon + 1);
		if(socketType == "unix") {
			unlink(socket.c_str());
			int server = ::socket(AF_UNIX, SOCK_STREAM, 0);
			if(server <= 0) {
				fprintf(stderr, "(PluginComm) Failed to create socket for %s: %s\n", socket.c_str(), strerror(errno));
				continue;
			}
			NOTBLOCKING(server);
			struct sockaddr_un addr;
			addr.sun_family = AF_UNIX;
			strcpy(addr.sun_path, socket.c_str());
			if(bind(server, (struct sockaddr*) &addr, sizeof(struct sockaddr_un)) < 0) {
				close(server);
				fprintf(stderr, "(PluginComm) Failed to start listening for local connections on %s: %s\n", socket.c_str(), strerror(errno));
				continue;
			}
			if(listen(server, 5) < 0) {
				close(server);
				unlink(socket.c_str());
			}
			localServers_.push_back(server);
		} else if(socketType == "tcp") {
			colon = socket.find(':');
			std::string tcpHost, tcpPort;
			if(colon == std::string::npos) {
				tcpHost = "localhost";
				tcpPort = socket;
			} else {
				tcpHost = socket.substr(0, colon);
				tcpPort = socket.substr(colon + 1);
			}
			unsigned int port;
			std::stringstream ports;
			ports << tcpPort;
			ports >> port;
			if(!ports || port > UINT16_MAX) {
				fprintf(stderr, "(PluginComm) Invalid TCP port number for socket %d\n", i);
				continue;
			}

			struct addrinfo *hints, *result;
			hints = (struct addrinfo*)calloc(1, sizeof(struct addrinfo));
			hints->ai_flags = 0;
			hints->ai_family = AF_INET;
			hints->ai_socktype = SOCK_STREAM;
			hints->ai_protocol = 0;

			int s = getaddrinfo(tcpHost.c_str(), tcpPort.c_str(), hints, &result);
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
				fprintf(stderr, "(PluginComm) Failed to start listening for TCP connections on %s: %s\n",
					socket.c_str(), strerror(errno));
			}
			if(listen(server, 5) < 0) {
				close(server);
			}
			tcpServers_.push_back(server);
		} else {
			fprintf(stderr, "(PluginComm) Skipping socket %d: unknown type %s\n", i, socketType.c_str());
		}
	}
}

void PluginComm::newTcpConnection() {
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
		}
	}
}

void PluginComm::newLocalConnection() {
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
		}
	}
}

void PluginComm::poll() {
	std::vector<int> toRemove;
	std::map<int,SocketInfo>::iterator it;
	for(it = sockets_.begin(); it != sockets_.end(); ++it) {
		int dev = it->first;
		SocketInfo info = it->second;
		bool appended = false;
		while(1) {
			char *readahead = (char*)malloc(512);
			ssize_t r = read(dev, readahead, 512);
			if(r == 0) {
				// end-of-file
				toRemove.push_back(dev);
				break;
			} else if(r < 0) {
				if(errno != EWOULDBLOCK) {
					fprintf(stderr, "Socket error: %s\n", strerror(errno));
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
						info.readahead = info.readahead.substr(info.waitingSize + 1);
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

void PluginComm::dispatch(const std::string &event, const std::vector<std::string> &parameters) {
	std::map<int,SocketInfo>::iterator it;
	for(it = sockets_.begin(); it != sockets_.end(); ++it) {
		SocketInfo &info = it->second;
		if(info.isSubscribed(event)) {
			info.dispatch(it->first, event, parameters);
		}
	}
}

void PluginComm::flushCommandQueue(const std::string &nick, bool identified) {
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

void PluginComm::messageReceived( const std::string &origin, const std::string &message,
                  const std::string &receiver, Network *n ) {
	assert(n != 0);

	std::map<std::string,std::string> config = config_->groupConfig("general");
	std::map<std::string,std::string>::iterator configIt;

	std::string payload;
	std::string highlightChar = "}";
	configIt = config.find("highlightCharacter");
	if(configIt != config.end())
		highlightChar = configIt->second;

	if( startsWith(message, n->user()->nick() + ":", true)
	 || startsWith(message, n->user()->nick() + ",", true)) {
		payload = trim(message.substr(n->user()->nick().length() + 1));
	} else if(startsWith(message, highlightChar, true)) {
		payload = trim(message.substr(highlightChar.length()));
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
			const std::string &command = args.front();
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

void PluginComm::ircEvent(const std::string &event, const std::string &origin, const std::vector<std::string> &params, Network *n) {
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
		std::string to = n->user()->nick();
		args << origin << to << params[0];
		dispatch("CTCP", args);
	} else if(event == "CTCP_REP") {
		MIN(1);
		// TODO: see above
		std::string to = n->user()->nick();
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
	} else {
		fprintf(stderr, "Unknown event: \"%s\" \"%s\" \"%s\"\n", Network::toString(n).c_str(), event.c_str(), origin.c_str());
		args << origin << params[0] << event << params;
		dispatch("UNKNOWN", args);
	}
#undef MIN
}

void PluginComm::handle(int dev, const std::string &line, SocketInfo &info) {
	const std::vector<Network*> &networks = dazeus_->networks();
	std::vector<Network*>::const_iterator nit;

	JSONNode n;
	try {
		n = libjson::parse(line.c_str());
	} catch(std::invalid_argument &exception) {
		fprintf(stderr, "Got incorrect JSON, ignoring\n");
		return;
	}
	std::vector<std::string> params;
	std::vector<std::string> scope;
	JSONNode::const_iterator i = n.begin();
	std::string action;
	while(i != n.end()) {
		if(i->name() == "params") {
			if(i->type() != JSON_ARRAY) {
				fprintf(stderr, "Got params, but not of array type\n");
				continue;
			}
			JSONNode::const_iterator j = i->begin();
			while(j != i->end()) {
				std::string s = libjson::to_std_string(j->as_string());
				params.push_back(s);
				j++;
			}
		} else if(i->name() == "scope") {
			if(i->type() != JSON_ARRAY) {
				fprintf(stderr, "Got scope, but not of array type\n");
				continue;
			}
			JSONNode::const_iterator j = i->begin();
			while(j != i->end()) {
				std::string s = libjson::to_std_string(j->as_string());
				scope.push_back(s);
				j++;
			}
		} else if(i->name() == "get") {
			action = libjson::to_std_string(i->as_string());
		} else if(i->name() == "do") {
			action = libjson::to_std_string(i->as_string());
		}
		i++;
	}

	JSONNode response(JSON_NODE);
	// REQUEST WITHOUT A NETWORK
	if(action == "networks") {
		response.push_back(JSONNode("got", "networks"));
		response.push_back(JSONNode("success", true));
		JSONNode nets(JSON_ARRAY);
		nets.set_name("networks");
		for(nit = networks.begin(); nit != networks.end(); ++nit) {
			nets.push_back(JSONNode("", libjson::to_json_string((*nit)->networkName())));
		}
		response.push_back(nets);
	// REQUESTS ON A NETWORK
	} else if(action == "channels" || action == "whois" || action == "join" || action == "part"
	       || action == "nick") {
		if(action == "channels" || action == "nick") {
			response.push_back(JSONNode("got", libjson::to_json_string(action)));
		} else {
			response.push_back(JSONNode("did", libjson::to_json_string(action)));
		}
		std::string network = "";
		if(params.size() > 0) {
			network = params[0];
		}
		response.push_back(JSONNode("network", libjson::to_json_string(network)));
		Network *net = 0;
		for(nit = networks.begin(); nit != networks.end(); ++nit) {
			if((*nit)->networkName() == network) {
				net = *nit; break;
			}
		}
		if(net == 0) {
			response.push_back(JSONNode("success", false));
			response.push_back(JSONNode("error", "Not on that network"));
		} else {
			if(action == "channels") {
				response.push_back(JSONNode("success", true));
				JSONNode chans(JSON_ARRAY);
				chans.set_name("channels");
				const std::vector<std::string> &channels = net->joinedChannels();
				for(unsigned i = 0; i < channels.size(); ++i) {
					chans.push_back(JSONNode("", libjson::to_json_string(channels[i])));
				}
				response.push_back(chans);
			} else if(action == "whois" || action == "join" || action == "part") {
				if(params.size() < 2) {
					response.push_back(JSONNode("success", false));
					response.push_back(JSONNode("error", "Missing parameters"));
				} else {
					response.push_back(JSONNode("success", true));
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
				response.push_back(JSONNode("success", true));
				response.push_back(JSONNode("nick", libjson::to_json_string(net->user()->nick())));
			} else {
				assert(false);
				return;
			}
		}
	// REQUESTS ON A CHANNEL
	} else if(action == "message" || action == "action" || action == "names") {
		response.push_back(JSONNode("did", libjson::to_json_string(action)));
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
		response.push_back(JSONNode("network", libjson::to_json_string(network)));
		if(action == "names") {
			response.push_back(JSONNode("channel", libjson::to_json_string(receiver)));
		} else {
			response.push_back(JSONNode("receiver", libjson::to_json_string(receiver)));
			response.push_back(JSONNode("message", libjson::to_json_string(message)));
		}

		bool netfound = false;
		for(nit = networks.begin(); nit != networks.end(); ++nit) {
			Network *n = *nit;
			if(n->networkName() == network) {
				netfound = true;
				if(receiver.substr(0, 1) != "#" || contains(n->joinedChannels(), strToLower(receiver))) {
					response.push_back(JSONNode("success", true));
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
					response.push_back(JSONNode("success", false));
					response.push_back(JSONNode("error", "Not in that chanel"));
				}
				break;
			}
		}
		if(!netfound) {
			fprintf(stderr, "Request for communication to network %s, but that network isn't joined, dropping\n", network.c_str());
			response.push_back(JSONNode("success", false));
			response.push_back(JSONNode("error", "Not on that network"));
		}
	// REQUESTS ON DAZEUS ITSELF
	} else if(action == "subscribe") {
		response.push_back(JSONNode("did", "subscribe"));
		response.push_back(JSONNode("success", true));
		int added = 0;
		foreach(const std::string &event, params) {
			if(info.subscribe(event))
				++added;
		}
		response.push_back(JSONNode("added", added));
	} else if(action == "unsubscribe") {
		response.push_back(JSONNode("did", "unsubscribe"));
		response.push_back(JSONNode("success", true));
		int removed = 0;
		foreach(const std::string &event, params) {
			if(info.unsubscribe(event))
				++removed;
		}
		response.push_back(JSONNode("removed", removed));
	} else if(action == "command") {
		response.push_back(JSONNode("did", "command"));
		// {"do":"command", "params":["cmdname"]}
		// {"do":"command", "params":["cmdname", "network"]}
		// {"do":"command", "params":["cmdname", "network", true, "sender"]}
		// {"do":"command", "params":["cmdname", "network", false, "receiver"]}
		if(params.size() == 0) {
			response.push_back(JSONNode("success", false));
			response.push_back(JSONNode("error", "Missing parameters"));
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
				response.push_back(JSONNode("success", false));
				response.push_back(JSONNode("error", "Wrong number of parameters"));
			}
			if(req != NULL) {
				info.subscribeToCommand(commandName, req);
				response.push_back(JSONNode("success", true));
			}
		}
	} else if(action == "property") {
		response.push_back(JSONNode("did", "property"));

		std::string network, receiver, sender;
		switch(scope.size()) {
		// fall-throughs
		case 3: sender   = scope[2];
		case 2: receiver = scope[1];
		case 1: network  = scope[0];
		default: break;
		}

		if(params.size() < 2) {
			response.push_back(JSONNode("success", false));
			response.push_back(JSONNode("error", "Missing parameters"));
		} else if(params[0] == "get") {
#define Q(x) QString::fromStdString(x)
			QVariant value = database_->property(Q(params[1]), Q(network), Q(receiver), Q(sender));
			response.push_back(JSONNode("success", true));
			response.push_back(JSONNode("variable", libjson::to_json_string(params[1])));
			if(!value.isNull()) {
				response.push_back(JSONNode("value", libjson::to_json_string(value.toString().toStdString())));
			}
		} else if(params[0] == "set") {
			if(params.size() < 3) {
				response.push_back(JSONNode("success", false));
				response.push_back(JSONNode("error", "Missing parameters"));
			} else {
				database_->setProperty(Q(params[1]), Q(params[2]), Q(network), Q(receiver), Q(sender));
				response.push_back(JSONNode("success", true));
			}
		} else if(params[0] == "unset") {
			if(params.size() < 2) {
				response.push_back(JSONNode("success", false));
				response.push_back(JSONNode("error", "Missing parameters"));
			} else {
				database_->setProperty(Q(params[1]), QVariant(), Q(network), Q(receiver), Q(sender));
				response.push_back(JSONNode("success", true));
			}
		} else if(params[0] == "keys") {
			QStringList qKeys = database_->propertyKeys(Q(params[1]), Q(network), Q(receiver), Q(sender));
			std::vector<std::string> dKeys;
			foreach(const QString &k, qKeys) {
				dKeys.push_back(k.toStdString());
			}
			JSONNode keys(JSON_ARRAY);
			keys.set_name("keys");
			std::vector<std::string>::iterator kit;
			for(kit = dKeys.begin(); kit != dKeys.end(); ++kit) {
				keys.push_back(JSONNode("", libjson::to_json_string(*kit)));
			}
			response.push_back(keys);
			response.push_back(JSONNode("success", true));
#undef Q
		} else {
			response.push_back(JSONNode("success", false));
			response.push_back(JSONNode("error", "Did not understand request"));
		}
	} else if(action == "config") {
		if(params.size() < 1) {
			response.push_back(JSONNode("success", false));
			response.push_back(JSONNode("error", "Missing parameters"));
		} else {
			std::vector<std::string> parts = split(params[0], ".");
			std::string group = parts.front();
			parts.erase(parts.begin());
			std::string name = join(parts, ".");

			std::stringstream configName;
			configName << "plugin " << group;
			std::map<std::string,std::string> groupConfig = config_->groupConfig(configName.str());
			std::map<std::string,std::string>::iterator configIt = groupConfig.find(name);
			response.push_back(JSONNode("success", true));
			response.push_back(JSONNode("variable", libjson::to_json_string(params[0])));
			if(configIt != groupConfig.end()) {
				response.push_back(JSONNode("value", libjson::to_json_string(configIt->second)));
			}
		}
	} else {
		response.push_back(JSONNode("success", false));
		response.push_back(JSONNode("error", "Did not understand request"));
	}

	std::string jsonMsg = libjson::to_std_string(response.write());
	std::stringstream mstr;
	mstr << jsonMsg.length();
	mstr << jsonMsg;
	mstr << "\n";
	if(write(dev, mstr.str().c_str(), mstr.str().length()) != (unsigned)mstr.str().length()) {
		fprintf(stderr, "Failed to write correct number of JSON bytes to client socket.\n");
		close(dev);
	}
}
