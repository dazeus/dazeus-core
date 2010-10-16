/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#include "database.h"
#include <QtCore/QVariant>
#include <QtCore/QDebug>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

/**
 * @brief Constructor.
 */
Database::Database( const QString &network, const QString &dbType,
                    const QString &databaseName, const QString &username,
                    const QString &password, const QString &hostname,
                    int port, const QString &options )
: network_(network.toLower())
, db_(QSqlDatabase::addDatabase(dbType))
{
  db_.setDatabaseName( databaseName );
  db_.setUserName( username );
  db_.setPassword( password );
  db_.setHostName( hostname );
  db_.setPort( port );
  db_.setConnectOptions( options );
}


/**
 * @brief Destructor.
 */
Database::~Database()
{
}

/**
 * @brief Returns the network for which this Database was created.
 */
const QString &Database::network() const
{
  return network_;
}

bool Database::open()
{
  return db_.open();
}

/**
 * @brief Retrieve property from database.
 *
 * The most specific scope will be selected first, i.e. user-specific scope.
 * If that yields no results, channel-specific scope will be returned, then
 * network-specific scope, then global scope.
 *
 * Variable should be formatted using reverse-domain format, i.e. for the
 * plugin MyFirstPlugin created by John Doe, to save a 'foo' variable one
 * could use:
 * <code>net.jondoe.myfirstplugin.foo</code>
 */
QVariant Database::property( const QString &variable,
 const QString &userScope, const QString &channelScope ) const
{
  // retrieve from database
  QSqlQuery data(db_);
  data.prepare("SELECT value FROM properties WHERE variable=:var"
               " AND networkScope=:net AND channelScope=:chn"
               " AND userScope=:usr");

  data.bindValue(":var",variable);
  data.bindValue(":net",network_);
  data.bindValue(":chn",channelScope);
  data.bindValue(":usr",userScope);

  // first, ask for most specific...
  if( ! network_.isEmpty() && !channelScope.isEmpty()
   && !userScope.isEmpty() )
  {
    data.exec();
    if( data.next() )
      return data.value(0);
    data.finish();
  }

  // then, ask for channel specific
  if( !network_.isEmpty() && !channelScope.isEmpty() )
  {
    data.bindValue(":usr",QVariant());
    data.exec();
    if( data.next() )
      return data.value(0);
    data.finish();
  }

  // ask for network specific
  if( !network_.isEmpty() )
  {
    data.bindValue(":chn",QVariant());
    data.exec();
    if( data.next() )
      return data.value(0);
    data.finish();
  }

  // global property, then
  data.bindValue(":net",QVariant());
  data.exec();
  if( data.next() )
    return data.value(0);
  data.finish();

  // nothing found, still? return null
  return QVariant();
}

void Database::setProperty( const QString &variable,
 const QVariant &value,
 const QString &userScope, const QString &channelScope )
{
  // set in database
  QSqlQuery data(db_);
  data.prepare("INSERT INTO properties (variable,value,network,channel,user)"
               " VALUES (:var, :val, :net, :chn, :usr)");

  data.bindValue(":var",variable);
  data.bindValue(":val",value);
  data.bindValue(":net",network_.isEmpty() ? QVariant() : channelScope);
  data.bindValue(":chn",channelScope.isEmpty() ? QVariant() : channelScope);
  data.bindValue(":usr",userScope.isEmpty() ? QVariant() : userScope);
  if( !data.exec() )
  {
    qWarning() << "Set property failed: " << data.lastError();
  }
}

bool Database::createTable()
{
  // variable | network | channel | user | value
#warning todo
}
bool Database::tableExists() const
{
#warning todo
}
