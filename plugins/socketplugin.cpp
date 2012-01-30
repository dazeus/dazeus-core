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

#include "socketplugin.h"
#include "pluginmanager.h"
#include "davinci.h"

SocketPlugin::SocketPlugin( PluginManager *man )
: Plugin( "SocketPlugin", man )
{}

SocketPlugin::~SocketPlugin() {
	qDeleteAll(sockets_.keys());
	qDeleteAll(tcpServers_);
}

void SocketPlugin::init() {
	int sockets = getConfig("sockets").toInt();
	for(int i = 1; i <= sockets; ++i) {
		QString socket = getConfig("socket" + QString::number(i)).toString();
		int colon = socket.indexOf(':');
		if(colon == -1) {
			qWarning() << "(SocketPlugin) Did not understand socket option " << i << ":" << socket;
			continue;
		}
		QString socketType = socket.left(colon);
		socket = socket.mid(colon + 1);
		if(socketType == "unix") {
			QFile::remove(socket);
			QLocalServer *server = new QLocalServer();
			if(!server->listen(socket)) {
				qWarning() << "(SocketPlugin) Failed to start listening for local connections on "
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
				qWarning() << "(SocketPlugin) Invalid TCP port number for socket " << i;
				continue;
			}
			QHostAddress host(tcpHost);
			if(tcpHost == "*") {
				host = QHostAddress::Any;
			} else if(host.isNull()) {
				QHostInfo info = QHostInfo::fromName(tcpHost);
				if(info.error() != QHostInfo::NoError || info.addresses().length() == 0) {
					qWarning() << "(SocketPlugin) Couldn't resolve TCP host for socket "
					           << i << ": " << info.errorString();
					continue;
				}
				host = info.addresses().at(0);
			}
			QTcpServer *server = new QTcpServer();
			if(!server->listen(host, port)) {
				qWarning() << "(SocketPlugin) Failed to start listening for TCP connections: " << server->errorString();
				continue;
			}
			connect(server, SIGNAL(newConnection()),
			        this,   SLOT(newTcpConnection()));
			tcpServers_.append(server);
		} else {
			qWarning() << "(SocketPlugin) Skipping socket " << i << ": unknown type " << socketType;
		}
	}
}

