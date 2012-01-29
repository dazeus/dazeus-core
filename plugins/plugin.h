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
#include <QtCore/QStringList>

#include "user.h"
#include "network.h"
#include "server.h"

class PluginManager;

class Plugin : public QObject
{
  Q_OBJECT

  public:
            Plugin(const QString &name, PluginManager *man);
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
    /// The variable doesn't bind and is the same everywhere.
    GlobalScope = 0x0,
    /// The variable binds to this network only.
    NetworkScope = 0x1,
    /// The variable binds to a specific receiver (for example, a channel) on
    /// a specific network
    ReceiverScope = 0x2,
    /// The variable binds to a network, a receiver, and the sender.
    SenderScope = 0x4,
  };

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
    virtual void whoisReceived( Network &net, const QString &origin, const QString &nick,
                                     bool identified, Irc::Buffer *buffer );
    virtual void namesReceived( Network &net, const QString &origin, const QString &channel,
                                     const QStringList &params, Irc::Buffer *buffer );

  protected slots:
    virtual QHash<QString, VariableScope> variables();

    void     set( VariableScope s, const QString &name, const QVariant &value );
    QVariant get( const QString &name, VariableScope *s = NULL ) const;
    QStringList keys( const QString &ns ) const;
    QVariant getConfig( const QString &name ) const;
    PluginManager *manager() const;
    void    setContext(QString network, QString receiver = QString(), QString sender = QString());
    void    clearContext();

  private:
    User                          *activeUser_;
    PluginManager                 *manager_;
    QHash<QString, VariableScope>  variables_;
    QString                        name_;
};

#endif
