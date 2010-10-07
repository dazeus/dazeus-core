/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#ifndef PLUGIN_H
#define PLUGIN_H

#include <QtCore/QList>
#include <QtCore/QHash>
#include <QtCore/QString>
#include <QtCore/QVariant>

#include "user.h"
#include "network.h"
#include "server.h"

class Plugin : public QObject
{
  Q_OBJECT

  public:
            Plugin();
  virtual  ~Plugin();

  /**
   * @brief Decides what a variable binds to.
   *
   * Plugins can define what variables they want to store in the database. By
   * giving a VariableScope, they can set scopes on the variable. For example,
   * a plugin might want to store some variable in one channel only, and that
   * variable needs to have a different value in a different channel on the
   * same network.
   *
   * User scope means that the variable binds to that user specifically; these
   * variables can only be stored and retrieved in slots that were called upon
   * action of a user on the network. For example, it works in
   * #messageReceived(), but it doesn't work in #connected(). In the case of,
   * for example, #userKicked(), the "active user" is the user who kicked
   * another user. There is currently no way to switch active user to the
   * victim.
   */
  enum VariableScope {
    /// The variable binds to this network only.
    NetworkScope = 0x1,
    /// The variable binds to one user specifically, but can bind to that user
    /// across the network.
    UserScope = NetworkScope | 0x2,
    /// The variable binds to the channel and network specifically.
    ChannelScope = NetworkScope | 0x4,
    /// The variable doesn't bind and is the same everywhere.
    EverywhereScope = 0x8,
  };

  public slots:
    virtual void init() = 0;
    virtual void welcomed( Network &net ) = 0;
    virtual void connected( Network &net, const Server &serv ) = 0;
    virtual void disconnected( Network &net );
    virtual void joined( Network &net, const QString &who, Irc::Buffer *channel ) = 0;
    virtual void parted( Network &net, const QString &who, const QString &leaveMessage,
                         Irc::Buffer *channel ) = 0;

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

  protected slots:
    virtual QHash<QString, VariableScope> variables() = 0;

    void     set( const QString &name, const QVariant &value );
    QVariant get( const QString &name ) const;

  private:
    User                          *activeUser_;
    QHash<QString, VariableScope>  variables_;
};

#endif