void SocketPlugin::newTcpConnection() {
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

void SocketPlugin::newLocalConnection() {
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

void SocketPlugin::poll() {
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

void SocketPlugin::dispatch(const QString &event, const QStringList &parameters) {
	foreach(QIODevice *i, sockets_.keys()) {
		SocketInfo &info = sockets_[i];
		if(info.isSubscribed(event)) {
			info.dispatch(i, event, parameters);
		}
	}
}

void SocketPlugin::welcomed( Network &net ) {
	dispatch("WELCOMED", QStringList() << net.networkName());
}

void SocketPlugin::connected( Network &net, const Server & ) {
	dispatch("CONNECTED", QStringList() << net.networkName());
}

void SocketPlugin::disconnected( Network &net ) {
	dispatch("DISCONNECTED", QStringList() << net.networkName());
}

void SocketPlugin::joined( Network &net, const QString &who, Irc::Buffer *channel ) {
	dispatch("JOINED", QStringList() << net.networkName() << who << channel->receiver());
}

void SocketPlugin::parted( Network &net, const QString &who, const QString &leaveMessage,
                         Irc::Buffer *channel ) {
	dispatch("PARTED", QStringList() << net.networkName() << who << channel->receiver() << leaveMessage);
}

void SocketPlugin::motdReceived( Network &net, const QString &motd, Irc::Buffer *buffer ) {
	dispatch("MOTD", QStringList() << net.networkName() << motd << buffer->receiver());
}

void SocketPlugin::quit(   Network &net, const QString &origin, const QString &message,
                     Irc::Buffer *buffer ) {
	dispatch("QUIT", QStringList() << net.networkName() << origin << buffer->receiver() << message);
}

void SocketPlugin::nickChanged( Network &net, const QString &origin, const QString &nick,
                          Irc::Buffer *buffer ) {
	dispatch("NICK", QStringList() << net.networkName() << origin << buffer->receiver() << nick);
}

void SocketPlugin::modeChanged( Network &net, const QString &origin, const QString &mode,
                          const QString &args, Irc::Buffer *buffer ) {
	dispatch("MODE", QStringList() << net.networkName() << origin << buffer->receiver() << mode << args);
}

void SocketPlugin::topicChanged( Network &net, const QString &origin, const QString &topic,
                           Irc::Buffer *buffer ) {
	dispatch("TOPIC", QStringList() << net.networkName() << origin << buffer->receiver() << topic);
}

void SocketPlugin::invited( Network &net, const QString &origin, const QString &receiver,
                      const QString &channel, Irc::Buffer *buffer ) {
	dispatch("INVITE", QStringList() << net.networkName() << origin << buffer->receiver() << receiver << channel);
}

void SocketPlugin::kicked( Network &net, const QString &origin, const QString &nick,
                     const QString &message, Irc::Buffer *buffer ) {
	dispatch("KICK", QStringList() << net.networkName() << origin << buffer->receiver() << nick << message);
}

void SocketPlugin::messageReceived( Network &net, const QString &origin, const QString &message,
                              Irc::Buffer *buffer ) {
	dispatch("MESSAGE", QStringList() << net.networkName() << origin << buffer->receiver() << message);
}

void SocketPlugin::noticeReceived( Network &net, const QString &origin, const QString &notice,
                             Irc::Buffer *buffer ) {
	dispatch("NOTICE", QStringList() << net.networkName() << origin << buffer->receiver() << notice);
}

void SocketPlugin::ctcpRequestReceived(Network &net, const QString &origin, const QString &request,
                                 Irc::Buffer *buffer ) {
	dispatch("CTCPREQ", QStringList() << net.networkName() << origin << buffer->receiver() << request);
}

void SocketPlugin::ctcpReplyReceived( Network &net, const QString &origin, const QString &reply,
                                Irc::Buffer *buffer ) {
	dispatch("CTCPREPL", QStringList() << net.networkName() << origin << buffer->receiver() << reply);
}

void SocketPlugin::ctcpActionReceived( Network &net, const QString &origin, const QString &action,
                                 Irc::Buffer *buffer ) {
	dispatch("ACTION", QStringList() << net.networkName() << origin << buffer->receiver() << action);
}

void SocketPlugin::numericMessageReceived( Network &net, const QString &origin, uint code,
                                     const QStringList &params,
                                     Irc::Buffer *buffer ) {
	dispatch("NUMERIC", QStringList() << net.networkName() << origin << buffer->receiver() << QString::number(code) << params);
}

void SocketPlugin::unknownMessageReceived( Network &net, const QString &origin,
                                       const QStringList &params,
                                       Irc::Buffer *buffer ) {
	dispatch("UNKNOWN", QStringList() << net.networkName() << origin << buffer->receiver() << params);
}

void SocketPlugin::whoisReceived( Network &net, const QString &origin, const QString &nick,
                                 bool identified, Irc::Buffer *buffer ) {
	dispatch("WHOIS", QStringList() << net.networkName() << origin << nick << (identified ? "true" : "false"));
}
void SocketPlugin::namesReceived( Network &net, const QString &origin, const QString &channel,
                                 const QStringList &params, Irc::Buffer *buffer ) {
	dispatch("NAMES", QStringList() << net.networkName() << origin << channel << params);
}

void SocketPlugin::handle(QIODevice *dev, const QByteArray &line, SocketInfo &info) {
	if(!dev->isOpen()) return;
	const QList<Network*> &networks = manager()->bot()->networks();

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
	} else if(action == "property") {
		response.push_back(JSONNode("did", "property"));

		VariableScope vScope;
		if(scope.size() == 0) {
			vScope = GlobalScope;
			setContext(QString());
		} else if(scope.size() == 1) {
			vScope = NetworkScope;
			setContext(scope[0]);
		} else if(scope.size() == 2) {
			vScope = ReceiverScope;
			setContext(scope[0], scope[1]);
		} else {
			vScope = SenderScope;
			setContext(scope[0], scope[1], scope[2]);
		}

		if(params.size() < 2) {
			response.push_back(JSONNode("success", false));
			response.push_back(JSONNode("error", "Missing parameters"));
		} else if(params[0] == "get") {
			QVariant value = get(params[1]);
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
				set(vScope, params[1], params[2]);
				response.push_back(JSONNode("success", true));
			}
		} else if(params[0] == "unset") {
			if(params.size() < 2) {
				response.push_back(JSONNode("success", false));
				response.push_back(JSONNode("error", "Missing parameters"));
			} else {
				set(vScope, params[1], QVariant());
				response.push_back(JSONNode("success", true));
			}
		} else if(params[0] == "keys") {
			QStringList qKeys = keys(params[1]);
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
		clearContext();
	} else if(action == "config") {
		if(params.size() < 1) {
			response.push_back(JSONNode("success", false));
			response.push_back(JSONNode("error", "Missing parameters"));
		} else {
			QStringList parts = params[0].split('.');
			QString group = parts.takeFirst();
			QString name = parts.join(".");
			QVariant conf = getConfig(name, group);
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
