/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#ifndef NETWORK_H
#define NETWORK_H

#include <QtCore/QHash>
#include <QtCore/QMap>

#include "user.h"
#include <vector>
#include <string>

class Server;
class Network;
class PluginComm;

struct ServerConfig;
struct NetworkConfig;

namespace Irc
{
  class Buffer;
};

class Network
{

  friend class Server;

  public:
                   ~Network();

    static Network *fromNetworkConfig( const NetworkConfig *c, PluginComm *p );
    static Network *getNetwork( const std::string &name );
    static std::string toString(const Network *n);

    enum DisconnectReason {
      UnknownReason,
      ShutdownReason,
      ConfigurationReloadReason,
      SwitchingServersReason,
      ErrorReason,
      AdminRequestReason,
    };

    bool                        autoConnectEnabled() const;
    const std::vector<ServerConfig*> &servers() const;
    User                       *user();
    Server                     *activeServer() const;
    const NetworkConfig        *config() const;
    int                         serverUndesirability( const ServerConfig *sc ) const;
    std::string                 networkName() const;
    std::vector<std::string>    joinedChannels() const; // knownUsers_.keys()
    bool                        isIdentified(const std::string &user) const;
    bool                        isKnownUser(const std::string &user) const;

    void connectToNetwork( bool reconnect = false );
    void disconnectFromNetwork( DisconnectReason reason = UnknownReason );
    void joinChannel( std::string channel );
    void leaveChannel( std::string channel );
    void say( std::string destination, std::string message );
    void action( std::string destination, std::string message );
    void names( std::string channel );
    void ctcp( std::string destination, std::string message );
    void sendWhois( std::string destination );
    void flagUndesirableServer( const ServerConfig *sc );
    void serverIsActuallyOkay( const ServerConfig *sc );

  private:
    void connectToServer( ServerConfig *conf, bool reconnect );

                          Network( const std::string &name );
    Server               *activeServer_;
    const NetworkConfig  *config_;
    static std::map<std::string,Network*> networks_;
    std::map<const ServerConfig*,int> undesirables_;
    User                 *me_;
    std::vector<std::string>        identifiedUsers_;
    std::map<std::string,std::vector<std::string> > knownUsers_;
    PluginComm           *plugins_;

    void onFailedConnection();
    void joinedChannel(const std::string &user, Irc::Buffer *b);
    void kickedChannel(const std::string &user, const std::string&, const std::string&, Irc::Buffer *b);
    void partedChannel(const std::string &user, const std::string &, Irc::Buffer *b);
    void slotQuit(const std::string &origin, const std::string&, Irc::Buffer*);
    void slotWhoisReceived(const std::string &origin, const std::string &nick, bool identified, Irc::Buffer *buf);
    void slotNickChanged( const std::string &origin, const std::string &nick, Irc::Buffer *buffer );
    void slotNamesReceived(const std::string&, const std::string&, const std::vector<std::string> &names, Irc::Buffer *buf );
    void slotIrcEvent(const std::string&, const std::string&, const std::vector<std::string>&, Irc::Buffer *buf);
};

#endif
