/**
 * Copyright (c) Sjors Gielen, 2011
 * See LICENSE for license.
 */

#include <QtCore/QList>
#include <QtCore/QDebug>
#include <QtCore/QFile>
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

#include "plugincomm.h"
#include "network.h"
#include "config.h"
#include "server.h"
#include "config.h"
#include "dazeus.h"
#include "database.h"

#define NOTBLOCKING(x) fcntl(x, F_SETFL, fcntl(x, F_GETFL) | O_NONBLOCK)

PluginComm::PluginComm(Database *d, Config *c, DaZeus *bot)
: QObject()
, database_(d)
, config_(c)
, dazeus_(bot)
, timer_(new QTimer())
{
	// TODO don't do this with a timer, do it with a real event loop
	connect(timer_, SIGNAL(timeout()),
	        this,   SLOT(      run()));
	timer_->setSingleShot(false);
	timer_->start(50);
}

PluginComm::~PluginComm() {
	foreach(int s, sockets_.keys()) {
		close(s);
	}
	foreach(int s, tcpServers_) {
		close(s);
	}
	foreach(int s, localServers_) {
		close(s);
	}
}

void PluginComm::run() {
	fd_set sockets, out_sockets;
	int highest = 0;
	struct timeval timeout;
	FD_ZERO(&sockets);
	FD_ZERO(&out_sockets);
	foreach(int tcpServer, tcpServers_) {
		if(tcpServer > highest)
			highest = tcpServer;
		FD_SET(tcpServer, &sockets);
	}
	foreach(int localServer,localServers_) {
		if(localServer > highest)
			highest = localServer;
		FD_SET(localServer, &sockets);
	}
	foreach(int socket, sockets_.keys()) {
		if(socket > highest)
			highest = socket;
		FD_SET(socket, &sockets);
	}
	// and add the IRC descriptors
	foreach(Network *n, dazeus_->networks()) {
		Server *active = n->activeServer();
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
		qWarning() << "select() failed: " << strerror(errno);
		return;
	}
	else if(socks == 0) {
		return;
	}
	foreach(int tcpServer, tcpServers_) {
		if(FD_ISSET(tcpServer, &sockets)) {
			newTcpConnection();
			break;
		}
	}
	foreach(int localServer, localServers_) {
		if(FD_ISSET(localServer, &sockets)) {
			newLocalConnection();
			break;
		}
	}
	foreach(int socket, sockets_.keys()) {
		if(FD_ISSET(socket, &sockets)) {
			poll();
			break;
		}
	}
	foreach(Network *n, dazeus_->networks()) {
		Server *active = n->activeServer();
		if(active) {
			active->processDescriptors(&sockets, &out_sockets);
		}
	}
}

