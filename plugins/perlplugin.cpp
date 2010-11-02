/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#include "perlplugin.h"
#include "perl/embedperl.h"
#include "../config.h"
#include "pluginmanager.h"

#include <QtCore/QDebug>
#include <ircclient-qt/IrcBuffer>

// #define DEBUG

extern "C" {
  void perlplugin_emote_callback( const char *net, const char *recv, const char *body, void *data )
  {
    PerlPlugin *pp = (PerlPlugin*) data;
    pp->emoteCallback( net, recv, body );
  }
  void perlplugin_privmsg_callback( const char *net, const char *recv, const char *body, void *data )
  {
    PerlPlugin *pp = (PerlPlugin*) data;
    pp->privmsgCallback( net, recv, body );
  }
  const char* perlplugin_getProperty_callback( const char *net, const char *variable, void *data)
  {
    PerlPlugin *pp = (PerlPlugin*) data;
    return pp->getPropertyCallback(net, variable);
  }
  void perlplugin_setProperty_callback( const char *net, const char *variable, const char *value, void *data)
  {
    PerlPlugin *pp = (PerlPlugin*) data;
    pp->setPropertyCallback(net, variable, value);
  }
  void perlplugin_unsetProperty_callback( const char *net, const char *variable, void *data)
  {
    PerlPlugin *pp = (PerlPlugin*) data;
    pp->unsetPropertyCallback(net, variable);
  }
  void perlplugin_sendWhois_callback(const char *who, void *data)
  {
    PerlPlugin *pp = (PerlPlugin*) data;
    pp->sendWhoisCallback(who);
  }
  void perlplugin_join_callback(const char *channel, void *data)
  {
    PerlPlugin *pp = (PerlPlugin*) data;
    pp->joinCallback(channel);
  }
  void perlplugin_part_callback(const char *channel, void *data)
  {
    PerlPlugin *pp = (PerlPlugin*) data;
    pp->partCallback(channel);
  }
  const char*perlplugin_getNick_callback( void *data )
  {
    PerlPlugin *pp = (PerlPlugin*) data;
    return pp->getNickCallback();
  }
}

PerlPlugin::PerlPlugin( PluginManager *man )
: Plugin(man)
, whois_identified(false)
{
  tickTimer_.setSingleShot( false );
  tickTimer_.setInterval( 5000 );
  connect( &tickTimer_, SIGNAL( timeout() ),
           this,        SLOT(      tick() ) );
}

PerlPlugin::~PerlPlugin() {}

void PerlPlugin::init()
{
#ifdef DEBUG
  qDebug() << "PerlPlugin initialising.";
#endif
  tickTimer_.start();
}

EmbedPerl *PerlPlugin::getNetworkEmbed( Network &net )
{
  if( ePerl.contains( net.config()->name ) )
    return ePerl.value( net.config()->name );

  EmbedPerl *newPerl = new EmbedPerl( net.config()->name.toLatin1() );
  newPerl->setCallbacks( perlplugin_emote_callback, perlplugin_privmsg_callback,
                         perlplugin_getProperty_callback, perlplugin_setProperty_callback,
                         perlplugin_unsetProperty_callback, perlplugin_sendWhois_callback,
                         perlplugin_join_callback, perlplugin_part_callback,
                         perlplugin_getNick_callback, this );
  newPerl->loadModule( "DazAuth" );
  newPerl->loadModule( "DazChannel" );
  newPerl->loadModule( "DazLoader" );
  newPerl->loadModule( "DazMessages" );
  newPerl->loadModule( "DazFactoids" );

  ePerl.insert( net.config()->name, newPerl );
  return newPerl;
}

void PerlPlugin::messageReceived( Network &net, const QString &origin, const QString &message,
                                  Irc::Buffer *buffer )
{
  getNetworkEmbed(net)->message( origin.toLatin1(), buffer->receiver().toLatin1(), message.toUtf8() );
}

