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

std::map<std::string, Network*> Network::networks_;

std::string Network::toString(const Network *n)
{
	std::stringstream res;
	res << "Network[";
	if(n  == 0) {
		res << "0";
	} else {
		const NetworkConfig *nc = n->config();
		res << nc->displayName.toStdString() << ":" << Server::toString(n->activeServer());
	}
	res << "]";

	return res.str();
}

/**
 * @brief Constructor.
 */
Network::Network( const std::string &name )
: activeServer_(0)
, config_(0)
, me_(0)
{
  assert( !contains(networks_, name) );
  networks_[name] = this;
}


/**
 * @brief Destructor.
 */
Network::~Network()
{
  disconnectFromNetwork();
  assert(contains(networks_, config_->name.toStdString()) && getNetwork( config_->name.toStdString() ) == this);
  networks_.erase( config_->name.toStdString() );
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
bool serverNicenessLessThan( const ServerConfig *c1, const ServerConfig *c2 )
{
  int var1 = c1->priority;
  int var2 = c2->priority;

  Network *n;

  // if no network is configured for either, we're done
  if( c1->network == 0 || c2->network == 0 )
    goto ready;

  // otherwise, look at server undesirability (failure rate) too
  n = Network::fromNetworkConfig( c1->network, 0 );
  assert( n == Network::fromNetworkConfig( c2->network, 0 ) );

  var1 -= n->serverUndesirability( c1 );
  var2 -= n->serverUndesirability( c2 );

ready:
  // If the priorities are equal, randomise the order.
  if( var1 == var2 )
    return (qrand() % 2) == 0 ? -1 : 1;
  return var1 < var2;
}

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
    qWarning() << "Trying to connect to network" << config_->displayName
               << "but there are no servers to connect to!";
    return;
  }

  // Find the best server to use

  // First, sort the list by priority and earlier failures
  std::vector<ServerConfig*> sortedServers = servers();
  std::sort( sortedServers.begin(), sortedServers.end(), serverNicenessLessThan );

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

  activeServer_ = Server::fromServerConfig( server, this );
  activeServer_->connectToServer();
}

void Network::joinedChannel(const std::string &user, Irc::Buffer *b)
{
	User u(user, this);
	if(u.isMe() && !contains(knownUsers_, strToLower(b->receiver()))) {
		knownUsers_[strToLower(b->receiver())] = std::vector<std::string>();
	}
	std::vector<std::string> users = knownUsers_[strToLower(b->receiver())];
	if(!contains(users, strToLower(user)))
		knownUsers_[strToLower(b->receiver())].push_back(strToLower(user));
#ifdef DEBUG
	qDebug() << "User " << user << " joined channel " << b->receiver();
	qDebug() << "knownUsers is now: " << knownUsers_;
#endif
}

void Network::partedChannel(const std::string &user, const std::string &, Irc::Buffer *b)
{
	User u(user, this);
	if(u.isMe()) {
		knownUsers_.erase(strToLower(b->receiver()));
	} else {
		erase(knownUsers_[strToLower(b->receiver())], strToLower(user));
	}
	if(!isKnownUser(u.nick())) {
		erase(identifiedUsers_, strToLower(u.nick()));
	}
#ifdef DEBUG
	qDebug() << "User " << user << " left channel " << b->receiver();
	qDebug() << "knownUsers is now: " << knownUsers_;
	qDebug() << "identifiedUsers is now: " << identifiedUsers_;
#endif
}

void Network::slotQuit(const std::string &origin, const std::string&, Irc::Buffer *b)
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

void Network::slotNickChanged( const std::string &origin, const std::string &nick, Irc::Buffer* )
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

void Network::kickedChannel(const std::string&, const std::string &user, const std::string&, Irc::Buffer *b)
{
	User u(user, this);
	if(u.isMe()) {
		knownUsers_.erase(strToLower(b->receiver()));
	} else {
		erase(knownUsers_[strToLower(b->receiver())], strToLower(u.nick()));
	}
	if(!isKnownUser(u.nick())) {
		erase(identifiedUsers_, strToLower(u.nick()));
	}
#ifdef DEBUG
	qDebug() << "User " << user << " was kicked from " << b->receiver();
	qDebug() << "knownUsers is now: " << knownUsers_;
	qDebug() << "identifiedUsers is now: " << identifiedUsers_;
#endif
}

void Network::onFailedConnection()
{
  qDebug() << "Connection failed on " << this;

  identifiedUsers_.clear();
  knownUsers_.clear();

  Irc::Buffer *b = new Irc::Buffer(activeServer_);
  plugins_->ircEvent("DISCONNECT", "", std::vector<std::string>(), this, b);

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


/**
 * @brief Create a Network from a given NetworkConfig.
 */
Network *Network::fromNetworkConfig( const NetworkConfig *c, PluginComm *p )
{
	if(contains(networks_, c->name.toStdString()))
		return networks_[c->name.toStdString()];

	Network *n = new Network( c->name.toStdString() );
	n->config_ = c;
	n->plugins_ = p;
	return n;
}


/**
 * @brief Retrieve the Network with given name.
 */
Network *Network::getNetwork( const std::string &name )
{
	if( !contains(networks_, name) )
		return 0;
	return networks_[name];
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
    me_ = new User( config_->nickName.toStdString(), config_->nickName.toStdString(),
                    "__local__", this );
  }

  return me_;
}



std::string Network::networkName() const
{
  return config()->name.toStdString();
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

void Network::slotWhoisReceived(const std::string &origin, const std::string &nick, bool identified, Irc::Buffer *buf) {
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

void Network::slotNamesReceived(const std::string&, const std::string &channel, const std::vector<std::string> &names, Irc::Buffer *buf ) {
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

void Network::slotIrcEvent(const std::string &event, const std::string &origin, const std::vector<std::string> &params, Irc::Buffer *buf) {
#define MIN(a) if(params.size() < a) { fprintf(stderr, "Too few parameters for event %s\n", event.c_str()); return; }
	if(event == "CONNECT") {
		serverIsActuallyOkay(activeServer_->config());
	} else if(event == "JOIN") {
		MIN(1);
		joinedChannel(origin, buf);
	} else if(event == "PART") {
		MIN(1);
		partedChannel(origin, std::string(), buf);
	} else if(event == "KICK") {
		MIN(2);
		kickedChannel(origin, params[1], std::string(), buf);
	} else if(event == "QUIT") {
		std::string message;
		if(params.size() > 0) {
			message = params[0];
		}
		slotQuit(origin, message, buf);
	} else if(event == "NICK") {
		MIN(1);
		slotNickChanged(origin, params[0], buf);
	}
#undef MIN
	assert(plugins_ != 0);
	plugins_->ircEvent(event, origin, params, this, buf);
}
