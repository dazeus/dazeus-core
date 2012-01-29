/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#include <QtCore/QStringList>
#include <QtCore/QTimer>
#include <QtNetwork/QHostAddress>
#include <assert.h>
#include <iostream>

#include "server.h"
#include "server_p.h"
#include "config.h"

QDebug operator<<(QDebug dbg, const Server &s)
{
	return operator<<(dbg, &s);
}

QDebug operator<<(QDebug dbg, const Server *s)
{
	dbg.nospace() << "Server[";
	if( s == 0 ) {
		dbg.nospace() << "0";
	} else {
		const ServerConfig *sc = s->config();
		dbg.nospace() << sc->host << ":" << sc->port;
	}
	dbg.nospace() << "]";

	return dbg.maybeSpace();
}

QDebug operator<<(QDebug dbg, const Irc::Buffer *b)
{
	dbg.nospace() << "Irc::Buffer[";
	if( b == 0 ) {
		dbg.nospace() << "0";
	} else {
		dbg.nospace() << b->receiver_;
	}
	dbg.nospace() << "]";
	return dbg.maybeSpace();
}

Server::Server()
: QObject()
, config_( 0 )
, connectTimer_( 0 )
, thread_( new ServerThread(this) )
{
#define SERVER_RELAY_1STR( signname ) \
	connect( thread_, SIGNAL(signname(const QString &, Irc::Buffer *)), \
	         this,    SIGNAL(signname(const QString &, Irc::Buffer *)), \
	         Qt::BlockingQueuedConnection );

#define SERVER_RELAY_2STR( signname ) \
	connect( thread_, SIGNAL(signname(const QString &, const QString &, Irc::Buffer *)), \
	         this,    SIGNAL(signname(const QString &, const QString &, Irc::Buffer *)), \
	         Qt::BlockingQueuedConnection );

#define SERVER_RELAY_3STR( signname ) \
	connect( thread_, SIGNAL(signname(const QString &, const QString &, const QString &, Irc::Buffer *)), \
	         this,    SIGNAL(signname(const QString &, const QString &, const QString &, Irc::Buffer *)), \
	         Qt::BlockingQueuedConnection );

	SERVER_RELAY_1STR( motdReceived );
	SERVER_RELAY_1STR( joined );
	SERVER_RELAY_2STR( parted );
	SERVER_RELAY_2STR( quit );
	SERVER_RELAY_2STR( nickChanged );
	SERVER_RELAY_3STR( modeChanged );
	SERVER_RELAY_2STR( topicChanged );
	SERVER_RELAY_3STR( invited );
	SERVER_RELAY_3STR( kicked );
	SERVER_RELAY_2STR( messageReceived );
	SERVER_RELAY_2STR( noticeReceived );
	SERVER_RELAY_2STR( ctcpRequestReceived );
	SERVER_RELAY_2STR( ctcpReplyReceived );
	SERVER_RELAY_2STR( ctcpActionReceived );

#undef SERVER_RELAY_1STR
#undef SERVER_RELAY_2STR
#undef SERVER_RELAY_3STR

	connect( thread_, SIGNAL(numericMessageReceived(const QString&, uint, const QStringList&, Irc::Buffer*)),
	         this,    SIGNAL(numericMessageReceived(const QString&, uint, const QStringList&, Irc::Buffer*)),
	         Qt::BlockingQueuedConnection );
	connect( thread_, SIGNAL(unknownMessageReceived(const QString&, const QStringList&, Irc::Buffer*)),
	         this,    SIGNAL(unknownMessageReceived(const QString&, const QStringList&, Irc::Buffer*)),
	         Qt::BlockingQueuedConnection );
	connect( thread_, SIGNAL(whoisReceived(const QString&, const QString&, bool, Irc::Buffer*)),
	         this,    SIGNAL(whoisReceived(const QString&, const QString&, bool, Irc::Buffer*)));
	connect( thread_, SIGNAL(namesReceived(const QString&, const QString&, const QStringList&, Irc::Buffer*)),
	         this,    SIGNAL(namesReceived(const QString&, const QString&, const QStringList&, Irc::Buffer*)));
	connect( thread_, SIGNAL(connected()),
	         this,    SIGNAL(connected()),
	         Qt::BlockingQueuedConnection );
	connect( thread_, SIGNAL(welcomed()),
	         this,    SIGNAL(welcomed()),
	         Qt::BlockingQueuedConnection );
}

