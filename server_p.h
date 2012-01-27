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
	ServerThread(Server *s) : QThread(), s_(s), config_(0), irc_(0) {}
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

	Server *s_;
	const ServerConfig *config_;
	irc_session_t *irc_;

signals:
	void connected();
	void disconnected();
	void welcomed();
	void connectionTimeout();

	void motdReceived( const QString &motd, Irc::Buffer *buffer );
	void joined( const QString &origin, Irc::Buffer *buffer );
	void parted( const QString &origin, const QString &message, Irc::Buffer *buffer );
	void quit(   const QString &origin, const QString &message, Irc::Buffer *buffer );
	void nickChanged( const QString &origin, const QString &nick, Irc::Buffer *buffer );
	void modeChanged( const QString &origin, const QString &mode, const QString &args, Irc::Buffer *buffer );
	void topicChanged( const QString &origin, const QString &topic, Irc::Buffer *buffer );
	void invited( const QString &origin, const QString &receiver, const QString &channel, Irc::Buffer *buffer );
	void kicked( const QString &origin, const QString &nick, const QString &message, Irc::Buffer *buffer );
	void messageReceived( const QString &origin, const QString &message, Irc::Buffer *buffer );
	void noticeReceived( const QString &origin, const QString &notice, Irc::Buffer *buffer );
	void ctcpRequestReceived( const QString &origin, const QString &request, Irc::Buffer *buffer );
	void ctcpReplyReceived( const QString &origin, const QString &reply, Irc::Buffer *buffer );
	void ctcpActionReceived( const QString &origin, const QString &action, Irc::Buffer *buffer );
	void numericMessageReceived( const QString &origin, uint code, const QStringList &params, Irc::Buffer *buffer );
	void unknownMessageReceived( const QString &origin, const QStringList &params, Irc::Buffer *buffer );

public:
	void slotMotdReceived( const QString &motd, Irc::Buffer *b );
	void slotJoined( const QString &origin, Irc::Buffer *b );
	void slotParted( const QString &origin, const QString &message, Irc::Buffer *b );
	void slotQuit(   const QString &origin, const QString &message, Irc::Buffer *b );
	void slotNickChanged( const QString &origin, const QString &nick, Irc::Buffer *b );
	void slotModeChanged( const QString &origin, const QString &mode, const QString &args, Irc::Buffer *b );
	void slotTopicChanged( const QString &origin, const QString &topic, Irc::Buffer *b );
	void slotInvited( const QString &origin, const QString &receiver, const QString &channel, Irc::Buffer *b );
	void slotKicked( const QString &origin, const QString &nick, const QString &message, Irc::Buffer *b );
	void slotMessageReceived( const QString &origin, const QString &message, Irc::Buffer *b );
	void slotNoticeReceived( const QString &origin, const QString &notice, Irc::Buffer *b );
	void slotCtcpRequestReceived( const QString &origin, const QString &request, Irc::Buffer *b );
	void slotCtcpReplyReceived( const QString &origin, const QString &reply, Irc::Buffer *b );
	void slotCtcpActionReceived( const QString &origin, const QString &action, Irc::Buffer *b );
	void slotNumericMessageReceived( const QString &origin, uint code, const QStringList &params, Irc::Buffer *b );
	void slotUnknownMessageReceived( const QString &origin, const QStringList &params, Irc::Buffer *b );
	void slotConnected();
	void slotWelcomed();
};

#endif
