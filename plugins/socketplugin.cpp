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

void SocketPlugin::welcomed( Network &net ) {
	foreach(QIODevice *i, sockets_.keys()) {
		SocketInfo &info = sockets_[i];
		if(info.isSubscribed("WELCOMED")) {
			info.dispatch(i, "WELCOMED", QStringList() << net.networkName());
		}
	}
}

void SocketPlugin::joined( Network &net, const QString &who, Irc::Buffer *channel ) {
	foreach(QIODevice *i, sockets_.keys()) {
		SocketInfo &info = sockets_[i];
		if(info.isSubscribed("JOINED")) {
			info.dispatch(i, "JOINED", QStringList() << net.networkName() << who << channel->receiver());
		}
	}
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
		}
		if(i->name() == "get") {
			action = QString::fromStdString(libjson::to_std_string(i->as_string()));
		} else if(i->name() == "do") {
			action = QString::fromStdString(libjson::to_std_string(i->as_string()));
		}
		i++;
	}

	JSONNode response(JSON_NODE);
	if(action == "networks") {
		response.push_back(JSONNode("got", "networks"));
		response.push_back(JSONNode("success", true));
		JSONNode nets(JSON_ARRAY);
		nets.set_name("networks");
		foreach(const Network *n, networks) {
			nets.push_back(JSONNode("", libjson::to_json_string(n->networkName().toStdString())));
		}
		response.push_back(nets);
	} else if(action == "channels") {
		response.push_back(JSONNode("got", "channels"));
		QString network = "";
		if(params.size() > 0) {
			network = params[0];
		}
		response.push_back(JSONNode("network", libjson::to_json_string(network.toStdString())));
		const Network *net = 0;
		foreach(const Network *n, networks) {
			if(n->networkName() == network) {
				net = n; break;
			}
		}
		if(net == 0) {
			response.push_back(JSONNode("success", false));
			response.push_back(JSONNode("error", "Not on that network"));
		} else {
			response.push_back(JSONNode("success", true));
			JSONNode chans(JSON_ARRAY);
			const QList<QString> &channels = net->joinedChannels();
			Q_FOREACH(const QString &chan, channels) {
				chans.push_back(JSONNode("", libjson::to_json_string(chan.toStdString())));
			}
			response.push_back(chans);
		}
	} else if(action == "message") {
		response.push_back(JSONNode("do", "message"));
		if(params.size() < 3) {
			qDebug() << "Wrong parameter size for message, skipping.";
			return;
		}
		QString network = params[0];
		QString channel = params[1];
		QString message = params[2];
		response.push_back(JSONNode("network", libjson::to_json_string(network.toStdString())));
		response.push_back(JSONNode("channel", libjson::to_json_string(channel.toStdString())));
		response.push_back(JSONNode("message", libjson::to_json_string(message.toStdString())));

		bool netfound = false;
		foreach(Network *n, networks) {
			if(n->networkName() == network) {
				netfound = true;
				if(n->joinedChannels().contains(channel.toLower())) {
					response.push_back(JSONNode("success", true));
					n->say(channel, message);
				} else {
					qWarning() << "Request for communication to network " << network
					           << " channel " << channel << ", but not in that channel; dropping";
					response.push_back(JSONNode("success", false));
					response.push_back(JSONNode("error", "Not in that chanel"));
				}
				break;
			}
		}
		qWarning() << "Request for communication to network " << network << ", but that network isn't joined, dropping";
		response.push_back(JSONNode("success", false));
		response.push_back(JSONNode("error", "Not on that network"));
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
