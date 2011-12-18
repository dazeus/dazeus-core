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
  const char** perlplugin_getPropertyKeys_callback( const char *net, const char *ns, int *length, void *data)
  {
    PerlPlugin *pp = (PerlPlugin*) data;
    return pp->getPropertyKeysCallback(net, ns, length);
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
: Plugin( "PerlPlugin", man )
, cKeys_(0)
, cKeysLength_(0)
, whois_identified(false)
{
  tickTimer_.setSingleShot( false );
  tickTimer_.setInterval( 1000 );
  connect( &tickTimer_, SIGNAL( timeout() ),
           this,        SLOT(      tick() ) );

  ePerl = new EmbedPerl();
  if(!ePerl->isInitialized()) {
    qWarning() << "Failed to initialize Perl! Quitting.\n";
    exit(1);
  }
  ePerl->setCallbacks( perlplugin_emote_callback, perlplugin_privmsg_callback,
                       perlplugin_getProperty_callback, perlplugin_setProperty_callback,
                       perlplugin_unsetProperty_callback, perlplugin_getPropertyKeys_callback, perlplugin_sendWhois_callback,
                       perlplugin_join_callback, perlplugin_part_callback,
                       perlplugin_getNick_callback, this );
}

PerlPlugin::~PerlPlugin()
{
  if(cKeys_ != 0) {
    for(int i = 0; i < cKeysLength_; ++i) {
      delete [] cKeys_[i];
    }
    delete [] cKeys_;
  }
}

void PerlPlugin::init()
{
#ifdef DEBUG
  qDebug() << "PerlPlugin initialising.";
#endif
  tickTimer_.start();
}

void PerlPlugin::initNetwork(QString uniqueId)
{
  Q_ASSERT(!uniqueIds_.contains(uniqueId));
  uniqueIds_.append(uniqueId);
  ePerl->init(uniqueId.toLatin1().constData());

  int modules = getConfig("modules").toInt();
  for(int i = 1; i <= modules; ++i) {
    QString module = getConfig("module" + QString::number(i)).toString();
    ePerl->loadModule( uniqueId.toLatin1(), module.toLatin1() );
  }
}

void PerlPlugin::messageReceived( Network &net, const QString &origin, const QString &message,
                                  Irc::Buffer *buffer )
{
  ePerl->message( net.networkName().toLatin1(), origin.toLatin1(), buffer->receiver().toLatin1(), message.toUtf8() );
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
    ePerl->whois( net.networkName().toLatin1(), in_whois.toLatin1(), whois_identified ? 1 : 0 );
    whois_identified = false;
    in_whois.clear();
  }
  // part of NAMES
  else if( code == 353 )
  {
    names_ += QLatin1Char(' ') + args.last();
  }
  else if( code == 366 )
  {
    ePerl->namesReceived( net.networkName().toLatin1(), args.at(1).toLatin1(), names_.toLatin1() );
    names_.clear();
  }
}

void PerlPlugin::connected( Network &net, const Server & ) {
  initNetwork(net.networkName());
  ePerl->connected(net.networkName().toLatin1());
}

void PerlPlugin::joined( Network &net, const QString &who, Irc::Buffer *channel ) {
  ePerl->join( net.networkName().toLatin1(), channel->receiver().toLatin1(), who.toLatin1() );
}

void PerlPlugin::nickChanged( Network &net, const QString &origin, const QString &nick,
                          Irc::Buffer* ) {
  ePerl->nick( net.networkName().toLatin1(), origin.toLatin1(), nick.toLatin1() );
}

void PerlPlugin::tick() {
  for(int i = 0; i < uniqueIds_.size(); ++i)
  {
    setContext(uniqueIds_[i]);
    ePerl->tick(uniqueIds_[i].toLatin1());
    clearContext();
  }
}

// Read as a Perl string (assume it's utf8)
#define ps(x) QString::fromUtf8(x)

void PerlPlugin::emoteCallback( const char *network, const char *receiver, const char *body )
{
  Network *net = Network::getNetwork( ps(network) );
  Q_ASSERT( net != 0 );
  net->action( ps(receiver), ps(body) );
}

void PerlPlugin::privmsgCallback( const char *network, const char *receiver, const char *body )
{
  Network *net = Network::getNetwork( ps(network) );
  Q_ASSERT( net != 0 );
  net->say( ps(receiver), ps(body) );
}

const char *PerlPlugin::getPropertyCallback(const char *network, const char *variable)
{
  Q_UNUSED(network);

  QVariant value = get(ps(variable));
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

  set(Plugin::NetworkScope, ps(variable), ps(value));
#ifdef DEBUG
  qDebug() << "Set property: " << variable << "to:" << value;
#endif
}

void PerlPlugin::unsetPropertyCallback(const char *network, const char *variable)
{
  Q_UNUSED(network);

  set(Plugin::NetworkScope, ps(variable), QVariant());
#ifdef DEBUG
  qDebug() << "Unset property: " << variable;
#endif
}

const char **PerlPlugin::getPropertyKeysCallback(const char *network, const char *ns, int *length) {
  Q_UNUSED(network);

  // Property keys are stored in this object, so they can be copied to SV
  // safely later on without leaking memory and without reading freed memory
  if(cKeys_ != 0) {
    for(int k = 0; k < cKeysLength_; ++k) {
       delete [] cKeys_[k];
    }
    delete [] cKeys_;
    cKeys_ = 0;
    cKeysLength_ = 0;
  }

  QStringList qKeys = keys(ps(ns));

  *length = cKeysLength_ = qKeys.length();
  cKeys_ = new const char * [cKeysLength_];

  for(int i = 0; i < *length; ++i) {
    QByteArray val = qKeys.at(i).toUtf8();
    char *str = new char [val.length() + 1];
    memcpy(str, val.constData(), val.length());
    str[val.length()] = '\0';

    cKeys_[i] = str;
  }

#ifdef DEBUG
  qDebug() << "In getPropertyKeys, retrieved keys starting with " << ns << ":";
  for(int j = 0; j < *length; ++j) {
    qDebug() << "keys[" << j << "]: " << cKeys_[j];
  }
#endif
  return cKeys_;
}

const char *PerlPlugin::getNickCallback()
{
  Network *net = Network::getNetwork( manager()->context()->network );
  Q_ASSERT( net != 0 );
  propertyCopy_ = net->user()->nick().toUtf8();
  return propertyCopy_.constData();
}

void PerlPlugin::sendWhoisCallback(const char *who)
{
  Network *net = Network::getNetwork( manager()->context()->network );
  Q_ASSERT( net != 0 );
  net->sendWhois( ps(who) );
}

void PerlPlugin::joinCallback(const char *channel)
{
  Network *net = Network::getNetwork( manager()->context()->network );
  Q_ASSERT( net != 0 );
  net->joinChannel( ps(channel) );
}

void PerlPlugin::partCallback(const char *channel)
{
  Network *net = Network::getNetwork( manager()->context()->network );
  Q_ASSERT( net != 0 );
  net->leaveChannel( ps(channel) );
}

