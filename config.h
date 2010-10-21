/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QList>
#include <QtCore/QSettings>

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
  QString userName;
  QString fullName;
  QString password;
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

struct DatabaseConfig {
  QString type;
  QString hostname;
  quint16 port;
  QString username;
  QString password;
  QString database;
  QString options;
};

class Config : public QObject
{
  Q_OBJECT

  public:
        Config();
       ~Config();
  bool  loadFromFile( QString fileName );

  const QList<NetworkConfig*> &networks();
  const QString               &lastError();
  const DatabaseConfig        *databaseConfig() const;

  signals:
  void  beforeConfigReload();
  void  configReloaded();
  void  configReloadFailed();

  private:
  QList<NetworkConfig*> oldNetworks_;
  QList<NetworkConfig*> networks_;
  QString               error_;
  QSettings            *settings_;
  DatabaseConfig       *databaseConfig_;
};

#endif