void PluginComm::init() {
	int sockets = config_->groupConfig("sockets").value("sockets").toInt();
	for(int i = 1; i <= sockets; ++i) {
		QString socket = config_->groupConfig("sockets").value("socket" + QString::number(i)).toString();
		int colon = socket.indexOf(':');
		if(colon == -1) {
			qWarning() << "(PluginComm) Did not understand socket option " << i << ":" << socket;
			continue;
		}
		QString socketType = socket.left(colon);
		socket = socket.mid(colon + 1);
		if(socketType == "unix") {
			QFile::remove(socket);
			int server = ::socket(AF_UNIX, SOCK_STREAM, 0);
			if(server <= 0) {
				qWarning() << "(PluginComm) Failed to create socket for "
				           << socket << ": " << strerror(errno);
				continue;
			}
			NOTBLOCKING(server);
			struct sockaddr_un addr;
			addr.sun_family = AF_UNIX;
			strcpy(addr.sun_path, socket.toLatin1());
			if(bind(server, (struct sockaddr*) &addr, sizeof(struct sockaddr_un)) < 0) {
				close(server);
				qWarning() << "(PluginComm) Failed to start listening for local connections on "
				           << socket << ": " << strerror(errno);
			}
			if(listen(server, 5) < 0) {
				close(server);
				QFile::remove(socket);
			}
			localServers_.append(server);
		} else if(socketType == "tcp") {
			colon = socket.indexOf(':');
			QString tcpHost, tcpPort;
			if(colon == -1) {
				tcpHost = "localhost";
				tcpPort = socket;
			} else {
				tcpHost = socket.left(colon);
				tcpPort = socket.mid(colon + 1);
			}
			bool ok = false;
			unsigned int port = tcpPort.toUInt(&ok);
			if(!ok || port > UINT16_MAX) {
				qWarning() << "(PluginComm) Invalid TCP port number for socket " << i;
				continue;
			}

			struct addrinfo *hints, *result;
			hints = (struct addrinfo*)calloc(1, sizeof(struct addrinfo));
			hints->ai_flags = 0;
			hints->ai_family = AF_INET;
			hints->ai_socktype = SOCK_STREAM;
			hints->ai_protocol = 0;

			int s = getaddrinfo(tcpHost.toLatin1(), tcpPort.toLatin1(), hints, &result);
			free(hints);
			hints = 0;

			if(s != 0) {
				qWarning() << "(PluginComm) Failed to resolve TCP address: " << (char*)gai_strerror(s);
				continue;
			}

			int server = ::socket(AF_INET, SOCK_STREAM, 0);
			if(server <= 0) {
				qWarning() << "(PluginComm) failed to create socket: " << strerror(errno);
				continue;
			}
			NOTBLOCKING(server);
			if(bind(server, result->ai_addr, result->ai_addrlen) < 0) {
				close(server);
				qWarning() << "(PluginComm) Failed to start listening for local connections on "
				           << socket << ": " << strerror(errno);
			}
			if(listen(server, 5) < 0) {
				close(server);
				QFile::remove(socket);
			}
			tcpServers_.append(server);
		} else {
			qWarning() << "(PluginComm) Skipping socket " << i << ": unknown type " << socketType;
		}
	}
}

void PluginComm::newTcpConnection() {
	for(int i = 0; i < tcpServers_.length(); ++i) {
		int s = tcpServers_[i];
		while(1) {
			int sock = accept(s, NULL, NULL);
			if(sock < 0) {
				if(errno != EWOULDBLOCK)
					qWarning() << "Error on listening socket: " << strerror(errno);
				break;
			}
			NOTBLOCKING(sock);
			sockets_.insert(sock, SocketInfo("tcp"));
		}
	}
}

void PluginComm::newLocalConnection() {
	for(int i = 0; i < localServers_.length(); ++i) {
		int s = localServers_[i];
		while(1) {
			int sock = accept(s, NULL, NULL);
			if(sock < 0) {
				if(errno != EWOULDBLOCK)
					qWarning() << "Error on listening socket: " << strerror(errno);
				break;
			}
			NOTBLOCKING(sock);
			sockets_.insert(sock, SocketInfo("unix"));
		}
	}
}

void PluginComm::poll() {
	QList<int> toRemove_;
	QList<int> socks = sockets_.keys();
	for(int i = 0; i < socks.size(); ++i) {
		int dev = socks[i];
		SocketInfo info = sockets_[dev];
		bool appended = false;
		while(1) {
			char *readahead = (char*)malloc(512);
			ssize_t r = read(dev, readahead, 512);
			if(r == 0) {
				// end-of-file
				toRemove_.append(dev);
				break;
			} else if(r < 0) {
				if(errno != EWOULDBLOCK) {
					qWarning() << "Socket error: " << strerror(errno);
					toRemove_.append(dev);
				}
				free(readahead);
				break;
			}
			appended = true;
			info.readahead.append(QByteArray(readahead, r));
			free(readahead);
		}
		if(appended) {
			// try reading as much commands as we can
			bool parsedPacket;
			do {
				parsedPacket = false;
				if(info.waitingSize == 0) {
					QByteArray waitingSize;
					int j;
					for(j = 0; j < info.readahead.size(); ++j) {
						if(isdigit(info.readahead[j])) {
							waitingSize.append(info.readahead[j]);
						} else if(info.readahead[j] == '{') {
							info.waitingSize = waitingSize.toInt();
							break;
						}
					}
					// remove everything to j from the readahead buffer
					if(info.waitingSize != 0)
						info.readahead = info.readahead.mid(j);
				}
				if(info.waitingSize != 0) {
					if(info.readahead.length() >= info.waitingSize) {
						QByteArray packet = info.readahead.left(info.waitingSize);
						info.readahead = info.readahead.mid(info.waitingSize + 1);
						handle(dev, packet, info);
						info.waitingSize = 0;
						parsedPacket = true;
					}
				}
			} while(parsedPacket);
		}
		sockets_[dev] = info;
	}
	foreach(int i, toRemove_) {
		sockets_.remove(i);
	}
}

