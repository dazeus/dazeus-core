/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QList>

/**
 * These structs are only for configuration as it is in the configuration
 * file. This configuration cannot be changed via the runtime interface of
 * the IRC bot. Things like autojoining channels are not in here, because
 * that's handled by admins during runtime... :)
 */
struct ServerConfig;

struct NetworkConfig {
  QString name;
  QString displayName;
  QString nickName;
  QList<ServerConfig*> servers;
  bool autoConnect;
};

struct ServerConfig {
  QString host;
  quint16 port;
  quint8 priority;
  NetworkConfig *network;
  bool ssl;
};

class Config : public QObject
{
  Q_OBJECT

  public:
        Config();
  bool  loadFromFile( QString fileName );

  const QList<NetworkConfig*> &networks();
  const QString               &lastError();

  signals:
  void  beforeConfigReload();
  void  configReloaded();
  void  configReloadFailed();

  private:
  QList<NetworkConfig*> oldNetworks_;
  QList<NetworkConfig*> networks_;
  QString               nickName_;
  QString               error_;
};

#endif
