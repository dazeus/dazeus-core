/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#include <config.h>
#include "pluginmanager.h"
#include "testplugin.h"
#include "perlplugin.h"
#include "statistics.h"
#include <IrcBuffer>
#include <IrcSession>

QDebug operator<<(QDebug dbg, const Context *c)
{
  dbg.nospace() << "Context[";
  if( c == 0 ) {
    dbg.nospace() << "0";
  } else {
    dbg.nospace() <<  "n=" << c->network
                  << ",r=" << c->receiver
                  << ",s=" << c->sender;
  }
  dbg.nospace() << "]";

  return dbg.maybeSpace();
}

/**
 * @brief Constructor.
 *
 * Does nothing.
 */
PluginManager::PluginManager( Database *db )
: QObject()
, config_( 0 )
, database_( db )
, context_( 0 )
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
 * @brief Clear the previously set context.
 * @see setContext(), context()
 */
void PluginManager::clearContext()
{
  Q_ASSERT( context_ != 0 );
  delete context_;
  context_ = 0;
}

/**
 * @brief Return the current call context.
 */
const Context *PluginManager::context() const
{
  return context_;
}

/**
 * @brief Gets a property.
 * Name must be fully qualified. The nearest scope will be returned in s if
 * non-null.
 */
QVariant PluginManager::get( const QString &name, Plugin::VariableScope *s  ) const
{
  if( s != NULL )
    *s = Plugin::GlobalScope;
  return QVariant();
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

  Plugin *plugin3 = new Statistics();
  plugin3->init();
  plugins_.append( plugin3 );

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
 * @brief Sets a property in the database on given scope.
 */
void PluginManager::set( Plugin::VariableScope s, const QString &name, const QVariant &value )
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

/**
 * @brief Set the context of this call.
 *
 * All PluginManager handlers call this method for later reference. For
 * example, the Database needs to look back at the current call, to save
 * variables in the correct scope.
 */
void PluginManager::setContext(QString network, QString receiver, QString sender)
{
  Q_ASSERT( context_ == 0 );
  context_ = new Context;
  context_->network  = network;
  context_->receiver = receiver;
  context_->sender   = sender;
}



/******* ADDITIONAL HANDLERS **********/

/**
 * @brief welcomed() handler.
 *
 * This method is called when DaVinci finishes a handshake with the IRC server.
 */
void PluginManager::welcomed( Network &n )
{
  setContext( n.config()->name );
  foreach( Plugin *p, plugins_ )
  {
    p->welcomed( n );
  }
  clearContext();
}

/**
 * @brief connected() handler.
 *
 * This method is called when DaVinci connects to an IRC server, before authorization.
 */
void PluginManager::connected( Network &n, const Server &s )
{
  setContext( n.config()->name );
  foreach( Plugin *p, plugins_ )
  {
    p->connected( n, s );
  }
  clearContext();
}

/**
 * @brief Database getter.
 *
 * Returns the database for this PluginManager and set of plugins.
 */
Database *PluginManager::database() const
{
  return database_;
}

/**
 * @brief disconnected() handler.
 *
 * This method is called when the connection is broken between DaVinci and an IRC server.
 */
void PluginManager::disconnected( Network &n )
{
  setContext( n.config()->name );
  foreach( Plugin *p, plugins_ )
  {
    p->disconnected( n );
  }
  clearContext();
}

void PluginManager::motdReceived ( const QString &motd, Irc::Buffer *b ) {
  Network *n = Network::fromBuffer( b );
  Q_ASSERT( n != 0 );
  setContext( n->config()->name );
  foreach( Plugin *p, plugins_ )
  {
    p->motdReceived( *n, motd, b );
  }
  clearContext();
}

#define PLUGIN_EVENT_RELAY_1STR( EVENTNAME ) \
void PluginManager::EVENTNAME ( const QString &str, Irc::Buffer *b ) { \
  Network *n = Network::fromBuffer( b ); \
  Q_ASSERT( n != 0 ); \
  setContext( n->config()->name, b->receiver(), str ); \
  foreach( Plugin *p, plugins_ ) \
  { \
    p->EVENTNAME( *n, str, b ); \
  } \
  clearContext(); \
}

#define PLUGIN_EVENT_RELAY_2STR( EVENTNAME ) \
void PluginManager::EVENTNAME ( const QString &str, const QString &str2, Irc::Buffer *b ) { \
  Network *n = Network::fromBuffer( b ); \
  Q_ASSERT( n != 0 ); \
  setContext( n->config()->name, b->receiver(), str ); \
  foreach( Plugin *p, plugins_ ) \
  { \
    p->EVENTNAME( *n, str, str2, b ); \
  } \
  clearContext(); \
}

#define PLUGIN_EVENT_RELAY_3STR( EVENTNAME ) \
void PluginManager::EVENTNAME ( const QString &str, const QString &str2, \
                           const QString &str3, Irc::Buffer *b ) { \
  Network *n = Network::fromBuffer( b ); \
  Q_ASSERT( n != 0 ); \
  setContext( n->config()->name, b->receiver(), str ); \
  foreach( Plugin *p, plugins_ ) \
  { \
    p->EVENTNAME( *n, str, str2, str3, b ); \
  } \
  clearContext(); \
}

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
  setContext( n->config()->name );
  foreach( Plugin *p, plugins_ )
  {
    p->numericMessageReceived( *n, origin, code, params, buffer );
  }
  clearContext();
}

void PluginManager::unknownMessageReceived( const QString &origin,
                          const QStringList &params, Irc::Buffer *buffer )
{
  Network *n = Network::fromBuffer( buffer );
  Q_ASSERT( n != 0 );
  setContext( n->config()->name );
  foreach( Plugin *p, plugins_ )
  {
    p->unknownMessageReceived( *n, origin, params, buffer );
  }
  clearContext();
}

