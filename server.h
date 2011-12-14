/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#ifndef SERVER_H
#define SERVER_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include "user.h"
#include "network.h"

#include <IrcSession>

// #define SERVER_FULLDEBUG

class QTimer;
struct ServerConfig;

class Server;

QDebug operator<<(QDebug, const Server &);
QDebug operator<<(QDebug, const Server *);

class Server : public Irc::Session
{
Q_OBJECT

friend class TestServer;

public:
	   Server();
	  ~Server();

	static Server *fromServerConfig( const ServerConfig *c );
	const ServerConfig *config() const;
	QString motd() const;

public slots:
	void connectToServer();
	void disconnectFromServer( Network::DisconnectReason );
	inline void quit( const QString &reason ) { Irc::Session::quit( reason ); }
	Q_DECL_DEPRECATED void joinChannel( QString channel, const QString &key = QString() );
	Q_DECL_DEPRECATED void leaveChannel( QString channel, const QString &reason = QString() );
	Q_DECL_DEPRECATED void say( QString destination, QString message );
	Q_DECL_DEPRECATED void action( QString destination, QString message );
	Q_DECL_DEPRECATED void ctcp( QString destination, QString message );

signals:
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


private slots:
	void slotBufferAdded( Irc::Buffer *buffer );
	void slotBufferRemoved( Irc::Buffer *buffer );

	void slotMotdReceived( const QString &motd );
	void slotJoined( const QString &origin );
	void slotParted( const QString &origin, const QString &message );
	void slotQuit(   const QString &origin, const QString &message );
	void slotNickChanged( const QString &origin, const QString &nick );
	void slotModeChanged( const QString &origin, const QString &mode, const QString &args );
	void slotTopicChanged( const QString &origin, const QString &topic );
	void slotInvited( const QString &origin, const QString &receiver, const QString &channel );
	void slotKicked( const QString &origin, const QString &nick, const QString &message );
	void slotMessageReceived( const QString &origin, const QString &message );
	void slotNoticeReceived( const QString &origin, const QString &notice );
	void slotCtcpRequestReceived( const QString &origin, const QString &request );
	void slotCtcpReplyReceived( const QString &origin, const QString &reply );
	void slotCtcpActionReceived( const QString &origin, const QString &action );
	void slotNumericMessageReceived( const QString &origin, uint code, const QStringList &params );
	void slotUnknownMessageReceived( const QString &origin, const QStringList &params );

private:
	const ServerConfig *config_;
	QString   motd_;
	QTimer   *connectTimer_;
};

#endif
