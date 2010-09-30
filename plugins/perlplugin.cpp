/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#include "perlplugin.h"
#include "perl/embedperl.h"

#include <QtCore/QDebug>
#include <ircclient-qt/IrcBuffer>

PerlPlugin::PerlPlugin()
: Plugin()
, ePerl( new EmbedPerl() )
{}

PerlPlugin::~PerlPlugin() {}

void PerlPlugin::init()
{
  qDebug() << "PerlPlugin initialising.";
  ePerl->loadModule( "DazMessages" );
}

void PerlPlugin::connected( Network &net, const Server &serv )
{
  Q_ASSERT( &serv == net.activeServer() );
}

void PerlPlugin::welcomed( Network &net, const Server &serv )
{
}

void PerlPlugin::joinedChannel( const QString &who, Irc::Buffer *channel )
{
  ePerl->message( "Sjors", "#ru", QString("}hi " + who).toLatin1() );
}

void PerlPlugin::leftChannel( const QString &who, const QString &leaveMessage,
                              Irc::Buffer *channel )
{}

QHash<QString, Plugin::VariableScope> PerlPlugin::variables()
{
  QHash<QString, Plugin::VariableScope> variables;
  return variables;
}
