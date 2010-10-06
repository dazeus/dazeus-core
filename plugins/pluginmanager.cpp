/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#include <config.h>
#include "pluginmanager.h"
#include "testplugin.h"
#include "perlplugin.h"

/**
 * @brief Constructor.
 *
 * Does nothing.
 */
PluginManager::PluginManager()
: QObject()
, config_( 0 )
, initialized_( false )
{
}


/**
 * @brief Destructor.
 */
PluginManager::~PluginManager()
{
}


/**
 * @brief Initialises plugins.
 *
 * This method takes all configuration from the configuration file, tries to
 * find all modules and load all modules marked 'load on start' (in order of
 * priority), then calls
 * init() on them. If it fails to load and initialize a plugin, it prints an
 * error and, depending on the value of fatalError in the configfile, either
 * ignores the error or stops loading all plugins altogether and returning
 * an error.
 *
 * When the configuration reloads, this method is automatically called to
 * reload all plugins.
 */
bool PluginManager::initialize()
{
  Q_ASSERT( config_ != 0 );

  Plugin *plugin = new TestPlugin();
  plugin->init();
  plugins_.append( plugin );

  Plugin *plugin2 = new PerlPlugin();
  plugin2->init();
  plugins_.append( plugin2 );

  initialized_ = true;
  return true;
}



/**
 * @returns whether this PluginManager is initialized.
 * @see initialize()
 */
bool PluginManager::isInitialized() const
{
  return initialized_;
}



/**
 * @brief Unloads all modules and resets this object.
 */
void PluginManager::reset()
{
  // TODO
}

/**
 * @brief Set the configuration of this object.
 * MUST be called before initialize().
 */
void PluginManager::setConfig( Config *c )
{
  config_ = c;
  connect( c,    SIGNAL( configReloaded() ),
           this, SLOT(       initialize() ) );
}



/******* ADDITIONAL HANDLERS **********/

/**
 * @brief welcomed() handler.
 *
 * This method is called when DaVinci finishes a handshake with the IRC server.
 */
void PluginManager::welcomed( Network &n )
{
  foreach( Plugin *p, plugins_ )
  {
    p->welcomed( n );
  }
}

/**
 * @brief connected() handler.
 *
 * This method is called when DaVinci connects to an IRC server, before authorization.
 */
void PluginManager::connected( Network &n, const Server &s )
{
  foreach( Plugin *p, plugins_ )
  {
    p->connected( n, s );
  }
}

/**
 * @brief disconnected() handler.
 *
 * This method is called when the connection is broken between DaVinci and an IRC server.
 */
void PluginManager::disconnected( Network &n )
{
  foreach( Plugin *p, plugins_ )
  {
    p->disconnected( n );
  }
}

#define PLUGIN_EVENT_RELAY_1STR( name ) \
void PluginManager::name ( const QString &str, Irc::Buffer *b ) { \
  Network *n = Network::fromBuffer( b ); \
  Q_ASSERT( n != 0 ); \
  foreach( Plugin *p, plugins_ ) \
  { \
    p->name( *n, str, b ); \
  } \
}

#define PLUGIN_EVENT_RELAY_2STR( name ) \
void PluginManager::name ( const QString &str, const QString &str2, Irc::Buffer *b ) { \
  Network *n = Network::fromBuffer( b ); \
  Q_ASSERT( n != 0 ); \
  foreach( Plugin *p, plugins_ ) \
  { \
    p->name( *n, str, str2, b ); \
  } \
}

#define PLUGIN_EVENT_RELAY_3STR( name ) \
void PluginManager::name ( const QString &str, const QString &str2, \
                           const QString &str3, Irc::Buffer *b ) { \
  Network *n = Network::fromBuffer( b ); \
  Q_ASSERT( n != 0 ); \
  foreach( Plugin *p, plugins_ ) \
  { \
    p->name( *n, str, str2, str3, b ); \
  } \
}

PLUGIN_EVENT_RELAY_1STR( motdReceived );
PLUGIN_EVENT_RELAY_1STR( joined );
PLUGIN_EVENT_RELAY_2STR( parted );
PLUGIN_EVENT_RELAY_2STR( quit );
PLUGIN_EVENT_RELAY_2STR( nickChanged );
PLUGIN_EVENT_RELAY_3STR( modeChanged );
PLUGIN_EVENT_RELAY_2STR( topicChanged );
PLUGIN_EVENT_RELAY_3STR( invited );
PLUGIN_EVENT_RELAY_3STR( kicked );
PLUGIN_EVENT_RELAY_2STR( messageReceived );
PLUGIN_EVENT_RELAY_2STR( noticeReceived );
PLUGIN_EVENT_RELAY_2STR( ctcpRequestReceived );
PLUGIN_EVENT_RELAY_2STR( ctcpReplyReceived );
PLUGIN_EVENT_RELAY_2STR( ctcpActionReceived );

#undef PLUGIN_EVENT_RELAY_1STR
#undef PLUGIN_EVENT_RELAY_2STR
#undef PLUGIN_EVENT_RELAY_3STR

void PluginManager::numericMessageReceived( const QString &origin, uint code,
                           const QStringList &params, Irc::Buffer *buffer )
{
  Network *n = Network::fromBuffer( buffer );
  Q_ASSERT( n != 0 );
  foreach( Plugin *p, plugins_ )
  {
    p->numericMessageReceived( *n, origin, code, params, buffer );
  }
}

void PluginManager::unknownMessageReceived( const QString &origin,
                          const QStringList &params, Irc::Buffer *buffer )
{
  Network *n = Network::fromBuffer( buffer );
  Q_ASSERT( n != 0 );
  foreach( Plugin *p, plugins_ )
  {
    p->unknownMessageReceived( *n, origin, params, buffer );
  }
}

