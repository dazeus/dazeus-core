/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#include "network.h"
#include "server.h"
#include "config.h"
#include "user.h"
#include "plugincomm.h"
#include "utils.h"

// #define DEBUG

std::string Network::toString(const Network *n)
{
	std::stringstream res;
	res << "Network[";
	if(n  == 0) {
		res << "0";
	} else {
		const NetworkConfig *nc = n->config();
		res << nc->displayName << ":" << Server::toString(n->activeServer());
	}
	res << "]";

	return res.str();
}

/**
 * @brief Constructor.
 */
Network::Network( const NetworkConfig *c, PluginComm *p )
: activeServer_(0)
, config_(c)
, me_(0)
, plugins_(p)
{}


/**
 * @brief Destructor.
 */
Network::~Network()
{
  disconnectFromNetwork();
}


/**
 * Returns the active server in this network, or 0 if there is none.
 */
Server *Network::activeServer() const
{
  return activeServer_;
}


/**
 * @brief Whether this network is marked for autoconnection.
 */
bool Network::autoConnectEnabled() const
{
  return config_->autoConnect;
}

void Network::action( std::string destination, std::string message )
{
  if( !activeServer_ )
    return;
  activeServer_->ctcpAction( destination, message );
}

void Network::names( std::string channel )
{
  if( !activeServer_ )
    return;
  return activeServer_->names( channel );
}

std::vector<std::string> Network::joinedChannels() const
{
	std::vector<std::string> res;
	std::map<std::string,std::vector<std::string> >::const_iterator it;
	for(it = knownUsers_.begin(); it != knownUsers_.end(); ++it) {
		res.push_back(it->first);
	}
	return res;
}

/**
 * @brief Chooses one server or the other based on the priority and failure
 *        rate.
 * Failure rate is recorded in the Network object that the ServerConfig object
 * links to via the NetworkConfig, if there is such a NetworkConfig object.
 * You must give two servers of the same Network.
 */
struct ServerSorter {
	private:
	Network *n_;

	public:
	ServerSorter(Network *n) : n_(n) {}
	bool operator()(const ServerConfig *c1, const ServerConfig *c2) {
		assert(c1->network == c2->network);
		assert(c1->network == n_->config());

		int prio1 = c1->priority - n_->serverUndesirability(c1);
		int prio2 = c2->priority - n_->serverUndesirability(c2);

		if( prio1 == prio2 ) // choose randomly
			return (qrand() % 2) == 0 ? -1 : 1;
		return prio1 < prio2;
	}
};

const NetworkConfig *Network::config() const
{
  return config_;
}

void Network::connectToNetwork( bool reconnect )
{
  if( !reconnect && activeServer_ )
    return;

  printf("Connecting to network: %s\n", Network::toString(this).c_str());

  // Check if there *is* a server to use
  if( servers().size() == 0 )
  {
    printf("Trying to connect to network '%s', but there are no servers to connect to!\n",
      config_->displayName.c_str());
    return;
  }

  // Find the best server to use

  // First, sort the list by priority and earlier failures
  std::vector<ServerConfig*> sortedServers = servers();
  std::sort( sortedServers.begin(), sortedServers.end(), ServerSorter(this) );

  // Then, take the first one and create a Server around it
  ServerConfig *best = sortedServers[0];

  // And set it as the active server, and connect.
  connectToServer( best, true );
}


void Network::connectToServer( ServerConfig *server, bool reconnect )
{
  if( !reconnect && activeServer_ )
    return;
  assert(knownUsers_.size() == 0);
  assert(identifiedUsers_.size() == 0);

  if( activeServer_ )
  {
    activeServer_->disconnectFromServer( SwitchingServersReason );
    // TODO: maybe deleteLater?
    delete(activeServer_);
  }

  activeServer_ = new Server( server, this );
  activeServer_->connectToServer();
}

void Network::joinedChannel(const std::string &user, const std::string &receiver)
{
	User u(user, this);
	if(u.isMe() && !contains(knownUsers_, strToLower(receiver))) {
		knownUsers_[strToLower(receiver)] = std::vector<std::string>();
	}
	std::vector<std::string> users = knownUsers_[strToLower(receiver)];
	if(!contains(users, strToLower(user)))
		knownUsers_[strToLower(receiver)].push_back(strToLower(user));
#ifdef DEBUG
	qDebug() << "User " << user << " joined channel " << receiver;
	qDebug() << "knownUsers is now: " << knownUsers_;
#endif
}

