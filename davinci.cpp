/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#include <QtCore/QTimer>

#include "davinci.h"
#include "network.h"
#include "config.h"
#include "plugins/pluginmanager.h"

/**
 * @brief Constructor.
 *
 * Initialises the object, and loads configuration if a path was given.
 * @param configFileName optional path to the configuration file.
 */
DaVinci::DaVinci( QString configFileName )
: QObject()
, config_( 0 )
, configFileName_( configFileName )
, pluginManager_( new PluginManager() )
{
  if( !configFileName_.isEmpty() )
    loadConfig();
}


/**
 * @brief Destructor.
 */
DaVinci::~DaVinci()
{
  foreach( Network *n, networks_ )
  {
    n->disconnectFromNetwork( Network::ShutdownReason );
    n->deleteLater();
  }

  resetConfig();
  delete pluginManager_;
}


/**
 * @brief Connect to all networks marked "autoconnect".
 * 
 * Warning: This method is usually called outside the event loop, just after
 * initialisation.
 */
void DaVinci::autoConnect()
{
  foreach( Network *n, networks_ )
  {
    if( n->autoConnectEnabled() )
    {
      n->connectToNetwork();
    }
  }
}


/**
 * @brief Returns whether the configuration is loaded.
 */
bool DaVinci::configLoaded() const
{
  return config_ != 0;
}


/**
 * @brief We received the IRC 'welcomed' message from a server.
 *
 * Give a signal through to the plugin manager.
 */
void DaVinci::welcomed()
{
  Network *n = qobject_cast<Network*>(sender());
  Q_ASSERT( n != 0 );
  pluginManager_->welcomed( *n );
}


/**
 * @brief The application connected.
 *
 * Give a signal through to the plugin manager.
 */
void DaVinci::connected()
{
  Network *n = qobject_cast<Network*>(sender());
  Q_ASSERT( n != 0 );
  const Server *s = n->activeServer();
  Q_ASSERT( s != 0 );
  pluginManager_->connected( *n, *s );
}


/**
 * @brief Initialises plugins from the configuration file.
 */
bool DaVinci::initPlugins()
{
  if( pluginManager_->isInitialized() )
    return true;

  pluginManager_->setConfig( config_ );
  if( !pluginManager_->initialize() ) {
    delete pluginManager_;
    return false;
  }

  return true;
}


/**
 * @brief (Re-)loads configuration from the configuration file.
 *
 * If loading failed, this method returns false.
 * Note: this method is automatically called when setConfigFileName is called,
 * and by the constructor.
 */
bool DaVinci::loadConfig()
{
  // Clean up this object.
  resetConfig();

  Q_ASSERT( !configFileName_.isEmpty() );

  if( config_ == 0 )
    config_ = new Config();

  config_->loadFromFile( configFileName_ );

  if( !config_ )
    return false;

  const QList<NetworkConfig*> &networks = config_->networks();
  foreach( NetworkConfig *netconf, networks )
  {
    Network *net = Network::fromNetworkConfig( netconf );
    if( net == 0 ) {
      resetConfig();
      return false;
    }
    connect( net,  SIGNAL(      welcomed() ),
             this, SLOT(        welcomed() ) );
    connect( net,  SIGNAL(     connected() ),
             this, SLOT(       connected() ) );
    connect( net,            SIGNAL( joined( const QString&, Irc::Buffer* ) ),
             pluginManager_, SLOT(   joinedChannel( const QString&, Irc::Buffer* ) ) );
    networks_.append( net );
  }

  return true;
}

/**
 * @brief Resets everything from the configuration of this object.
 *
 * Destroys the config_ object if it's already there, resets networks, servers,
 * et cetera.
 * This method will not clear the configFileName_. It is always called when
 * loadConfig() is called.
 */
void DaVinci::resetConfig()
{
  //delete config_;
  //config_ = 0;

  //foreach( Network *net, networks_ )
  //{
    //net->disconnect( Network::ConfigurationReloadReason );
    //net->deleteLater();
  //}
  //networks_.clear();
}
