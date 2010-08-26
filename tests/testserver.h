/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#ifndef TESTSERVER_H
#define TESTSERVER_H

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtTest/QtTest>

#include "server.h"

class TestServer : public QObject
{
  Q_OBJECT

private slots:
  void initTest();
  void inputCommand();
  void inputOriginCommand();
  void inputCommandArg();
  void inputCommandLongArg();
  void inputCommandArgs();
  void inputOriginCommandArg();
  void inputOriginCommandLongArg();
  void inputOriginCommandArgs();
  void inputAnnoy1();
  void destroyTest();

private:
  Server *s;
  QString command, origin;
  QStringList arguments;
};

#endif
