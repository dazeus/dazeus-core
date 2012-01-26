/**
 * Copyright (c) Sjors Gielen, 2011
 * See LICENSE for license.
 */

#ifndef SOCKETPLUGIN_H
#define SOCKETPLUGIN_H

#include "plugin.h"
#include <QtCore/QVariant>
#include <QtCore/QIODevice>
#include <QtCore/QByteArray>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QLocalServer>

#include <sstream>
#include <libjson.h>

class SocketPlugin : public Plugin
{
  Q_OBJECT

  struct SocketInfo {
   public:
    SocketInfo(QString t = QString()) : type(t), waitingSize(0) {}
    bool isSubscribed(QString t) const {
      return subscriptions.contains(t.toUpper());
    }
    bool unsubscribe(QString t) {
      return subscriptions.removeOne(t.toUpper());
    }
    bool subscribe(QString t) {
      if(isSubscribed(t))
        return false;
      subscriptions.append(t.toUpper());
      return true;
    }
    void dispatch(QIODevice *d, QString event, QStringList parameters) {
      Q_ASSERT(!event.contains(' '));

      JSONNode params(JSON_ARRAY);
      params.set_name("params");
      foreach(const QString &p, parameters) {
        params.push_back(JSONNode("", libjson::to_json_string(p.toLatin1().constData())));
      }

      JSONNode n(JSON_NODE);
      n.push_back(JSONNode("event", libjson::to_json_string(event.toLatin1().constData())));
      n.push_back(params);

      std::string jsonMsg = libjson::to_std_string(n.write());
      std::stringstream mstr;
      mstr << jsonMsg.length();
      mstr << jsonMsg;
      mstr << "\n";
      d->write(mstr.str().c_str(), mstr.str().length());
    }
    QString type;
    QStringList subscriptions;
    int waitingSize;
  };

  public:
            SocketPlugin(PluginManager *man);
  virtual  ~SocketPlugin();
    void dispatch(const QString &event, const QStringList &parameters);

  public slots:
    virtual void init();
    virtual void welcomed( Network &net );
    virtual void connected( Network &net, const Server &serv );
    virtual void disconnected( Network &net );
    virtual void joined( Network &net, const QString &who, Irc::Buffer *channel );
    virtual void parted( Network &net, const QString &who, const QString &leaveMessage,
                         Irc::Buffer *channel );
    virtual void motdReceived( Network &net, const QString &motd, Irc::Buffer *buffer );
    virtual void quit(   Network &net, const QString &origin, const QString &message,
                     Irc::Buffer *buffer );
    virtual void nickChanged( Network &net, const QString &origin, const QString &nick,
                          Irc::Buffer *buffer );
    virtual void modeChanged( Network &net, const QString &origin, const QString &mode,
                          const QString &args, Irc::Buffer *buffer );
    virtual void topicChanged( Network &net, const QString &origin, const QString &topic,
                           Irc::Buffer *buffer );
    virtual void invited( Network &net, const QString &origin, const QString &receiver,
                      const QString &channel, Irc::Buffer *buffer );
    virtual void kicked( Network &net, const QString &origin, const QString &nick,
                     const QString &message, Irc::Buffer *buffer );
    virtual void messageReceived( Network &net, const QString &origin, const QString &message,
                              Irc::Buffer *buffer );
    virtual void noticeReceived( Network &net, const QString &origin, const QString &notice,
                             Irc::Buffer *buffer );
    virtual void ctcpRequestReceived(Network &net, const QString &origin, const QString &request,
                                 Irc::Buffer *buffer );
    virtual void ctcpReplyReceived( Network &net, const QString &origin, const QString &reply,
                                Irc::Buffer *buffer );
    virtual void ctcpActionReceived( Network &net, const QString &origin, const QString &action,
                                 Irc::Buffer *buffer );
    virtual void numericMessageReceived( Network &net, const QString &origin, uint code,
                                     const QStringList &params,
                                     Irc::Buffer *buffer );
    virtual void unknownMessageReceived( Network &net, const QString &origin,
                                       const QStringList &params,
                                       Irc::Buffer *buffer );

  private slots:
    void newTcpConnection();
    void newLocalConnection();
    void poll();

  private:
    QList<QTcpServer*> tcpServers_;
    QList<QLocalServer*> localServers_;
    QMap<QIODevice*,SocketInfo> sockets_;
    void handle(QIODevice *dev, const QByteArray &line, SocketInfo &info);
};

#endif
