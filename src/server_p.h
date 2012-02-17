/**
 * Copyright (c) Sjors Gielen, 2011
 * See LICENSE for license.
 */

#ifndef SERVER_P_H
#define SERVER_P_H

#include <QtCore/QThread>
#include <QtCore/QStringList>
#include <QtNetwork/QHostAddress>
#include <assert.h>
#include <iostream>

#include <libircclient.h>
#include <cassert>
#include "server.h"
#include "config.h"

class ServerThread : public QThread
{
Q_OBJECT

public:
	ServerThread(Server *s) : QThread(), s_(s), config_(0), irc_(0), whois_identified_(false) {}
	~ServerThread() { irc_destroy_session(irc_); }

	Server *server() {
		return s_;
	}
	void setConfig(const ServerConfig *c) {
		config_ = c;
	}
	irc_session_t *getIrc() const {
		return irc_;
	}

	void run();
	void quit() { QThread::quit(); }

	void quit( const QString &reason ) {
		irc_cmd_quit(irc_, reason.toUtf8());
	}

	void whois( const QString &destination ) {
		irc_cmd_whois(irc_, destination.toUtf8());
	}

	void ctcpAction( const QString &destination, const QString &message ) {
		irc_cmd_me(irc_, destination.toUtf8(), message.toUtf8());
	}

	void ctcpRequest( const QString &destination, const QString &message ) {
		irc_cmd_ctcp_request(irc_, destination.toUtf8(), message.toUtf8());
	}

	void join( const QString &channel, const QString &key = QString() ) {
		irc_cmd_join(irc_, channel.toUtf8(), key.toUtf8());
	}

	void part( const QString &channel, const QString &reason = QString() ) {
		irc_cmd_part(irc_, channel.toUtf8());
	}

	void message( const QString &destination, const QString &message ) {
		QStringList lines = message.split(QRegExp(QLatin1String("[\\r\\n]+")), QString::SkipEmptyParts);
		foreach(const QString& line, lines)
			irc_cmd_msg(irc_, destination.toUtf8(), line.toUtf8());
	}

	void names( const QString &channel ) {
		irc_cmd_names(irc_, channel.toUtf8());
	}

	Server *s_;
	const ServerConfig *config_;
	irc_session_t *irc_;
	QString in_whois_for_;
	bool whois_identified_;
	QStringList in_names_;

signals:
	void disconnected();
	void connectionTimeout();

	void motdReceived( const QString &motd, Irc::Buffer *buffer );
	void invited( const QString &origin, const QString &receiver, const QString &channel, Irc::Buffer *buffer );
	void kicked( const QString &origin, const QString &nick, const QString &message, Irc::Buffer *buffer );
	void ctcpRequestReceived( const QString &origin, const QString &request, Irc::Buffer *buffer );
	void ctcpReplyReceived( const QString &origin, const QString &reply, Irc::Buffer *buffer );
	void ctcpActionReceived( const QString &origin, const QString &action, Irc::Buffer *buffer );
	void numericMessageReceived( const QString &origin, uint code, const QStringList &params, Irc::Buffer *buffer );
	void unknownMessageReceived( const QString &origin, const QStringList &params, Irc::Buffer *buffer );
	void ircEvent(const QString &event, const QString &origin, const QStringList &params, Irc::Buffer *buffer );
	void whoisReceivedHiPrio(const QString &origin, const QString &nick, bool identified, Irc::Buffer *buffer );
	void whoisReceived(const QString &origin, const QString &nick, bool identified, Irc::Buffer *buffer );
	void namesReceivedHiPrio(const QString &origin, const QString &channel, const QStringList &names, Irc::Buffer *buffer );
	void namesReceived(const QString &origin, const QString &channel, const QStringList &names, Irc::Buffer *buffer );

public:
	void slotMotdReceived( const QString &motd, Irc::Buffer *b );
	void slotJoined( const QString &origin, Irc::Buffer *b );
	void slotParted( const QString &origin, const QString &message, Irc::Buffer *b );
	void slotInvited( const QString &origin, const QString &receiver, const QString &channel, Irc::Buffer *b );
	void slotKicked( const QString &origin, const QString &nick, const QString &message, Irc::Buffer *b );
	void slotCtcpRequestReceived( const QString &origin, const QString &request, Irc::Buffer *b );
	void slotCtcpReplyReceived( const QString &origin, const QString &reply, Irc::Buffer *b );
	void slotCtcpActionReceived( const QString &origin, const QString &action, Irc::Buffer *b );
	void slotNumericMessageReceived( const QString &origin, uint code, const QStringList &params, Irc::Buffer *b );
	void slotUnknownMessageReceived( const QString &origin, const QStringList &params, Irc::Buffer *b );
	void slotConnected();
	void slotDisconnected() {
		emit disconnected();
	}
	void slotWelcomed();
	void slotIrcEvent(const QString &event, const QString &origin, const QStringList &params, Irc::Buffer *b);
};

#endif