Server::~Server()
{
	delete connectTimer_;
	thread_->quit();
	delete thread_;
}

const ServerConfig *Server::config() const
{
	return config_;
}

void Server::connectToServer()
{
	qDebug() << "Connecting to server" << this;
	Q_ASSERT( !config_->network->nickName.isEmpty() );
	Q_ASSERT( !thread_->isRunning() );
	thread_->setConfig(config_);
	thread_->start();
}

void Server::disconnectFromServer( Network::DisconnectReason reason )
{
	QString reasonString;
	switch( reason )
	{
#define rw(x) QLatin1String(x)
	case Network::ShutdownReason:
		reasonString = rw("Shutting down");
		break;
	case Network::ConfigurationReloadReason:
		reasonString = rw("Reloading configuration");
		break;
	case Network::SwitchingServersReason:
		reasonString = rw("Switching servers");
		break;
	case Network::ErrorReason:
		reasonString = rw("Unknown error");
		break;
	case Network::AdminRequestReason:
		reasonString = rw("An admin asked me to disconnect");
		break;
	case Network::UnknownReason:
	default:
		reasonString = rw("See you around!");
#undef rw
	}

	quit( reasonString );
}


/**
 * @brief Create a Server from a given ServerConfig.
 */
Server *Server::fromServerConfig( const ServerConfig *c )
{
	Server *s = new Server;
	s->config_ = c;
	return s;
}


QString Server::motd() const
{
	qWarning() << "MOTD cannot be retrieved.";
	return QString();
}

void Server::quit( const QString &reason ) {
	thread_->quit(reason);
}
void Server::whois( const QString &destination ) {
	//int sep = destination.indexOf('!');
	//if(sep != -1) {
	//	thread_->whois(destination.left(sep));
	//} else {
		thread_->whois(destination);
	//}
}
void Server::ctcpAction( const QString &destination, const QString &message ) {
	thread_->ctcpAction(destination, message);
}
void Server::ctcpRequest( const QString &destination, const QString &message ) {
	thread_->ctcpRequest(destination, message);
}
void Server::join( const QString &channel, const QString &key ) {
	thread_->join(channel, key);
}
void Server::part( const QString &channel, const QString &reason ) {
	thread_->part(channel, reason);
}
void Server::message( const QString &destination, const QString &message ) {
	thread_->message(destination, message);
}

void Irc::Buffer::message(const QString &message) {
	assert(!receiver_.isEmpty());
	session_->message(receiver_, message);
}

#define SERVER_SLOT_RELAY_1STR( slotname, signname ) \
void ServerThread::slotname ( const QString &str, Irc::Buffer *buf ) { \
	Q_ASSERT( buf != 0 ); \
	emit signname ( str, buf ); \
}

#define SERVER_SLOT_RELAY_2STR( slotname, signname ) \
void ServerThread::slotname ( const QString &str, const QString &str2, Irc::Buffer *buf ) { \
	Q_ASSERT( buf != 0 ); \
	emit signname ( str, str2, buf ); \
}

#define SERVER_SLOT_RELAY_3STR( slotname, signname ) \
void ServerThread::slotname (const QString &str, const QString &str2, const QString &str3, Irc::Buffer *buf ) \
{ \
	Q_ASSERT( buf != 0 ); \
	emit signname ( str, str2, str3, buf ); \
}

SERVER_SLOT_RELAY_1STR( slotMotdReceived, motdReceived );
SERVER_SLOT_RELAY_1STR( slotJoined, joined );
SERVER_SLOT_RELAY_2STR( slotParted, parted );
SERVER_SLOT_RELAY_2STR( slotQuit, quit );
SERVER_SLOT_RELAY_2STR( slotNickChanged, nickChanged );
SERVER_SLOT_RELAY_3STR( slotModeChanged, modeChanged );
SERVER_SLOT_RELAY_2STR( slotTopicChanged, topicChanged );
SERVER_SLOT_RELAY_3STR( slotInvited, invited );
SERVER_SLOT_RELAY_3STR( slotKicked, kicked );
SERVER_SLOT_RELAY_2STR( slotMessageReceived, messageReceived );
SERVER_SLOT_RELAY_2STR( slotNoticeReceived, noticeReceived );
SERVER_SLOT_RELAY_2STR( slotCtcpRequestReceived, ctcpRequestReceived );
SERVER_SLOT_RELAY_2STR( slotCtcpReplyReceived, ctcpReplyReceived );
SERVER_SLOT_RELAY_2STR( slotCtcpActionReceived, ctcpActionReceived );

