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

class DaVinci : public QObject
{
  Q_OBJECT

  public:
             DaVinci( QString configFileName = QString() );
            ~DaVinci();
    void     setConfigFileName( QString fileName );
    QString  configFileName() const;
    bool     configLoaded() const;

  public slots:
    bool     loadConfig();
    bool     initPlugins();
    void     autoConnect();

  private slots:
    void     resetConfig();
    void     authenticated();
    void     connected();

  private:
    Config          *config_;
    QString          configFileName_;
    PluginManager   *pluginManager_;
    QList<Network*>  networks_;
};

#endif
