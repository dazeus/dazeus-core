/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#include "plugin.h"

Plugin::Plugin()
: QObject()
{}

Plugin::~Plugin()
{}


void Plugin::set( const QString &name, const QVariant &value )
{
}

QVariant Plugin::get( const QString &name ) const
{
  return QVariant();
}


/*******STUB HANDLERS********/
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