#undef SERVER_SLOT_RELAY_1STR
#undef SERVER_SLOT_RELAY_2STR
#undef SERVER_SLOT_RELAY_3STR

void ServerThread::slotNumericMessageReceived( const QString &origin, uint code,
	const QStringList &args, Irc::Buffer *buf )
{
	Q_ASSERT( buf != 0 );
	// Also emit some other interesting signals
	if(code == 311) {
		in_whois_for_ = args[1];
		Q_ASSERT( !whois_identified_ );
	}
	// TODO: should use CAP IDENTIFY_MSG for this:
	else if(code == 307 || code == 330) // 330 means "logged in as", but doesn't check whether nick is grouped
	{
		whois_identified_ = true;
	}
	else if(code == 318)
	{
		emit whoisReceived( origin, in_whois_for_, whois_identified_, buf );
		whois_identified_ = false;
		in_whois_for_.clear();
	}
	// part of NAMES
	else if(code == 353)
	{
		QStringList names = args.last().split(' ', QString::SkipEmptyParts);
		in_names_.append(names);
	}
	else if(code == 366)
	{
		emit namesReceived( origin, args.at(1), in_names_, buf );
		in_names_.clear();
	}
	emit numericMessageReceived( origin, code, args, buf );
}

void ServerThread::slotUnknownMessageReceived( const QString &str, const QStringList &list, Irc::Buffer *buf )
{
	Q_ASSERT( buf != 0 );
	emit unknownMessageReceived( str, list, buf );
}

void ServerThread::slotConnected()
{
	emit connected();
}

void ServerThread::slotWelcomed()
{
	emit welcomed();
}

void irc_eventcode_callback(irc_session_t *s, unsigned int event, const char *origin, const char **p, unsigned int count) {
	ServerThread *server = (ServerThread*) irc_get_ctx(s);
	assert(server->getIrc() == s);
	QStringList params;
	for(unsigned int i = 0; i < count; ++i) {
		params.append(QString::fromUtf8(p[i]));
	}
	Irc::Buffer *b = new Irc::Buffer(server->server());
	server->slotNumericMessageReceived(QString::fromUtf8(origin), event, params, b);
}

