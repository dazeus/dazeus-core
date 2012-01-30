/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#include "plugin.h"
#include "pluginmanager.h"

Plugin::Plugin(const QString &name, PluginManager *man)
: QObject()
, manager_(man)
, name_(name)
{}

Plugin::~Plugin()
{}


void Plugin::set( VariableScope scope, const QString &name, const QVariant &value )
{
  if( !name.contains(QLatin1Char('.')) )
  {
    qWarning() << "Warning: Variable contains no dot, is it qualified?" << name;
  }
  manager_->set( scope, name, value );
}

QVariant Plugin::get( const QString &name, VariableScope *scope ) const
{
  return manager_->get(name, scope);
}

QStringList Plugin::keys( const QString &ns ) const
{
  return manager_->keys(ns);
}

QVariant Plugin::getConfig( const QString &name, const QString &group ) const
{
  QString g = group;
  if(group.isEmpty())
    g = name_;
  return manager_->getConfig(g, name);
}


PluginManager *Plugin::manager() const
{
  return manager_;
}

void Plugin::setContext(QString network, QString receiver, QString sender)
{
  manager_->setContext( network, receiver, sender );
}

void Plugin::clearContext()
{
  manager_->clearContext();
}

/*******STUB HANDLERS********/
void Plugin::init() {}
void Plugin::welcomed( Network &net ) {}
void Plugin::connected( Network &net, const Server &serv ) {}
void Plugin::joined( Network &net, const QString &who, Irc::Buffer *channel ) {}
void Plugin::parted( Network &net, const QString &who, const QString &leaveMessage,
                         Irc::Buffer *channel ) {}
QHash<QString, Plugin::VariableScope> Plugin::variables() {
  return QHash<QString,Plugin::VariableScope>();
}
void Plugin::disconnected( Network& ) {}
void Plugin::motdReceived( Network&, const QString&, Irc::Buffer* ) {}
void Plugin::quit(   Network&, const QString&, const QString&,
                     Irc::Buffer* ) {}
void Plugin::nickChanged( Network&, const QString&, const QString&,
                          Irc::Buffer* ) {}
void Plugin::modeChanged( Network&, const QString&, const QString&,
                          const QString &, Irc::Buffer* ) {}
void Plugin::topicChanged( Network&, const QString&, const QString&,
                           Irc::Buffer* ) {}
void Plugin::invited( Network&, const QString&, const QString&,
                      const QString&, Irc::Buffer* ) {}
void Plugin::kicked( Network&, const QString&, const QString&,
                     const QString&, Irc::Buffer* ) {}
void Plugin::messageReceived( Network&, const QString&, const QString&,
                              Irc::Buffer* ) {}
void Plugin::noticeReceived( Network&, const QString&, const QString&,
                             Irc::Buffer* ) {}
void Plugin::ctcpRequestReceived(Network&, const QString&, const QString&,
                                 Irc::Buffer* ) {}
void Plugin::ctcpReplyReceived( Network&, const QString&, const QString&,
                                Irc::Buffer* ) {}
void Plugin::ctcpActionReceived( Network&, const QString&, const QString&,
                                 Irc::Buffer* ) {}
void Plugin::numericMessageReceived( Network&, const QString&, uint,
                                     const QStringList&,
                                     Irc::Buffer* ) {}
void Plugin::unknownMessageReceived( Network&, const QString&,
                                     const QStringList&,
                                     Irc::Buffer* ) {}
void Plugin::whoisReceived( Network&, const QString &origin, const QString &nick,
                                 bool identified, Irc::Buffer *buffer ) {}
void Plugin::namesReceived( Network&, const QString &origin, const QString &channel,
                                 const QStringList &params, Irc::Buffer *buffer ) {}
