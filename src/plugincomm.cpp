/**
 * Copyright (c) Sjors Gielen, 2011
 * See LICENSE for license.
 */

#include <QtCore/QList>
#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QHostInfo>
#include <QtNetwork/QLocalServer>
#include <QtNetwork/QLocalSocket>
#define __STDC_LIMIT_MACROS
#include <stdint.h>
#include <libjson.h>

#include "plugincomm.h"
#include "network.h"
#include "config.h"
#include "server.h"
#include "config.h"
#include "dazeus.h"
#include "database.h"

PluginComm::PluginComm(Database *d, Config *c, DaZeus *bot)
: QObject()
, database_(d)
, config_(c)
, dazeus_(bot)
{}

PluginComm::~PluginComm() {
	qDeleteAll(sockets_.keys());
	qDeleteAll(tcpServers_);
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
			QLocalServer *server = new QLocalServer();
			if(!server->listen(socket)) {
				qWarning() << "(PluginComm) Failed to start listening for local connections on "
				           << socket << ": " << server->errorString();
				continue;
			}
			connect(server, SIGNAL(newConnection()),
			        this,   SLOT(newLocalConnection()));
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
			QHostAddress host(tcpHost);
			if(tcpHost == "*") {
				host = QHostAddress::Any;
			} else if(host.isNull()) {
				QHostInfo info = QHostInfo::fromName(tcpHost);
				if(info.error() != QHostInfo::NoError || info.addresses().length() == 0) {
					qWarning() << "(PluginComm) Couldn't resolve TCP host for socket "
					           << i << ": " << info.errorString();
					continue;
				}
				host = info.addresses().at(0);
			}
			QTcpServer *server = new QTcpServer();
			if(!server->listen(host, port)) {
				qWarning() << "(PluginComm) Failed to start listening for TCP connections: " << server->errorString();
				continue;
			}
			connect(server, SIGNAL(newConnection()),
			        this,   SLOT(newTcpConnection()));
			tcpServers_.append(server);
		} else {
			qWarning() << "(PluginComm) Skipping socket " << i << ": unknown type " << socketType;
		}
	}
}

void PluginComm::newTcpConnection() {
	for(int i = 0; i < tcpServers_.length(); ++i) {
		QTcpServer *s = tcpServers_[i];
		if(!s->isListening()) {
			qWarning() << "Something is wrong: TCP server is not listening anymore\n";
			qWarning() << s->serverAddress() << s->serverPort() << s->errorString();
			continue;
		}
		while(s->hasPendingConnections()) {
			QTcpSocket *sock = s->nextPendingConnection();
			sock->setParent(this);
			connect(sock, SIGNAL(readyRead()), this, SLOT(poll()));
			sockets_.insert(sock, SocketInfo("tcp"));
		}
	}
}

void PluginComm::newLocalConnection() {
	for(int i = 0; i < localServers_.length(); ++i) {
		QLocalServer *s = localServers_[i];
		if(!s->isListening()) {
			qWarning() << "Something is wrong: local server is not listening anymore\n";
			qWarning() << s->serverName() << s->errorString();
			continue;
		}
		while(s->hasPendingConnections()) {
			QLocalSocket *sock = s->nextPendingConnection();
			sock->setParent(this);
			connect(sock, SIGNAL(readyRead()), this, SLOT(poll()));
			sockets_.insert(sock, SocketInfo("unix"));
		}
	}
}

