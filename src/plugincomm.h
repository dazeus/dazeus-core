/**
 * Copyright (c) Sjors Gielen, 2011
 * See LICENSE for license.
 */

#ifndef SOCKETPLUGIN_H
#define SOCKETPLUGIN_H

#include <sstream>
#include <utility>
#include <map>
#include <libjson.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include "utils.h"
#include "../contrib/libdazeus-irc/src/utils.h"
#include "network.h"

class Database;
class Config;
class DaZeus;

class PluginComm : public NetworkListener
{

  struct Command {
    Network &network;
    std::string origin;
    std::string channel;
    std::string command;
    std::string fullArgs;
    std::vector<std::string> args;
    bool whoisSent;
    Command(Network &n) : network(n), origin(), channel(),
    command(), fullArgs(), args(), whoisSent(false) {}
  };

  struct RequirementInfo {
    bool needsNetwork;
    bool needsReceiver;
    bool needsSender;
    Network *wantedNetwork;
    std::string wantedReceiver;
    std::string wantedSender;
    // Constructor which allows anything
    RequirementInfo() : needsNetwork(false), needsReceiver(false),
        needsSender(false), wantedNetwork(0), wantedReceiver(),
        wantedSender() {}
    // Constructor which allows anything on a network
    RequirementInfo(Network *n) : needsNetwork(true), needsReceiver(false),
        needsSender(false), wantedNetwork(n), wantedReceiver(),
        wantedSender() {}
    // Constructor which allows anything from a sender (isSender=true)
    // or to some receiver (isSender=false)
    RequirementInfo(Network *n, std::string obj, bool isSender) :
        needsNetwork(true), needsReceiver(false),
        needsSender(false), wantedNetwork(n), wantedReceiver(),
        wantedSender()
    {
        if(isSender) {
            needsSender = true; wantedSender = obj;
        } else {
            needsReceiver = true; wantedReceiver = obj;
        }
    }
    private:
    // explicitly disable copy constructor
    RequirementInfo(const RequirementInfo&);
    void operator=(const RequirementInfo&);
  };

  struct SocketInfo {
   public:
    SocketInfo(std::string t = std::string()) : type(t),
      subscriptions(), commands(), waitingSize(0), readahead() {}
    bool isSubscribed(std::string t) const {
      return contains(subscriptions, strToUpper(t));
    }
    bool unsubscribe(std::string t) {
      if(!isSubscribed(t)) return false;
      erase(subscriptions, strToUpper(t));
      return true;
    }
    bool subscribe(std::string t) {
      if(isSubscribed(t))
        return false;
      subscriptions.push_back(strToUpper(t));
      return true;
    }
    bool isSubscribedToCommand(const std::string &cmd, const std::string &recv,
        const std::string &sender, bool identified, const Network &network)
    {
        std::multimap<std::string,RequirementInfo*>::iterator it;
        for(it = commands.begin(); it != commands.end(); ++it) {
            if(it->first != cmd) {
                continue;
            } else if(it->second->needsNetwork && it->second->wantedNetwork != &network) {
                continue;
            } else if(it->second->needsReceiver && it->second->wantedReceiver != recv) {
                continue;
            } else if(it->second->needsSender) {
                if(!identified || it->second->wantedSender != sender)
                    continue;
            }
            return true;
        }
        return false;
    }
    bool commandMightNeedWhois(const std::string &cmd) {
        std::multimap<std::string,RequirementInfo*>::iterator it;
        for(it = commands.begin(); it != commands.end(); ++it) {
            if(it->first != cmd) {
                continue;
            } else if(it->second->needsSender) {
                return true;
            }
        }
        return false;
    }
    void subscribeToCommand(const std::string &cmd, RequirementInfo *info) {
        commands.insert(std::make_pair(cmd, info));
    }
    void dispatch(int d, std::string event, std::vector<std::string> parameters) {
      assert(!contains(event, ' '));

      JSONNode params(JSON_ARRAY);
      params.set_name("params");
      std::vector<std::string>::iterator it;
      for(it = parameters.begin(); it != parameters.end(); ++it) {
        params.push_back(JSONNode("", libjson::to_json_string(it->c_str())));
      }

      JSONNode n(JSON_NODE);
      n.push_back(JSONNode("event", libjson::to_json_string(event.c_str())));
      n.push_back(params);

      std::string jsonMsg = fix_unicode_in_json(
          libjson::to_std_string(n.write()));
      std::stringstream mstr;
      mstr << jsonMsg.length();
      mstr << jsonMsg;
      mstr << "\n";
      if(write(d, mstr.str().c_str(), mstr.str().length()) != (unsigned)mstr.str().length()) {
        fprintf(stderr, "Failed to write correct number of JSON bytes to client socket in dispatch().\n");
        close(d);
      }
    }
    std::string type;
    std::vector<std::string> subscriptions;
    std::multimap<std::string,RequirementInfo*> commands;
    int waitingSize;
    std::string readahead;
  };

  public:
            PluginComm( Database *d, Config *c, DaZeus *bot );
  virtual  ~PluginComm();
  void dispatch(const std::string &event, const std::vector<std::string> &parameters);
  void init();
  void ircEvent(const std::string &event, const std::string &origin,
                const std::vector<std::string> &params, Network *n );
  void run();

  private:
    // explicitly disable copy constructor
    PluginComm(const PluginComm&);
    void operator=(const PluginComm&);

    void newTcpConnection();
    void newLocalConnection();
    void poll();
    void messageReceived(const std::string &origin, const std::string &message, const std::string &receiver, Network *n);

    std::vector<int> tcpServers_;
    std::vector<int> localServers_;
    std::vector<Command*> commandQueue_;
    std::map<int,SocketInfo> sockets_;
    Database *database_;
    Config *config_;
    DaZeus *dazeus_;
    void handle(int dev, const std::string &line, SocketInfo &info);
    void flushCommandQueue(const std::string &nick = std::string(), bool identified = false);
};

#endif