void Network::partedChannel(const std::string &user, const std::string &, const std::string &receiver)
{
	User u(user, this);
	if(u.isMe()) {
		knownUsers_.erase(strToLower(receiver));
	} else {
		erase(knownUsers_[strToLower(receiver)], strToLower(user));
	}
	if(!isKnownUser(u.nick())) {
		erase(identifiedUsers_, strToLower(u.nick()));
	}
#ifdef DEBUG
	qDebug() << "User " << user << " left channel " << receiver;
	qDebug() << "knownUsers is now: " << knownUsers_;
	qDebug() << "identifiedUsers is now: " << identifiedUsers_;
#endif
}

void Network::slotQuit(const std::string &origin, const std::string&, const std::string &receiver)
{
	std::map<std::string,std::vector<std::string> >::iterator it;
	for(it = knownUsers_.begin(); it != knownUsers_.end(); ++it) {
		erase(it->second, strToLower(origin));
	}
	if(!isKnownUser(origin)) {
		erase(identifiedUsers_, strToLower(origin));
	}
#ifdef DEBUG
	qDebug() << "User " << origin << " quit IRC.";
	qDebug() << "knownUsers is now: " << knownUsers_;
	qDebug() << "identifiedUsers is now: " << identifiedUsers_;
#endif
}

void Network::slotNickChanged( const std::string &origin, const std::string &nick, const std::string & )
{
	erase(identifiedUsers_, strToLower(origin));
	erase(identifiedUsers_, strToLower(nick));

	User u(origin, this);
	if(u.isMe()) {
		user()->setNick(nick);
	}
	std::map<std::string,std::vector<std::string> >::iterator it;
	for(it = knownUsers_.begin(); it != knownUsers_.end(); ++it) {
		if(contains(it->second, strToLower(origin))) {
			erase(it->second, strToLower(origin));
			it->second.push_back(strToLower(nick));
		}
	}
#ifdef DEBUG
	qDebug() << "User " << origin << " changed nicks to " << nick;
	qDebug() << "knownUsers is now: " << knownUsers_;
	qDebug() << "identifiedUsers is now: " << identifiedUsers_;
#endif
}

void Network::kickedChannel(const std::string&, const std::string &user, const std::string&, const std::string &receiver)
{
	User u(user, this);
	if(u.isMe()) {
		knownUsers_.erase(strToLower(receiver));
	} else {
		erase(knownUsers_[strToLower(receiver)], strToLower(u.nick()));
	}
	if(!isKnownUser(u.nick())) {
		erase(identifiedUsers_, strToLower(u.nick()));
	}
#ifdef DEBUG
	qDebug() << "User " << user << " was kicked from " << receiver;
	qDebug() << "knownUsers is now: " << knownUsers_;
	qDebug() << "identifiedUsers is now: " << identifiedUsers_;
#endif
}

void Network::onFailedConnection()
{
  qDebug() << "Connection failed on " << this;

  identifiedUsers_.clear();
  knownUsers_.clear();

  plugins_->ircEvent("DISCONNECT", "", std::vector<std::string>(), this);

  // Flag old server as undesirable
  flagUndesirableServer( activeServer_->config() );
  delete activeServer_;
  activeServer_ = 0;

  connectToNetwork();
}


void Network::ctcp( std::string destination, std::string message )
{
  if( !activeServer_ )
    return;
  return activeServer_->ctcpAction( destination, message );
}


/**
 * @brief Disconnect from the network.
 */
void Network::disconnectFromNetwork( DisconnectReason reason )
{
  if( activeServer_ == 0 )
    return;

  identifiedUsers_.clear();
  knownUsers_.clear();

  activeServer_->disconnectFromServer( reason );
  // TODO: maybe deleteLater?
  delete activeServer_;
  activeServer_ = 0;
}


void Network::joinChannel( std::string channel )
{
  if( !activeServer_ )
    return;
  return activeServer_->join( channel );
}