void irc_callback(irc_session_t *s, const char *e, const char *o, const char **params, unsigned int count) {
	ServerThread *server = (ServerThread*) irc_get_ctx(s);
	assert(server->getIrc() == s);

	std::string event(e);
	Irc::Buffer *b = new Irc::Buffer(server->server());

	// for now, keep these QStrings:
	QString origin = QString::fromUtf8(o);
	int exclamMark = origin.indexOf('!');
	if(exclamMark != -1) {
		origin = origin.left(exclamMark);
	}

	if(event == "CONNECT") {
		assert(count <= 2);
		server->slotConnected();
		server->slotWelcomed();
		// first one is my nick
		// second one is "End of message of the day"
		// ignore
	} else if(event == "NICK") {
		assert(count == 1);
		server->slotNickChanged(origin, QString::fromUtf8(params[0]), b);
	} else if(event == "QUIT") {
		assert(count == 0 || count == 1);
		QString message;
		if(count == 1)
			message = QString::fromUtf8(params[0]);
		server->slotQuit(origin, message, b);
	} else if(event == "JOIN") {
		assert(count == 1);
		b->setReceiver(QString::fromUtf8(params[0]));
		server->slotJoined(origin, b);
	} else if(event == "PART") {
		assert(count == 1 || count == 2);
		QString message;
		if(count == 2)
			message = QString::fromUtf8(params[1]);
		b->setReceiver(QString::fromUtf8(params[0]));
		server->slotParted(origin, message, b);
	} else if(event == "MODE") {
		assert(count >= 1);
		QString mode;
		if(count == 1) {
			mode = QString::fromUtf8(params[0]);
		} else {
			mode = QString::fromUtf8(params[1]);
			b->setReceiver(QString::fromUtf8(params[0]));
		}
		QStringList args;
		for(unsigned int i = 2; i < count; ++i)
			args.append(QString::fromUtf8(params[i]));
		server->slotModeChanged(origin, mode, args.join(" "), b);
	} else if(event == "UMODE") {
		assert(count == 1);
		server->slotModeChanged(origin, params[0], QString(), b);
	} else if(event == "TOPIC") {
		assert(count == 1 || count == 2);
		QString topic;
		if(count == 2)
			topic = QString::fromUtf8(params[1]);
		b->setReceiver(QString::fromUtf8(params[0]));
		server->slotTopicChanged(origin, topic, b);
	} else if(event == "KICK") {
		assert(count > 1 && count <= 3);
		QString nick;
		QString message;
		if(count >= 2)
			nick = QString::fromUtf8(params[1]);
		if(count == 3)
			message = QString::fromUtf8(params[2]);
		b->setReceiver(QString::fromUtf8(params[0]));
		server->slotKicked(origin, nick, message, b);
	} else if(event == "CHANNEL" || event == "PRIVMSG") {
		assert(count == 1 || count == 2);
		QString message;
		if(count == 2)
			message = QString::fromUtf8(params[1]);
		b->setReceiver(QString::fromUtf8(params[0]));
		server->slotMessageReceived(origin, message, b);
	} else if(event == "NOTICE" || event == "CHANNEL_NOTICE") {
		assert(count == 1 || count == 2);
		QString message;
		if(count == 2)
			message = QString::fromUtf8(params[1]);
		b->setReceiver(QString::fromUtf8(params[0]));
		server->slotNoticeReceived(origin, message, b);
	} else if(event == "INVITE") {
		assert(count == 2);
		QString receiver = QString::fromUtf8(params[0]);
		QString channel  = QString::fromUtf8(params[1]);
		server->slotInvited(origin, receiver, channel, b);
	} else if(event == "CTCP_REQ" || event == "CTCP") {
		assert(count == 1);
		server->slotCtcpRequestReceived(origin, QString::fromUtf8(params[0]), b);
	} else if(event == "CTCP_REP") {
		assert(count == 1);
		server->slotCtcpReplyReceived(origin, QString::fromUtf8(params[0]), b);
	} else if(event == "CTCP_ACTION" || event == "ACTION") {
		assert(count == 1 || count == 2);
		QString message;
		if(count == 2)
			message = QString::fromUtf8(params[1]);
		b->setReceiver(QString::fromUtf8(params[0]));
		server->slotCtcpActionReceived(origin, message, b);
	} else if(event == "UNKNOWN") {
		QStringList args;
		for(unsigned int i = 0; i < count; ++i) {
			args.append(QString::fromUtf8(params[i]));
		}
		server->slotUnknownMessageReceived(origin, args, b);
	} else if(event == "ERROR") {
		QStringList args;
		for(unsigned int i = 0; i < count; ++i) {
			args.append(QString::fromUtf8(params[i]));
		}
		qWarning() << "Error received from libircclient: " << args;
		qWarning() << "Origin: " << origin;
	} else {
		std::cerr << "Unknown event received from libircclient: " << event << std::endl;
		assert(false);
	}
}

void ServerThread::run()
{
	irc_callbacks_t callbacks;
	callbacks.event_connect = irc_callback;
	callbacks.event_nick = irc_callback;
	callbacks.event_quit = irc_callback;
	callbacks.event_join = irc_callback;
	callbacks.event_part = irc_callback;
	callbacks.event_mode = irc_callback;
	callbacks.event_umode = irc_callback;
	callbacks.event_topic = irc_callback;
	callbacks.event_kick = irc_callback;
	callbacks.event_channel = irc_callback;
	callbacks.event_privmsg = irc_callback;
	callbacks.event_notice = irc_callback;
	// TODO
	//callbacks.event_channel_notice = irc_callback;
	callbacks.event_invite = irc_callback;
	callbacks.event_ctcp_req = irc_callback;
	callbacks.event_ctcp_rep = irc_callback;
	callbacks.event_ctcp_action = irc_callback;
	callbacks.event_unknown = irc_callback;
	callbacks.event_numeric = irc_eventcode_callback;
	callbacks.event_dcc_chat_req = NULL;
	callbacks.event_dcc_send_req = NULL;

	irc_ = irc_create_session(&callbacks);
	if(!irc_) {
		std::cerr << "Couldn't create IRC session in Server.";
		abort();
	}
	irc_set_ctx(irc_, this);

	Q_ASSERT( !config_->network->nickName.isEmpty() );
	irc_connect(irc_, config_->host.toLatin1(),
		config_->port,
		config_->network->password.toLatin1(),
		config_->network->nickName.toLatin1(),
		config_->network->userName.toLatin1(),
		config_->network->fullName.toLatin1());
	irc_run(irc_);
}

