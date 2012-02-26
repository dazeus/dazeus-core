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

// #define DEBUG

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
	connect( thread_, SIGNAL(ircEvent(const QString&, const QString&, const QStringList&, Irc::Buffer*)),
	         this,    SIGNAL(ircEvent(const QString&, const QString&, const QStringList&, Irc::Buffer*)),
	         Qt::BlockingQueuedConnection );
	connect( thread_, SIGNAL(whoisReceived(const QString&, const QString&, bool, Irc::Buffer*)),
	         this,    SIGNAL(whoisReceived(const QString&, const QString&, bool, Irc::Buffer*)),
	         Qt::BlockingQueuedConnection );
	connect( thread_, SIGNAL(namesReceived(const QString&, const QString&, const QStringList&, Irc::Buffer*)),
	         this,    SIGNAL(namesReceived(const QString&, const QString&, const QStringList&, Irc::Buffer*)),
	         Qt::BlockingQueuedConnection );
	connect( thread_, SIGNAL(disconnected()),
	         this,    SIGNAL(disconnected()) );
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
void Server::names( const QString &channel ) {
	thread_->names(channel);
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
		emit ircEvent( "WHOIS", origin, QStringList() << in_whois_for_ << (whois_identified_ ? "true" : "false"), buf );
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
		emit ircEvent( "NAMES", origin, QStringList() << args.at(1) << in_names_, buf );
		in_names_.clear();
	}
	emit ircEvent( "NUMERIC", origin, QStringList() << QString::number(code) << args, buf );
}

void ServerThread::slotIrcEvent(const QString &event, const QString &origin, const QStringList &args, Irc::Buffer *buf)
{
	Q_ASSERT(buf != 0);
	emit ircEvent(event, origin, args, buf);
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
	// From libircclient docs, but CHANNEL_NOTICE is bullshit...
	if(event == "CHANNEL_NOTICE") {
		event = "NOTICE";
	}

	// for now, keep these QStrings:
	QString origin = QString::fromUtf8(o);
	int exclamMark = origin.indexOf('!');
	if(exclamMark != -1) {
		origin = origin.left(exclamMark);
	}

	QStringList arguments;
	for(unsigned int i = 0; i < count; ++i) {
		arguments.append(QString::fromUtf8(params[i]));
	}
	if(arguments.size() >= 1) {
		b->setReceiver(arguments[0]);
	}

#ifdef DEBUG
	qDebug() << server->server() << " - " << QString::fromStdString(event) << " from " << origin << ": " << arguments;
#endif

	// TODO: handle disconnects nicely (probably using some ping and LIBIRC_ERR_CLOSED
	if(event == "ERROR") {
		qWarning() << "Error received from libircclient: " << arguments;
		qWarning() << "Origin: " << origin;
		server->slotDisconnected();
	} else if(event == "CONNECT") {
		qDebug() << "Connected to server:" << server->server();
	}

	server->slotIrcEvent(QString::fromStdString(event), origin, arguments, b);
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