void PerlPlugin::numericMessageReceived( Network &net, const QString &origin, uint code,
                                     const QStringList &args, Irc::Buffer *buf )
{
  if( code == 311 )
  {
    in_whois = args[1];
    Q_ASSERT( !whois_identified );
  }
#warning This will not work - should use CAP IDENTIFY_MSG for this:
  else if( code == 307 || code == 330 ) // 330 means "logged in as", but doesn't check whether nick is grouped.
  {
    whois_identified = true;
  }
  else if( code == 318 )
  {
    getNetworkEmbed(net)->whois( in_whois.toLatin1().constData(), whois_identified ? 1 : 0 );
    whois_identified = false;
    in_whois.clear();
  }
  // part of NAMES
  else if( code == 353 )
  {
    names_ += " " + args.last();
  }
  else if( code == 366 )
  {
    getNetworkEmbed(net)->namesReceived( args.at(1).toLatin1().constData(), names_.toLatin1().constData() );
    names_.clear();
  }
}

void PerlPlugin::connected( Network &net, const Server & ) {
  getNetworkEmbed(net)->connected();
}

void PerlPlugin::joined( Network &net, const QString &who, Irc::Buffer *channel ) {
  getNetworkEmbed(net)->join( channel->receiver().toLatin1(), who.toLatin1() );
}

void PerlPlugin::nickChanged( Network &net, const QString &origin, const QString &nick,
                          Irc::Buffer* ) {
  getNetworkEmbed(net)->nick( origin.toLatin1(), nick.toLatin1() );
}

void PerlPlugin::tick() {
  // tick every NetworkEmbed
  foreach(EmbedPerl *e, ePerl)
  {
    e->tick();
  }
}

void PerlPlugin::emoteCallback( const char *network, const char *receiver, const char *body )
{
  Network *net = Network::getNetwork( network );
  Q_ASSERT( net != 0 );
  net->action( receiver, body );
}

void PerlPlugin::privmsgCallback( const char *network, const char *receiver, const char *body )
{
  Network *net = Network::getNetwork( network );
  Q_ASSERT( net != 0 );
  net->say( receiver, body );
}

const char *PerlPlugin::getPropertyCallback(const char *network, const char *variable)
{
  Q_UNUSED(network);

  QVariant value = get(variable);
#ifdef DEBUG
  qDebug() << "Get property: " << variable << "=" << value;
#endif
  if( value.isNull() )
    return 0;
  // save it here until the next call, it will be copied later
  propertyCopy_ = value.toString().toUtf8();
  return propertyCopy_.constData();
}

void PerlPlugin::setPropertyCallback(const char *network, const char *variable, const char *value)
{
  Q_UNUSED(network);

  set(Plugin::GlobalScope, variable, value);
#ifdef DEBUG
  qDebug() << "Set property: " << variable << "to:" << value;
#endif
}

void PerlPlugin::unsetPropertyCallback(const char *network, const char *variable)
{
  Q_UNUSED(network);

  set(Plugin::GlobalScope, variable, QVariant());
#ifdef DEBUG
  qDebug() << "Unset property: " << variable;
#endif
}

const char *PerlPlugin::getNickCallback()
{
  Network *net = Network::getNetwork( manager()->context()->network );
  Q_ASSERT( net != 0 );
  return net->user()->nick().toUtf8().constData();
}

void PerlPlugin::sendWhoisCallback(const char *who)
{
  Network *net = Network::getNetwork( manager()->context()->network );
  Q_ASSERT( net != 0 );
  net->sendWhois( who );
}

void PerlPlugin::joinCallback(const char *channel)
{
  Network *net = Network::getNetwork( manager()->context()->network );
  Q_ASSERT( net != 0 );
  net->joinChannel( channel );
}

void PerlPlugin::partCallback(const char *channel)
{
  Network *net = Network::getNetwork( manager()->context()->network );
  Q_ASSERT( net != 0 );
  net->leaveChannel( channel );
}

