/**
 * Copyright (c) Sjors Gielen, 2010-2012
 * See LICENSE for license.
 */

#ifndef SERVER_H
#define SERVER_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <libircclient.h>
#include <cassert>
#include <iostream>

#include "user.h"
#include "network.h"
#include "server.h"
#include "config.h"

// #define SERVER_FULLDEBUG

class QTimer;
struct ServerConfig;

class ServerThread;
class Server;

QDebug operator<<(QDebug, const Server &);
QDebug operator<<(QDebug, const Server *);
QDebug operator<<(QDebug, const Irc::Buffer *);

namespace Irc {
	struct Buffer {
		// TEMPORARY PLACEHOLDER
		Server *session_;
		QString receiver_;
		inline Server *session() const { return session_; }
		inline void setReceiver(const QString &p) { receiver_ = p; }
		inline const QString &receiver() const { return receiver_; }

		Buffer(Server *s) : session_(s) {}

		void message(const QString &message);
	};
};

class Server : public QObject
{
Q_OBJECT

friend class TestServer;
friend class ServerThread;

private:
	   Server();

public:
	  ~Server();
	static Server *fromServerConfig( const ServerConfig *c, Network *n );
	const ServerConfig *config() const;
	QString motd() const;
	void addDescriptors(fd_set *in_set, fd_set *out_set, int *maxfd);
	void processDescriptors(fd_set *in_set, fd_set *out_set);
	irc_session_t *getIrc() const;

private:
	void run();

public slots:
	void connectToServer();
	void disconnectFromServer( Network::DisconnectReason );
	void quit( const QString &reason );
	void whois( const QString &destination );
	void ctcpAction( const QString &destination, const QString &message );
	void ctcpRequest( const QString &destination, const QString &message );
	void join( const QString &channel, const QString &key = QString() );
	void part( const QString &channel, const QString &reason = QString() );
	void message( const QString &destination, const QString &message );
	void names( const QString &channel );
	void slotNumericMessageReceived( const QString &origin, uint code, const QStringList &params, Irc::Buffer *b );
	void slotIrcEvent(const QString &event, const QString &origin, const QStringList &params, Irc::Buffer *b);
	void slotDisconnected();

private:
	const ServerConfig *config_;
	QString   motd_;
	QTimer   *connectTimer_;
	Network  *network_;
	irc_session_t *irc_;
	QString in_whois_for_;
	bool whois_identified_;
	QStringList in_names_;
};

#endif
