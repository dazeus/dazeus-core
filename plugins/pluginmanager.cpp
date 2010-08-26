/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#include <config.h>
#include "pluginmanager.h"
#include "testplugin.h"

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
 * @brief authenticated() handler.
 *
 * This method is called when DaVinci is authenticated to an IRC server, ie
 * when handshake is completed.
 */
void PluginManager::authenticated( Network &n, const Server &s )
{
  foreach( Plugin *p, plugins_ )
  {
    p->authenticated( n, s );
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
 * @brief Joined channel handler.
 *
 * This method is called when someone (including ourselves) joins a channel.
 */
void PluginManager::joinedChannel( const QString &w, Irc::Buffer *b )
{
  foreach( Plugin *p, plugins_ )
  {
    p->joinedChannel( w, b );
  }
}


/**
 * @brief Left channel handler.
 *
 * This method is called when someone (including ourselves) leaves a channel.
 */
void PluginManager::leftChannel( const QString &w, const QString &message, Irc::Buffer *b )
{
  foreach( Plugin *p, plugins_ )
  {
    p->leftChannel( w, message, b );
  }
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
