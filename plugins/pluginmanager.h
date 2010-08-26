/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <QtCore/QObject>

class Config;
class Plugin;
class User;
class Network;
class Server;

namespace Irc {
  class Buffer;
};

class PluginManager : public QObject
{
  Q_OBJECT

  public:
            PluginManager();
           ~PluginManager();
    bool    isInitialized() const;

  public slots:
    void    setConfig( Config* );
    bool    initialize();
    void    authenticated( Network &n, const Server &s );
    void    connected( Network &n, const Server &s );
    void    joinedChannel( const QString&, Irc::Buffer* );
    void    leftChannel( const QString&, const QString &message, Irc::Buffer* );

  private slots:
    void    reset();

  private:
    Config         *config_;
    QList<Plugin*>  plugins_;
    bool            initialized_;
};

#endif
