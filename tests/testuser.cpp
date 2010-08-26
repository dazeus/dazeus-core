/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#include "testuser.h"

void TestUser::constructAllSetAll()
{
  User u( "_NICK_", "_USER_", "_HOST_" );
  QCOMPARE( u.nick(), QString( "_NICK_" ) );
  QCOMPARE( u.ident(), QString( "_USER_" ) );
  QCOMPARE( u.host(), QString( "_HOST_" ) );
  u.setNick( "_NICK2_" );
  u.setIdent( "_USER2_" );
  u.setHost( "_HOST2_" );
  QCOMPARE( u.nick(), QString( "_NICK2_" ) );
  QCOMPARE( u.ident(), QString( "_USER2_" ) );
  QCOMPARE( u.host(), QString( "_HOST2_" ) );
}

void TestUser::constructAllSetFull() {
  User u( "_NICK3_", "_USER3_", "_HOST3_" );
  QCOMPARE( u.nick(), QString( "_NICK3_" ) );
  QCOMPARE( u.ident(), QString( "_USER3_" ) );
  QCOMPARE( u.host(), QString( "_HOST3_" ) );
  u.setFullName( "_NICK4_!_USER4_@_HOST4_" );
  QCOMPARE( u.nick(), QString( "_NICK4_" ) );
  QCOMPARE( u.ident(), QString( "_USER4_" ) );
  QCOMPARE( u.host(), QString( "_HOST4_" ) );
  u.setFullName( "_NICK5_!_USER5_" );
  QCOMPARE( u.nick(), QString( "_NICK5_" ) );
  QCOMPARE( u.ident(), QString( "_USER5_" ) );
  QCOMPARE( u.host(), QString( "" ) );
  u.setFullName( "_NICK6_@_HOST6_" );
  QCOMPARE( u.nick(), QString( "_NICK6_" ) );
  QCOMPARE( u.ident(), QString( "" ) );
  QCOMPARE( u.host(), QString( "_HOST6_" ) );
  u.setFullName( "_NICK7_" );
  QCOMPARE( u.nick(), QString( "_NICK7_" ) );
  QCOMPARE( u.ident(), QString( "" ) );
  QCOMPARE( u.host(), QString( "" ) );
}

void TestUser::constructFullSetAll() {
  User u( "_NICK8_!_USER8_@_HOST8_" );
  QCOMPARE( u.nick(), QString( "_NICK8_" ) );
  QCOMPARE( u.ident(), QString( "_USER8_" ) );
  QCOMPARE( u.host(), QString( "_HOST8_" ) );
  u.setNick( "_NICK9_" );
  u.setIdent( "_USER9_" );
  u.setHost( "_HOST9_" );
  QCOMPARE( u.nick(), QString( "_NICK9_" ) );
  QCOMPARE( u.ident(), QString( "_USER9_" ) );
  QCOMPARE( u.host(), QString( "_HOST9_" ) );
}
void TestUser::constructFullSetFull() {
  User u( "_NICK10_!_USER10_@_HOST10_" );
  QCOMPARE( u.nick(), QString( "_NICK10_" ) );
  QCOMPARE( u.ident(), QString( "_USER10_" ) );
  QCOMPARE( u.host(), QString( "_HOST10_" ) );
  u.setFullName( "_NICK11_!_USER11_@_HOST11_" );
  QCOMPARE( u.nick(), QString( "_NICK11_" ) );
  QCOMPARE( u.ident(), QString( "_USER11_" ) );
  QCOMPARE( u.host(), QString( "_HOST11_" ) );
  u.setFullName( "_NICK12_!_USER12_" );
  QCOMPARE( u.nick(), QString( "_NICK12_" ) );
  QCOMPARE( u.ident(), QString( "_USER12_" ) );
  QCOMPARE( u.host(), QString( "" ) );
  u.setFullName( "_NICK13_@_HOST13_" );
  QCOMPARE( u.nick(), QString( "_NICK13_" ) );
  QCOMPARE( u.ident(), QString( "" ) );
  QCOMPARE( u.host(), QString( "_HOST13_" ) );
  u.setFullName( "_NICK14_" );
  QCOMPARE( u.nick(), QString( "_NICK14_" ) );
  QCOMPARE( u.ident(), QString( "" ) );
  QCOMPARE( u.host(), QString( "" ) );
}
