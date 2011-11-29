/**
 * Copyright (c) Sjors Gielen, 2011
 * See LICENSE for license.
 */

#ifndef SOCKETPLUGIN_H
#define SOCKETPLUGIN_H

#include "plugin.h"
#include <QtCore/QVariant>
#include <QtCore/QIODevice>
#include <QtCore/QByteArray>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QLocalServer>

class SocketPlugin : public Plugin
{
  Q_OBJECT

  public:
            SocketPlugin(PluginManager *man);
  virtual  ~SocketPlugin();

  public slots:
    virtual void init();

  private slots:
    void newTcpConnection();
    void newLocalConnection();
    void poll();

  private:
    QList<QTcpServer*> tcpServers_;
    QList<QLocalServer*> localServers_;
    QList<QIODevice*> sockets_;
    void handle(QIODevice *dev, const QByteArray &line);
};

#endif
