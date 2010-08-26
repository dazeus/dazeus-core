/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#include "testserver.h"

void TestServer::initTest()
{
  s = new Server();
}

void TestServer::inputCommand()
{
  QVERIFY( s->parseLine( 
    "COMMAND1", origin, command, arguments ) );
  QCOMPARE( origin, QString("") );
  QCOMPARE( command, QString("COMMAND1") );
  QCOMPARE( arguments.size(), 0 );
}

void TestServer::inputOriginCommand()
{
  QVERIFY( s->parseLine( 
    ":ORIGIN2 COMMAND2", origin, command, arguments ) );
  QCOMPARE( origin, QString("ORIGIN2") );
  QCOMPARE( command, QString("COMMAND2") );
  QCOMPARE( arguments.size(), 0 );
}

void TestServer::inputCommandArg()
{
  QVERIFY( s->parseLine( 
    "COMMAND3 ARG3", origin, command, arguments ) );
  QCOMPARE( origin, QString("") );
  QCOMPARE( command, QString("COMMAND3") );
  QCOMPARE( arguments.size(), 1 );
  if( arguments.size() == 1 )
  {
    QCOMPARE( arguments[0], QString( "ARG3" ) );
  }
}

void TestServer::inputCommandLongArg()
{
  QVERIFY( s->parseLine( 
    "COMMAND4 :LONG ARG 4", origin, command, arguments ) );
  QCOMPARE( origin, QString("") );
  QCOMPARE( command, QString("COMMAND4") );
  QCOMPARE( arguments.size(), 1 );
  if( arguments.size() == 1 )
  {
    QCOMPARE( arguments[0], QString( "LONG ARG 4" ) );
  }
}

void TestServer::inputCommandArgs()
{
  QVERIFY( s->parseLine( 
    "COMMAND5 ARG5 :LONG ARG 5", origin, command, arguments ) );
  QCOMPARE( origin, QString("") );
  QCOMPARE( command, QString("COMMAND5") );
  QCOMPARE( arguments.size(), 2 );
  if( arguments.size() == 2 )
  {
    QCOMPARE( arguments[0], QString( "ARG5" ) );
    QCOMPARE( arguments[1], QString( "LONG ARG 5" ) );
  }
}

void TestServer::inputOriginCommandArg()
{
  QVERIFY( s->parseLine( 
    ":ORIGIN6 COMMAND6 ARG6", origin, command, arguments ) );
  QCOMPARE( origin, QString("ORIGIN6") );
  QCOMPARE( command, QString("COMMAND6") );
  QCOMPARE( arguments.size(), 1 );
  if( arguments.size() == 1 )
  {
    QCOMPARE( arguments[0], QString( "ARG6" ) );
  }
}

void TestServer::inputOriginCommandLongArg()
{
  QVERIFY( s->parseLine( 
    ":ORIGIN7 COMMAND7 :LONG ARG 7", origin, command, arguments ) );
  QCOMPARE( origin, QString("ORIGIN7") );
  QCOMPARE( command, QString("COMMAND7") );
  QCOMPARE( arguments.size(), 1 );
  if( arguments.size() == 1 )
  {
    QCOMPARE( arguments[0], QString( "LONG ARG 7" ) );
  }
}

void TestServer::inputOriginCommandArgs()
{
  QVERIFY( s->parseLine( 
    ":ORIGIN8 COMMAND8 ARG8 :LONG ARG 8", origin, command, arguments ) );
  QCOMPARE( origin, QString("ORIGIN8") );
  QCOMPARE( command, QString("COMMAND8") );
  QCOMPARE( arguments.size(), 2 );
  if( arguments.size() == 2 )
  {
    QCOMPARE( arguments[0], QString( "ARG8" ) );
    QCOMPARE( arguments[1], QString( "LONG ARG 8" ) );
  }
}

void TestServer::inputAnnoy1()
{
  QVERIFY( s->parseLine( 
    ": 2 3  5  7 : 9 ", origin, command, arguments ) );
  QCOMPARE( origin, QString("") );
  QCOMPARE( command, QString("2") );
  QCOMPARE( arguments.size(), 6 );
  if( arguments.size() == 6 )
  {
    QCOMPARE( arguments[0], QString( "3" ) );
    QCOMPARE( arguments[1], QString( ""  ) );
    QCOMPARE( arguments[2], QString( "5" ) );
    QCOMPARE( arguments[3], QString( ""  ) );
    QCOMPARE( arguments[4], QString( "7" ) );
    QCOMPARE( arguments[5], QString( " 9 " ) );
  }
}

void TestServer::destroyTest()
{
  delete s;
}

