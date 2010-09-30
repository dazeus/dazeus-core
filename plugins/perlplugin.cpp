/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#include "perlplugin.h"
#include "perl/embedperl.h"
#include "../config.h"

#include <QtCore/QDebug>
#include <ircclient-qt/IrcBuffer>

extern "C" {
  void perlplugin_emote_callback( const char *recv, const char *body, void *data )
  {
    PerlPlugin *pp = (PerlPlugin*) data;
    pp->emoteCallback( recv, body );
  }
  void perlplugin_privmsg_callback( const char *recv, const char *body, void *data )
  {
    PerlPlugin *pp = (PerlPlugin*) data;
    pp->privmsgCallback( recv, body );
  }
}

PerlPlugin::PerlPlugin()
: Plugin()
{
}

PerlPlugin::~PerlPlugin() {}

void PerlPlugin::init()
{
  qDebug() << "PerlPlugin initialising.";
}

EmbedPerl *PerlPlugin::getNetworkEmbed( Network &net )
{
  if( ePerl.contains( net.config()->name ) )
    return ePerl.value( net.config()->name );

  EmbedPerl *newPerl = new EmbedPerl( net.config()->name.toLatin1() );
  newPerl->setCallbacks( perlplugin_emote_callback, perlplugin_privmsg_callback, this );
  newPerl->loadModule( "DazMessages" );

  ePerl.insert( net.config()->name, newPerl );
  return newPerl;
}

void PerlPlugin::connected( Network &net, const Server &serv )
{
  Q_ASSERT( &serv == net.activeServer() );
}

void PerlPlugin::welcomed( Network &net, const Server &serv )
{
}

void PerlPlugin::joinedChannel( Network &net, const QString &who, Irc::Buffer *channel )
{
  getNetworkEmbed(net)->message( "Sjors", "#ru", QString("}hi " + who).toLatin1() );
}

void PerlPlugin::leftChannel( Network &net, const QString &who, const QString &leaveMessage,
                              Irc::Buffer *channel )
{
  //getNetworkEmbed(net)
}

QHash<QString, Plugin::VariableScope> PerlPlugin::variables()
{
  QHash<QString, Plugin::VariableScope> variables;
  return variables;
}

void PerlPlugin::emoteCallback( const char *receiver, const char *body )
{
  //emote( receiver, body );
}

void PerlPlugin::privmsgCallback( const char *receiver, const char *body )
{
  //privmsg( receiver, body );
}
