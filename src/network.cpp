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

QHash<QString, Network*> Network::networks_;

QDebug operator<<(QDebug dbg, const Network &n)
{
  return operator<<( dbg, &n );
}

QDebug operator<<(QDebug dbg, const Network *n)
{
  dbg.nospace() << "Network[";
  if( n == 0 ) {
    dbg.nospace() << "0";
  } else {
    const NetworkConfig *nc = n->config();
    dbg.nospace() << nc->displayName << "," << n->activeServer();
  }
  dbg.nospace() << "]";

  return dbg.maybeSpace();
}

/**
 * @brief Constructor.
 */
Network::Network( const QString &name )
: QObject()
, activeServer_(0)
, config_(0)
, me_(0)
{
  Q_ASSERT( !networks_.contains( name ) );
  networks_.insert( name, this );
}


/**
 * @brief Destructor.
 */
Network::~Network()
{
  disconnectFromNetwork();
  Q_ASSERT( networks_.contains( config_->name ) && getNetwork( config_->name.toStdString() ) == this );
  networks_.remove( config_->name );
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

void Network::action( QString destination, QString message )
{
  if( !activeServer_ )
    return;
  activeServer_->ctcpAction( destination.toStdString(), message.toStdString() );
}

void Network::names( QString channel )
{
  if( !activeServer_ )
    return;
  return activeServer_->names( channel.toStdString() );
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
  Q_ASSERT( n == Network::fromNetworkConfig( c2->network, 0 ) );

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

  qDebug() << "Connecting to network: " << this;

  // Check if there *is* a server to use
  if( servers().count() == 0 )
  {
    qWarning() << "Trying to connect to network" << config_->displayName
               << "but there are no servers to connect to!";
    return;
  }

  // Find the best server to use

  // First, sort the list by priority and earlier failures
  QList<ServerConfig*> sortedServers = servers();
  qSort( sortedServers.begin(), sortedServers.end(), serverNicenessLessThan );

  // Then, take the first one and create a Server around it
  ServerConfig *best = sortedServers[0];

  // And set it as the active server, and connect.
  connectToServer( best, true );
}


void Network::connectToServer( ServerConfig *server, bool reconnect )
{
  if( !reconnect && activeServer_ )
    return;
  Q_ASSERT(knownUsers_.isEmpty());
  Q_ASSERT(identifiedUsers_.isEmpty());

  if( activeServer_ )
  {
    activeServer_->disconnectFromServer( SwitchingServersReason );
    // TODO: maybe deleteLater?
    delete(activeServer_);
  }

  activeServer_ = Server::fromServerConfig( server, this );
  activeServer_->connectToServer();
}

void Network::joinedChannel(const QString &user, Irc::Buffer *b)
{
	User u(user.toStdString(), this);
	if(u.isMe() && !knownUsers_.keys().contains(QString::fromStdString(strToLower(b->receiver())))) {
		knownUsers_.insert(QString::fromStdString(strToLower(b->receiver())), QList<QString>());
	}
	if(!knownUsers_[QString::fromStdString(strToLower(b->receiver()))].contains(user.toLower()))
		knownUsers_[QString::fromStdString(strToLower(b->receiver()))].append(user.toLower());
#ifdef DEBUG
	qDebug() << "User " << user << " joined channel " << QString::fromStdString(b->receiver());
	qDebug() << "knownUsers is now: " << knownUsers_;
#endif
}

void Network::partedChannel(const QString &user, const QString &, Irc::Buffer *b)
{
	User u(user.toStdString(), this);
	if(u.isMe()) {
		knownUsers_.remove(QString::fromStdString(strToLower(b->receiver())));
	} else {
		knownUsers_[QString::fromStdString(strToLower(b->receiver()))].removeAll(user.toLower());
	}
	if(!isKnownUser(u.nick())) {
		identifiedUsers_.removeAll(QString::fromStdString(strToLower(u.nick())));
	}
#ifdef DEBUG
	qDebug() << "User " << user << " left channel " << QString::fromStdString(b->receiver());
	qDebug() << "knownUsers is now: " << knownUsers_;
	qDebug() << "identifiedUsers is now: " << identifiedUsers_;
#endif
}

void Network::slotQuit(const QString &origin, const QString&, Irc::Buffer *b)
{
	foreach(const QString &channel, knownUsers_.keys()) {
		knownUsers_[channel.toLower()].removeAll(origin.toLower());
	}
	if(!isKnownUser(origin.toStdString())) {
		identifiedUsers_.removeAll(origin.toLower());
	}
#ifdef DEBUG
	qDebug() << "User " << origin << " quit IRC.";
	qDebug() << "knownUsers is now: " << knownUsers_;
	qDebug() << "identifiedUsers is now: " << identifiedUsers_;
#endif
}

void Network::slotNickChanged( const QString &origin, const QString &nick, Irc::Buffer* )
{
	identifiedUsers_.removeAll(origin.toLower());
	identifiedUsers_.removeAll(nick.toLower());
	User u(origin.toStdString(), this);
	if(u.isMe()) {
		user()->setNick(nick.toStdString());
	}
	foreach(const QString &channel, knownUsers_.keys()) {
		QStringList &users = knownUsers_[channel];
		if(users.removeAll(origin.toLower())) {
			users.append(nick.toLower());
		}
	}
#ifdef DEBUG
	qDebug() << "User " << origin << " changed nicks to " << nick;
	qDebug() << "knownUsers is now: " << knownUsers_;
	qDebug() << "identifiedUsers is now: " << identifiedUsers_;
#endif
}

void Network::kickedChannel(const QString&, const QString &user, const QString&, Irc::Buffer *b)
{
	User u(user.toStdString(), this);
	if(u.isMe()) {
		knownUsers_.remove(QString::fromStdString(strToLower(b->receiver())));
	} else {
		knownUsers_[QString::fromStdString(strToLower(b->receiver()))].removeAll(QString::fromStdString(strToLower(u.nick())));
	}
	if(!isKnownUser(u.nick())) {
		identifiedUsers_.removeAll(QString::fromStdString(strToLower(u.nick())));
	}
#ifdef DEBUG
	qDebug() << "User " << user << " was kicked from " << QString::fromStdString(b->receiver());
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
  plugins_->ircEvent("DISCONNECT", "", QStringList(), b);

  // Flag old server as undesirable
  flagUndesirableServer( activeServer_->config() );
  delete activeServer_;
  activeServer_ = 0;

  connectToNetwork();
}


void Network::ctcp( QString destination, QString message )
{
  if( !activeServer_ )
    return;
  return activeServer_->ctcpAction( destination.toStdString(), message.toStdString() );
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
 * @brief Get the Network belonging to this Buffer.
 */
Network *Network::fromBuffer( Irc::Buffer *b )
{
  // One of the Servers in one of the Networks in network_, is an
  // Irc::Session that has created this buffer.
  foreach( Network *n, networks_.values() )
  {
    if( n->activeServer() == b->session() )
    {
      return n;
    }
  }
  return 0;
}


/**
 * @brief Create a Network from a given NetworkConfig.
 */
Network *Network::fromNetworkConfig( const NetworkConfig *c, PluginComm *p )
{
  if( networks_.contains( c->name ) )
    return networks_.value( c->name );

  Network *n = new Network( c->name );
  n->config_ = c;
  n->plugins_ = p;
  return n;
}


/**
 * @brief Retrieve the Network with given name.
 */
Network *Network::getNetwork( const std::string &name )
{
  if( !networks_.contains( QString::fromStdString(name) ) )
    return 0;
  return networks_.value( QString::fromStdString(name) );
}

void Network::joinChannel( QString channel )
{
  if( !activeServer_ )
    return;
  return activeServer_->join( channel.toStdString() );
}


void Network::leaveChannel( QString channel )
{
  if( !activeServer_ )
    return;
  activeServer_->part( channel.toStdString() );
}


void Network::say( QString destination, QString message )
{
  if( !activeServer_ )
    return;
  activeServer_->message( destination.toStdString(), message.toStdString() );
}


void Network::sendWhois( QString destination )
{
  activeServer_->whois(destination.toStdString());
}

const QList<ServerConfig*> &Network::servers() const
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



QString Network::networkName() const
{
  return config()->name;
}



int Network::serverUndesirability( const ServerConfig *sc ) const
{
  return undesirables_.contains(sc) ? undesirables_.value(sc) : 0;
}

void Network::flagUndesirableServer( const ServerConfig *sc )
{
  if( undesirables_.contains(sc) )
    undesirables_.insert( sc, undesirables_.value(sc) + 1 );
  undesirables_.insert( sc, 0 );
}

void Network::serverIsActuallyOkay( const ServerConfig *sc )
{
  undesirables_.remove(sc);
}

bool Network::isIdentified(const QString &user) const {
	return identifiedUsers_.contains(user.toLower());
}

bool Network::isKnownUser(const std::string &user) const {
	foreach(const QString &chan, knownUsers_.keys()) {
		if(knownUsers_[chan.toLower()].contains(QString::fromStdString(strToLower(user)))) {
			return true;
		}
	}
	return false;
}

void Network::slotWhoisReceived(const std::string &origin, const std::string &nick, bool identified, Irc::Buffer *buf) {
	if(!identified) {
		identifiedUsers_.removeAll(QString::fromStdString(strToLower(nick)));
	} else if(identified && !identifiedUsers_.contains(QString::fromStdString(strToLower(nick))) && isKnownUser(nick)) {
		identifiedUsers_.append(QString::fromStdString(strToLower(nick)));
	}
#ifdef DEBUG
	qDebug() << "Whois received for"<< QString::fromStdString(nick) <<"; identifiedUsers_ is now:";
	qDebug() << identifiedUsers_;
#endif
}

void Network::slotNamesReceived(const std::string&, const std::string &channel, const std::vector<std::string> &names, Irc::Buffer *buf ) {
	Q_ASSERT(knownUsers_.keys().contains(QString::fromStdString(strToLower(channel))));
	QStringList &users = knownUsers_[QString::fromStdString(strToLower(channel))];
	foreach(std::string n, names) {
		unsigned int nickStart;
		for(nickStart = 0; nickStart < n.length(); ++nickStart) {
			if(n[nickStart] != '@' && n[nickStart] != '~' && n[nickStart] != '+'
			&& n[nickStart] != '%' && n[nickStart] != '!') {
				break;
			}
		}
		n = n.substr(nickStart);
		if(!users.contains(QString::fromStdString(strToLower(n))))
			users.append(QString::fromStdString(strToLower(n)));
	}
#ifdef DEBUG
	qDebug() << "Names received for" << QString::fromStdString(channel) << "; knownUsers_ is now:";
	qDebug() << knownUsers_;
#endif
}

void Network::slotIrcEvent(const std::string &event, const std::string &origin, const std::vector<std::string> &params, Irc::Buffer *buf) {
#define MIN(a) if(params.size() < a) { fprintf(stderr, "Too few parameters for event %s\n", event.c_str()); return; }
#define S(a) QString::fromStdString(a)
	if(event == "CONNECT") {
		serverIsActuallyOkay(activeServer_->config());
	} else if(event == "JOIN") {
		MIN(1);
		joinedChannel(S(origin), buf);
	} else if(event == "PART") {
		MIN(1);
		partedChannel(S(origin), QString(), buf);
	} else if(event == "KICK") {
		MIN(2);
		kickedChannel(S(origin), S(params[1]), QString(), buf);
	} else if(event == "QUIT") {
		QString message;
		if(params.size() > 0) {
			message = S(params[0]);
		}
		slotQuit(S(origin), message, buf);
	} else if(event == "NICK") {
		MIN(1);
		slotNickChanged(S(origin), S(params[0]), buf);
	}
#undef MIN
	Q_ASSERT(plugins_ != 0);
	QStringList qParams;
	for(unsigned int i = 0; i < params.size(); ++i) {
		qParams.append(S(params[i]));
	}
	plugins_->ircEvent(S(event), S(origin), qParams, buf);
#undef S
}