void PluginComm::dispatch(const QString &event, const QStringList &parameters) {
	foreach(int i, sockets_.keys()) {
		SocketInfo &info = sockets_[i];
		if(info.isSubscribed(event)) {
			info.dispatch(i, event, parameters);
		}
	}
}

void PluginComm::flushCommandQueue(const QString &nick, bool identified) {
	int i;
	for(i = commandQueue_.length() - 1; i >= 0; --i) {
		Command *cmd = commandQueue_.at(i);
		// If there is at least one plugin that does sender checking,
		// and the sender is not already known as identified, a whois
		// check is necessary to send it to those plugins.
		// If the whois check was done and it failed (nick is set and
		// identified is false), send it to all other relevant plugins
		// anyway.
		// Check if we have all needed information
		bool whoisRequired = false;
		foreach(int i, sockets_.keys()) {
			SocketInfo &info = sockets_[i];
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
		Q_ASSERT(!whoisRequired || cmd->network.isIdentified(cmd->origin)
		       || nick == cmd->origin);

		QStringList parameters;
		parameters << cmd->network.networkName() << cmd->origin
		           << cmd->channel << cmd->command << cmd->fullArgs
		           << cmd->args;

		foreach(int i, sockets_.keys()) {
			SocketInfo &info = sockets_[i];
			if(!info.isSubscribedToCommand(cmd->command, cmd->channel, cmd->origin,
				cmd->network.isIdentified(cmd->origin) || identified, cmd->network)) {
				continue;
			}

			// Receiver, sender, network matched; dispatch command to this plugin
			info.dispatch(i, "COMMAND", parameters);
		}

		commandQueue_.takeAt(i);
		delete cmd;
	}
}

void PluginComm::messageReceived( const QString &origin, const QString &message,
                              Irc::Buffer *buffer ) {
	Network *n = Network::fromBuffer(buffer);
	Q_ASSERT(n != 0);

	QString payload;
	QString highlightChar = config_->groupConfig("general").value("highlightCharacter").toString();
	if(highlightChar.isNull())
		highlightChar = "}";

	if( message.startsWith(n->user()->nick() + ":", Qt::CaseInsensitive)
	 || message.startsWith(n->user()->nick() + ",", Qt::CaseInsensitive)) {
		payload = message.mid(n->user()->nick().length() + 1).trimmed();
	} else if(message.startsWith(highlightChar, Qt::CaseInsensitive)) {
		payload = message.mid(highlightChar.length()).trimmed();
	}

	if(!payload.isEmpty()) {
		// parse arguments from this string
		bool inQuoteArg = false;
		bool inEscape = false;
		bool hasCommand = false;
		QStringList args;
		QString stringBuilder;
		QString fullArgs;
		// loop through characters
		for(int i = 0; i < payload.length(); ++i) {
			if(hasCommand)
				fullArgs.append(payload.at(i));
			if(inEscape) {
				inEscape = false;
				stringBuilder.append(payload.at(i));
			} else if(payload.at(i) == '\\') {
				inEscape = true;
			} else if(!inQuoteArg && payload.at(i) == ' ') {
				// finish this word
				args.append(stringBuilder);
				stringBuilder.clear();
				hasCommand = true;
			} else if(payload.at(i) == '"') {
				inQuoteArg = !inQuoteArg;
			} else {
				stringBuilder.append(payload.at(i));
			}
		}
		if(!stringBuilder.isEmpty())
			args.append(stringBuilder);

		if(args.length() > 0) {
			const QString &command = args.takeFirst();

			Command *cmd = new Command(*n);
			cmd->origin = origin;
			cmd->channel = buffer->receiver();
			cmd->command = command;
			cmd->fullArgs = fullArgs.trimmed();
			cmd->args = args;
			commandQueue_.append(cmd);
			flushCommandQueue();
		}
	}
	dispatch("PRIVMSG", QStringList() << n->networkName() << origin << buffer->receiver() << message << (n->isIdentified(origin) ? "true" : "false"));
}

void PluginComm::ircEvent(const QString &event, const QString &origin, const QStringList &params, Irc::Buffer *buffer) {
	Network *n = Network::fromBuffer(buffer);
	Q_ASSERT(n != 0);
#define MIN(a) if(params.size() < a) { qWarning() << "Too few parameters for event " << event << ":" << params; return; }
	if(event == "PRIVMSG") {
		MIN(2);
		messageReceived(origin, params[1], buffer);
	} else if(event == "NOTICE") {
		MIN(2);
		dispatch("NOTICE", QStringList() << n->networkName() << origin
		   << buffer->receiver() << params[1]
		   << (n->isIdentified(origin) ? "true" : "false"));
	} else if(event == "MODE" || event == "UMODE") {
		MIN(1);
		dispatch("MODE", QStringList() << n->networkName() << origin << params);
	} else if(event == "NICK") {
		MIN(1);
		dispatch("NICK", QStringList() << n->networkName() << origin << params[0]);
	} else if(event == "JOIN") {
		MIN(1);
		dispatch("JOIN", QStringList() << n->networkName() << origin << buffer->receiver());
	} else if(event == "PART") {
		MIN(1);
		QString message;
		if(params.size() == 2)
			message = params[1];
		dispatch("PART", QStringList() << n->networkName() << origin << buffer->receiver() << message);
	} else if(event == "KICK") {
		MIN(2);
		QString nick = params[1];
		QString message;
		if(params.size() == 3)
			message = params[2];
		dispatch("KICK", QStringList() << n->networkName() << origin << params[0] << nick << message);
	} else if(event == "INVITE") {
		MIN(2);
		QString channel  = params[1];
		dispatch("INVITE", QStringList() << n->networkName() << origin << channel);
	} else if(event == "QUIT") {
		QString message;
		if(params.size() == 1)
			message = params[0];
		dispatch("QUIT", QStringList() << n->networkName() << origin << message);
	} else if(event == "TOPIC") {
		MIN(1);
		QString topic;
		if(params.size() > 1)
			topic = params[1];
		dispatch("TOPIC", QStringList() << n->networkName() << origin << buffer->receiver() << topic);
	} else if(event == "CONNECT") {
		dispatch("CONNECT", QStringList() << n->networkName());
	} else if(event == "DISCONNECT") {
		dispatch("DISCONNECT", QStringList() << n->networkName());
	} else if(event == "CTCP_REQ" || event == "CTCP") {
		MIN(1);
		// TODO: libircclient does not seem to tell us where the ctcp
		// request was sent (user or channel), so just assume it was
		// sent to our nick
		QString to = n->user()->nick();
		dispatch("CTCP", QStringList() << n->networkName() << origin << to << params[0]);
	} else if(event == "CTCP_REP") {
		MIN(1);
		// TODO: see above
		QString to = n->user()->nick();
		dispatch("CTCP_REP", QStringList() << n->networkName() << origin << to << params[0]);
	} else if(event == "CTCP_ACTION" || event == "ACTION") {
		MIN(1);
		QString message;
		if(params.size() >= 2)
			message = params[1];
		dispatch("ACTION", QStringList() << n->networkName() << origin << buffer->receiver() << message);
	} else if(event == "WHOIS") {
		MIN(2);
		dispatch("WHOIS", QStringList() << n->networkName() << origin << params[0] << params[1]);
		flushCommandQueue(params[0], params[1] == "true");
	} else if(event == "NAMES") {
		MIN(2);
		dispatch("NAMES", QStringList() << n->networkName() << origin << params);
	} else if(event == "NUMERIC") {
		MIN(1);
		dispatch("NUMERIC", QStringList() << n->networkName() << origin << buffer->receiver() << params);
	} else {
		qDebug() << "Unknown event: " << n << event << origin << buffer->receiver() << params;
		dispatch("UNKNOWN", QStringList() << n->networkName() << origin << buffer->receiver() << event << params);
	}
#undef MIN
}

void PluginComm::handle(int dev, const QByteArray &line, SocketInfo &info) {
	const QList<Network*> &networks = dazeus_->networks();

	JSONNode n;
	try {
		n = libjson::parse(line.constData());
	} catch(std::invalid_argument &exception) {
		qWarning() << "Got incorrect JSON, ignoring";
		return;
	}
	QStringList params;
	QStringList scope;
	JSONNode::const_iterator i = n.begin();
	QString action;
	while(i != n.end()) {
		if(i->name() == "params") {
			if(i->type() != JSON_ARRAY) {
				qWarning() << "Got params, but not of array type";
				continue;
			}
			JSONNode::const_iterator j = i->begin();
			while(j != i->end()) {
				std::string s = libjson::to_std_string(j->as_string());
				params << QString::fromUtf8(s.c_str(), s.length());
				j++;
			}
		} else if(i->name() == "scope") {
			if(i->type() != JSON_ARRAY) {
				qWarning() << "Got scope, but not of array type";
				continue;
			}
			JSONNode::const_iterator j = i->begin();
			while(j != i->end()) {
				std::string s = libjson::to_std_string(j->as_string());
				scope << QString::fromUtf8(s.c_str(), s.length());
				j++;
			}
		} else if(i->name() == "get") {
			action = QString::fromStdString(libjson::to_std_string(i->as_string()));
		} else if(i->name() == "do") {
			action = QString::fromStdString(libjson::to_std_string(i->as_string()));
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
		foreach(const Network *n, networks) {
			nets.push_back(JSONNode("", libjson::to_json_string(n->networkName().toStdString())));
		}
		response.push_back(nets);
	// REQUESTS ON A NETWORK
	} else if(action == "channels" || action == "whois" || action == "join" || action == "part"
	       || action == "nick") {
		if(action == "channels" || action == "nick") {
			response.push_back(JSONNode("got", libjson::to_json_string(action.toStdString())));
		} else {
			response.push_back(JSONNode("did", libjson::to_json_string(action.toStdString())));
		}
		QString network = "";
		if(params.size() > 0) {
			network = params[0];
		}
		response.push_back(JSONNode("network", libjson::to_json_string(network.toStdString())));
		Network *net = 0;
		foreach(Network *n, networks) {
			if(n->networkName() == network) {
				net = n; break;
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
				const QList<QString> &channels = net->joinedChannels();
				Q_FOREACH(const QString &chan, channels) {
					chans.push_back(JSONNode("", libjson::to_json_string(chan.toStdString())));
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
						Q_ASSERT(false);
						return;
					}
				}
			} else if(action == "nick") {
				response.push_back(JSONNode("success", true));
				response.push_back(JSONNode("nick", libjson::to_json_string(net->user()->nick().toStdString())));
			} else {
				Q_ASSERT(false);
				return;
			}
		}
	// REQUESTS ON A CHANNEL
	} else if(action == "message" || action == "action" || action == "names") {
		response.push_back(JSONNode("did", libjson::to_json_string(action.toStdString())));
		if(params.size() < 2 || (action != "names" && params.size() < 3)) {
			qDebug() << "Wrong parameter size for message, skipping.";
			return;
		}
		QString network = params[0];
		QString receiver = params[1];
		QString message;
		if(action != "names") {
			message = params[2];
		}
		response.push_back(JSONNode("network", libjson::to_json_string(network.toStdString())));
		if(action == "names") {
			response.push_back(JSONNode("channel", libjson::to_json_string(receiver.toStdString())));
		} else {
			response.push_back(JSONNode("receiver", libjson::to_json_string(receiver.toStdString())));
			response.push_back(JSONNode("message", libjson::to_json_string(message.toStdString())));
		}

		bool netfound = false;
		foreach(Network *n, networks) {
			if(n->networkName() == network) {
				netfound = true;
				if(receiver.left(1) != "#" || n->joinedChannels().contains(receiver.toLower())) {
					response.push_back(JSONNode("success", true));
					if(action == "names") {
						n->names(receiver);
					} else if(action == "message") {
						n->say(receiver, message);
					} else {
						n->action(receiver, message);
					}
				} else {
					qWarning() << "Request for communication to network " << network
					           << " receiver " << receiver << ", but not in that channel; dropping";
					response.push_back(JSONNode("success", false));
					response.push_back(JSONNode("error", "Not in that chanel"));
				}
				break;
			}
		}
		if(!netfound) {
			qWarning() << "Request for communication to network " << network << ", but that network isn't joined, dropping";
			response.push_back(JSONNode("success", false));
			response.push_back(JSONNode("error", "Not on that network"));
		}
	// REQUESTS ON DAZEUS ITSELF
	} else if(action == "subscribe") {
		response.push_back(JSONNode("did", "subscribe"));
		response.push_back(JSONNode("success", true));
		int added = 0;
		foreach(const QString &event, params) {
			if(info.subscribe(event))
				++added;
		}
		response.push_back(JSONNode("added", added));
	} else if(action == "unsubscribe") {
		response.push_back(JSONNode("did", "unsubscribe"));
		response.push_back(JSONNode("success", true));
		int removed = 0;
		foreach(const QString &event, params) {
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
			QString commandName = params.takeFirst();
			RequirementInfo *req = 0;
			Network *net = 0;
			if(params.size() > 0) {
				foreach(Network *n, networks) {
					if(n->networkName() == params.at(0)) {
						net = n; break;
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

		QString network, receiver, sender;
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
			QVariant value = database_->property(params[1], network, receiver, sender);
			response.push_back(JSONNode("success", true));
			response.push_back(JSONNode("variable", libjson::to_json_string(params[1].toStdString())));
			if(!value.isNull()) {
				response.push_back(JSONNode("value", libjson::to_json_string(value.toString().toStdString())));
			}
		} else if(params[0] == "set") {
			if(params.size() < 3) {
				response.push_back(JSONNode("success", false));
				response.push_back(JSONNode("error", "Missing parameters"));
			} else {
				database_->setProperty(params[1], params[2], network, receiver, sender);
				response.push_back(JSONNode("success", true));
			}
		} else if(params[0] == "unset") {
			if(params.size() < 2) {
				response.push_back(JSONNode("success", false));
				response.push_back(JSONNode("error", "Missing parameters"));
			} else {
				database_->setProperty(params[1], QVariant(), network, receiver, sender);
				response.push_back(JSONNode("success", true));
			}
		} else if(params[0] == "keys") {
			QStringList qKeys = database_->propertyKeys(params[1], network, receiver, sender);
			JSONNode keys(JSON_ARRAY);
			keys.set_name("keys");
			foreach(const QString &k, qKeys) {
				keys.push_back(JSONNode("", libjson::to_json_string(k.toStdString())));
			}
			response.push_back(keys);
			response.push_back(JSONNode("success", true));
		} else {
			response.push_back(JSONNode("success", false));
			response.push_back(JSONNode("error", "Did not understand request"));
		}
	} else if(action == "config") {
		if(params.size() < 1) {
			response.push_back(JSONNode("success", false));
			response.push_back(JSONNode("error", "Missing parameters"));
		} else {
			QStringList parts = params[0].split('.');
			QString group = parts.takeFirst();
			QString name = parts.join(".");
			// TODO get plugin group name for this plugin
			QVariant conf = config_->groupConfig("plugin " + group).value(name);
			response.push_back(JSONNode("success", true));
			response.push_back(JSONNode("variable", libjson::to_json_string(params[0].toStdString())));
			if(!conf.isNull()) {
				response.push_back(JSONNode("value", libjson::to_json_string(conf.toString().toStdString())));
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
		qWarning() << "Failed to write correct number of JSON bytes to client socket.";
		close(dev);
	}
}