void PluginComm::poll() {
	QList<QIODevice*> toRemove_;
	QList<QIODevice*> socks = sockets_.keys();
	for(int i = 0; i < socks.size(); ++i) {
		QIODevice *dev = socks[i];
		SocketInfo info = sockets_[dev];
		if(!dev->isOpen()) {
			toRemove_.append(dev);
			continue;
		}
		while(dev->isOpen() && dev->bytesAvailable()) {
			if(info.waitingSize == 0) {
				// Read the byte size, everything until the start of the JSON message
				char size[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
				dev->peek(size, sizeof(size));
				for(unsigned i = 0; i < sizeof(size); ++i) {
					if(size[i] == '{') {
						// Start of the JSON character, we can start reading the full response
						dev->read(i);
						for(unsigned j = 0; j < i; ++j) {
							// Ignore any other character than numbers (i.e. newlines)
							if(size[j] >= 0x30 && size[j] <= 0x39) {
								info.waitingSize *= 10;
								info.waitingSize += size[j] - 0x30;
							}
						}
					}
				}
				Q_ASSERT(info.waitingSize >= 0);
				if(info.waitingSize == 0)
					break;
				continue;
			} else if(info.waitingSize > dev->bytesAvailable()) {
				break;
			}
			QByteArray json = dev->read(info.waitingSize);
			if(json.length() != info.waitingSize) {
				qDebug() << "Failed to read from socket; breaking";
			}
			handle(dev, json, info);
			info.waitingSize = 0;
		}
		sockets_[dev] = info;
	}
	foreach(QIODevice* i, toRemove_) {
		sockets_.remove(i);
	}
}

void PluginComm::dispatch(const QString &event, const QStringList &parameters) {
	foreach(QIODevice *i, sockets_.keys()) {
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
		foreach(QIODevice *i, sockets_.keys()) {
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

		foreach(QIODevice *i, sockets_.keys()) {
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

void PluginComm::welcomed( Network &net ) {
	dispatch("WELCOMED", QStringList() << net.networkName());
}

void PluginComm::connected( Network &net, const Server & ) {
	dispatch("CONNECTED", QStringList() << net.networkName());
}

void PluginComm::disconnected( Network &net ) {
	dispatch("DISCONNECTED", QStringList() << net.networkName());
}

void PluginComm::joined( const QString &who, Irc::Buffer *channel ) {
	Network *n = Network::fromBuffer(channel);
	Q_ASSERT(n != 0);
	dispatch("JOINED", QStringList() << n->networkName() << who << channel->receiver());
}

void PluginComm::parted( const QString &who, const QString &leaveMessage,
                         Irc::Buffer *channel ) {
	Network *n = Network::fromBuffer(channel);
	Q_ASSERT(n != 0);
	dispatch("PARTED", QStringList() << n->networkName() << who << channel->receiver() << leaveMessage);
}

void PluginComm::motdReceived( const QString &motd, Irc::Buffer *buffer ) {
	Network *n = Network::fromBuffer(buffer);
	Q_ASSERT(n != 0);
	dispatch("MOTD", QStringList() << n->networkName() << motd << buffer->receiver());
}

void PluginComm::quit(   const QString &origin, const QString &message,
                     Irc::Buffer *buffer ) {
	Network *n = Network::fromBuffer(buffer);
	Q_ASSERT(n != 0);
	dispatch("QUIT", QStringList() << n->networkName() << origin << buffer->receiver() << message);
}

void PluginComm::nickChanged( const QString &origin, const QString &nick,
                          Irc::Buffer *buffer ) {
	Network *n = Network::fromBuffer(buffer);
	Q_ASSERT(n != 0);
	dispatch("NICK", QStringList() << n->networkName() << origin << nick);
}

void PluginComm::modeChanged( const QString &origin, const QString &mode,
                          const QString &args, Irc::Buffer *buffer ) {
	Network *n = Network::fromBuffer(buffer);
	Q_ASSERT(n != 0);
	dispatch("MODE", QStringList() << n->networkName() << origin << buffer->receiver() << mode << args);
}

void PluginComm::topicChanged( const QString &origin, const QString &topic,
                           Irc::Buffer *buffer ) {
	Network *n = Network::fromBuffer(buffer);
	Q_ASSERT(n != 0);
	dispatch("TOPIC", QStringList() << n->networkName() << origin << buffer->receiver() << topic);
}

void PluginComm::invited( const QString &origin, const QString &receiver,
                      const QString &channel, Irc::Buffer *buffer ) {
	Network *n = Network::fromBuffer(buffer);
	Q_ASSERT(n != 0);
	dispatch("INVITE", QStringList() << n->networkName() << origin << buffer->receiver() << receiver << channel);
}

void PluginComm::kicked( const QString &origin, const QString &nick,
                     const QString &message, Irc::Buffer *buffer ) {
	Network *n = Network::fromBuffer(buffer);
	Q_ASSERT(n != 0);
	dispatch("KICK", QStringList() << n->networkName() << origin << buffer->receiver() << nick << message);
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

void PluginComm::noticeReceived( const QString &origin, const QString &notice,
                             Irc::Buffer *buffer ) {
	Network *n = Network::fromBuffer(buffer);
	Q_ASSERT(n != 0);
	dispatch("NOTICE", QStringList() << n->networkName() << origin << buffer->receiver() << notice << (n->isIdentified(origin) ? "true" : "false"));
}

void PluginComm::ctcpRequestReceived(const QString &origin, const QString &request,
                                 Irc::Buffer *buffer ) {
	Network *n = Network::fromBuffer(buffer);
	Q_ASSERT(n != 0);
	dispatch("CTCPREQ", QStringList() << n->networkName() << origin << buffer->receiver() << request);
}

void PluginComm::ctcpReplyReceived( const QString &origin, const QString &reply,
                                Irc::Buffer *buffer ) {
	Network *n = Network::fromBuffer(buffer);
	Q_ASSERT(n != 0);
	dispatch("CTCPREPL", QStringList() << n->networkName() << origin << buffer->receiver() << reply);
}

void PluginComm::ctcpActionReceived( const QString &origin, const QString &action,
                                 Irc::Buffer *buffer ) {
	Network *n = Network::fromBuffer(buffer);
	Q_ASSERT(n != 0);
	dispatch("ACTION", QStringList() << n->networkName() << origin << buffer->receiver() << action);
}

void PluginComm::numericMessageReceived( const QString &origin, uint code,
                                     const QStringList &params,
                                     Irc::Buffer *buffer ) {
	Network *n = Network::fromBuffer(buffer);
	Q_ASSERT(n != 0);
	dispatch("NUMERIC", QStringList() << n->networkName() << origin << buffer->receiver() << QString::number(code) << params);
}

void PluginComm::unknownMessageReceived( const QString &origin,
                                       const QStringList &params,
                                       Irc::Buffer *buffer ) {
	Network *n = Network::fromBuffer(buffer);
	Q_ASSERT(n != 0);
	dispatch("UNKNOWN", QStringList() << n->networkName() << origin << buffer->receiver() << params);
}

void PluginComm::ircEvent(const QString &event, const QString &origin, const QStringList &params, Irc::Buffer *buffer) {
	Network *n = Network::fromBuffer(buffer);
	Q_ASSERT(n != 0);
	if(event == "PRIVMSG") {
		messageReceived(origin, params[1], buffer);
	} else {
		qDebug() << n << event << origin << buffer->receiver() << params;
	}
}

void PluginComm::whoisReceived( const QString &origin, const QString &nick,
                                 bool identified, Irc::Buffer *buffer ) {
	Network *n = Network::fromBuffer(buffer);
	Q_ASSERT(n != 0);
	dispatch("WHOIS", QStringList() << n->networkName() << origin << nick << (identified ? "true" : "false"));
	flushCommandQueue(nick, identified);
}
void PluginComm::namesReceived( const QString &origin, const QString &channel,
                                 const QStringList &params, Irc::Buffer *buffer ) {
	Network *n = Network::fromBuffer(buffer);
	Q_ASSERT(n != 0);
	dispatch("NAMES", QStringList() << n->networkName() << origin << channel << params);
}

void PluginComm::handle(QIODevice *dev, const QByteArray &line, SocketInfo &info) {
	if(!dev->isOpen()) return;
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
	dev->write(mstr.str().c_str(), mstr.str().length());
}
