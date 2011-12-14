/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#include <QtCore/QDebug>

#include "testplugin.h"
#include "../server.h"

TestPlugin::TestPlugin( PluginManager *man )
: Plugin( "TestPlugin", man )
{}

TestPlugin::~TestPlugin() {}

void TestPlugin::init()
{
  qDebug() << "TestPlugin initialising.";
}

void TestPlugin::connected( Network &net, const Server &serv )
{
  qDebug() << "Connected to network" << net;
  if( &serv != net.activeServer() )
  {
    qWarning() << "Warning! Connected to different server" << serv;
  }
}

void TestPlugin::welcomed( Network &net )
{
  qDebug() << "Authenticated to network " << net;
}

void TestPlugin::joined( Network &net, const QString &who, Irc::Buffer *channel )
{
  User u( who, &net );
  qDebug() << "User " << u << " joined channel " << channel;
  /* if( u.isMe() )
  {
    channel->message( "Hi all!" );
  }
  else
  {
    channel->message( "Hi " + u.nick() + "!" );
  } */
}

void TestPlugin::parted( Network &net, const QString &who, const QString &leaveMessage,
                              Irc::Buffer *channel )
{
  qDebug() << "User " << who << " left channel " << channel << "on" << net
           << " (leave message " << leaveMessage << ")";
}
