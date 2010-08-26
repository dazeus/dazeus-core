/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#ifndef TESTUSER_H
#define TESTUSER_H

#include <QtCore/QString>
#include <QtTest/QtTest>

#include "user.h"

class TestUser : public QObject
{
  Q_OBJECT

private slots:
  void constructAllSetAll();
  void constructAllSetFull();
  void constructFullSetAll();
  void constructFullSetFull();
};

#endif
