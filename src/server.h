/**
 * Copyright (c) Sjors Gielen, 2010-2012
 * See LICENSE for license.
 */

#ifndef SERVER_H
#define SERVER_H

#include <libircclient.h>
#include <cassert>
#include <iostream>
#include <vector>
#include <string>

#include "user.h"
#include "network.h"
#include "server.h"
#include "config.h"

// #define SERVER_FULLDEBUG

struct ServerConfig;

class Server;

namespace Irc {
	struct Buffer {
		// TEMPORARY PLACEHOLDER
		Server *session_;
		std::string receiver_;
		inline Server *session() const { return session_; }
		inline void setReceiver(const std::string &p) { receiver_ = p; }
		inline const std::string &receiver() const { return receiver_; }

		Buffer(Server *s) : session_(s) {}

		void message(const std::string &message);
	};
};

class Server
{

private:
	   Server();
	void run();

public:
	  ~Server();
	static Server *fromServerConfig( const ServerConfig *c, Network *n );
	const ServerConfig *config() const;
	std::string motd() const;
	void addDescriptors(fd_set *in_set, fd_set *out_set, int *maxfd);
	void processDescriptors(fd_set *in_set, fd_set *out_set);
	irc_session_t *getIrc() const;
	static std::string toString(const Server*);

	void connectToServer();
	void disconnectFromServer( Network::DisconnectReason );
	void quit( const std::string &reason );
	void whois( const std::string &destination );
	void ctcpAction( const std::string &destination, const std::string &message );
	void ctcpRequest( const std::string &destination, const std::string &message );
	void join( const std::string &channel, const std::string &key = std::string() );
	void part( const std::string &channel, const std::string &reason = std::string() );
	void message( const std::string &destination, const std::string &message );
	void names( const std::string &channel );
	void slotNumericMessageReceived( const std::string &origin, uint code, const std::vector<std::string> &params, Irc::Buffer *b );
	void slotIrcEvent(const std::string &event, const std::string &origin, const std::vector<std::string> &params, Irc::Buffer *b);
	void slotDisconnected();

private:
	const ServerConfig *config_;
	std::string   motd_;
	Network  *network_;
	irc_session_t *irc_;
	std::string in_whois_for_;
	bool whois_identified_;
	std::vector<std::string> in_names_;
};

#endif