void Network::leaveChannel( std::string channel )
{
  if( !activeServer_ )
    return;
  activeServer_->part( channel );
}


void Network::say( std::string destination, std::string message )
{
  if( !activeServer_ )
    return;
  activeServer_->message( destination, message );
}


void Network::sendWhois( std::string destination )
{
  activeServer_->whois(destination);
}

const std::vector<ServerConfig*> &Network::servers() const
{
  return config_->servers;
}


User *Network::user()
{
  if( me_ == 0 )
  {
    me_ = new User( config_->nickName, config_->nickName,
                    "__local__", this );
  }

  return me_;
}



std::string Network::networkName() const
{
  return config()->name;
}



int Network::serverUndesirability( const ServerConfig *sc ) const
{
	return contains(undesirables_, sc) ? undesirables_.find(sc)->second : 0;
}

void Network::flagUndesirableServer( const ServerConfig *sc )
{
	if(contains(undesirables_, sc))
		undesirables_[sc] = undesirables_[sc] + 1;
	undesirables_[sc] = 1;
}

void Network::serverIsActuallyOkay( const ServerConfig *sc )
{
  undesirables_.erase(sc);
}

bool Network::isIdentified(const std::string &user) const {
	return contains(identifiedUsers_, strToLower(user));
}

bool Network::isKnownUser(const std::string &user) const {
	std::map<std::string,std::vector<std::string> >::const_iterator it;
	for(it = knownUsers_.begin(); it != knownUsers_.end(); ++it) {
		if(contains(it->second, strToLower(user))) {
			return true;
		}
	}
	return false;
}

void Network::slotWhoisReceived(const std::string &origin, const std::string &nick, bool identified) {
	if(!identified) {
		erase(identifiedUsers_, strToLower(nick));
	} else if(identified && !contains(identifiedUsers_, strToLower(nick)) && isKnownUser(nick)) {
		identifiedUsers_.push_back(strToLower(nick));
	}
#ifdef DEBUG
	qDebug() << "Whois received for"<< nick <<"; identifiedUsers_ is now:";
	qDebug() << identifiedUsers_;
#endif
}

void Network::slotNamesReceived(const std::string&, const std::string &channel, const std::vector<std::string> &names, const std::string &receiver ) {
	assert(contains(knownUsers_, strToLower(channel)));
	std::vector<std::string> &users = knownUsers_[strToLower(channel)];
	foreach(std::string n, names) {
		unsigned int nickStart;
		for(nickStart = 0; nickStart < n.length(); ++nickStart) {
			if(n[nickStart] != '@' && n[nickStart] != '~' && n[nickStart] != '+'
			&& n[nickStart] != '%' && n[nickStart] != '!') {
				break;
			}
		}
		n = n.substr(nickStart);
		if(!contains(users, strToLower(n)))
			users.push_back(strToLower(n));
	}
#ifdef DEBUG
	qDebug() << "Names received for" << channel << "; knownUsers_ is now:";
	qDebug() << knownUsers_;
#endif
}

void Network::slotIrcEvent(const std::string &event, const std::string &origin, const std::vector<std::string> &params) {
	std::string receiver;
	if(params.size() > 0)
		receiver = params[0];

#define MIN(a) if(params.size() < a) { fprintf(stderr, "Too few parameters for event %s\n", event.c_str()); return; }
	if(event == "CONNECT") {
		serverIsActuallyOkay(activeServer_->config());
	} else if(event == "JOIN") {
		MIN(1);
		joinedChannel(origin, receiver);
	} else if(event == "PART") {
		MIN(1);
		partedChannel(origin, std::string(), receiver);
	} else if(event == "KICK") {
		MIN(2);
		kickedChannel(origin, params[1], std::string(), receiver);
	} else if(event == "QUIT") {
		std::string message;
		if(params.size() > 0) {
			message = params[0];
		}
		slotQuit(origin, message, receiver);
	} else if(event == "NICK") {
		MIN(1);
		slotNickChanged(origin, params[0], receiver);
	}
#undef MIN
	assert(plugins_ != 0);
	plugins_->ircEvent(event, origin, params, this);
}
