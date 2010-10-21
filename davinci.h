/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#ifndef DAVINCI_H
#define DAVINCI_H

#include <QtCore/QString>
#include <QtCore/QObject>

// TODO remove
#include "network.h"

class Config;
class PluginManager;
class Network;
class Database;

class DaVinci : public QObject
{
  Q_OBJECT

  public:
             DaVinci( QString configFileName = QString() );
            ~DaVinci();
    void     setConfigFileName( QString fileName );
    QString  configFileName() const;
    bool     configLoaded() const;

    Database *database() const;

  public slots:
    bool     loadConfig();
    bool     initPlugins();
    void     autoConnect();
    bool     connectDatabase();

  private slots:
    void     resetConfig();
    void     welcomed();
    void     connected();
    void     disconnected();

  private:
    Config          *config_;
    QString          configFileName_;
    PluginManager   *pluginManager_;
    Database        *database_;
    QList<Network*>  networks_;
};

#endif
