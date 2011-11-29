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

#include "socketplugin.h"
#include "pluginmanager.h"
#include "davinci.h"

SocketPlugin::SocketPlugin( PluginManager *man )
: Plugin( "SocketPlugin", man )
{}

SocketPlugin::~SocketPlugin() {
	qDeleteAll(sockets_);
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
			sockets_.append(sock);
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
			sockets_.append(sock);
		}
	}
}

void SocketPlugin::poll() {
	QList<int> toRemove_;
	for(int i = 0; i < sockets_.length(); ++i) {
		QIODevice *dev = sockets_[i];
		if(!dev->isOpen()) {
			toRemove_.append(i);
			continue;
		}
		while(dev->isOpen() && dev->canReadLine()) {
			QByteArray line = dev->readLine().trimmed();
			handle(dev, line);
		}
	}
	foreach(int i, toRemove_) {
		sockets_.removeAt(i);
	}
}

void SocketPlugin::handle(QIODevice *dev, const QByteArray &line) {
	if(!dev->isOpen()) return;
	const QList<Network*> &networks = manager()->bot()->networks();
	if(line.startsWith("?networks")) {
		QString response;
		foreach(const Network *n, networks) {
			response += n->networkName() + " ";
		}
		response = response.left(response.length() - 1);
		dev->write(QString(response + "\n").toLatin1());
	} else if(line.startsWith("?channels ")) {
		QString network = line.mid(10);
		const Network *net = 0;
		foreach(const Network *n, networks) {
			if(n->networkName() == network) {
				net = n; break;
			}
		}
		if(net == 0) {
			dev->write(QString("NAK '" + network + "'\n").toLatin1());
			return;
		} else {
			// TODO FIXME
			dev->write("TODO\n");
		}
	} else if(line.startsWith("!msg ")) {
		int space1 = line.indexOf(' ', 5);
		int space2 = line.indexOf(' ', space1 + 1);
		if(space1 == -1 || space2 == -1) {
			dev->write("Usage: !msg <net> <chan> <len>\n");
			return;
		}
		QString network = line.mid(5, space1 - 5);
		QString channel = line.mid(space1 + 1, space2 - space1 - 1);
		QString len     = line.mid(space2 + 1);

		unsigned int length = len.toUInt();
		if(length == 0 || length > 512) {
			qWarning() << "Invalid length " << length << " in SocketPlugin";
			dev->write("NAK, invalid length\n");
			return;
		}

		Network *net = 0;
		foreach(Network *n, networks) {
			if(n->networkName() == network) {
				net = n; break;
			}
		}
		if(net == 0) {
			dev->write(QString("NAK '" + network + "'\n").toLatin1());
			return;
		} else {
			// TODO: check if channel is joined
			QByteArray message;
			int tries = 5;
			while((unsigned int)message.length() < length && tries >= 0) {
				dev->waitForReadyRead(100);
				message.append(dev->read(length - message.length()));
				tries--;
			}
			if((unsigned int)message.length() == length) {
				net->say(channel, message);
				dev->write("ACK\n");
			} else {
				qWarning() << "Didn't write data: reading took too long.";
				qWarning() << "Wanted " << length << "bytes, got " << message.length();
				dev->close(); // unread data still coming, ignore it
				return;
			}
		}
	} else {
		dev->write("This is DaZeus 2.0 SocketPlugin. Usage:\n");
		dev->write("?networks, ?channels <net>, !msg <net> <chan> <len>\n");
	}
}
