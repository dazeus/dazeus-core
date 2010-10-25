/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#include "database.h"
#include "config.h"
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
, db_(QSqlDatabase::addDatabase(typeToQtPlugin(dbType)))
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
 * @brief Create a Database from database configuration.
 */
Database *Database::fromConfig(const DatabaseConfig *dbc)
{
  return new Database( "", dbc->type, dbc->database, dbc->username,
                           dbc->password, dbc->hostname, dbc->port,
                           dbc->options );
}

/**
 * @brief Returns the last error in the QSqlDatabase object.
 */
QSqlError Database::lastError() const
{
  return db_.lastError();
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
 const QString &networkScope, const QString &receiverScope,
 const QString &senderScope ) const
{
  // retrieve from database
  QSqlQuery data(db_);
  data.prepare("SELECT value FROM properties WHERE variable=:var"
               " AND networkScope=:net AND receiverScope=:rcv"
               " AND senderScope=:snd");

  data.bindValue(":var",variable);
  data.bindValue(":net",networkScope);
  data.bindValue(":rcv",receiverScope);
  data.bindValue(":snd",senderScope);

  // first, ask for most specific...
  if( !networkScope.isEmpty() && !receiverScope.isEmpty()
   && !senderScope.isEmpty() )
  {
    data.exec();
    if( data.next() )
      return data.value(0);
    data.finish();
  }

  // then, ask for channel specific
  if( !networkScope.isEmpty() && !receiverScope.isEmpty() )
  {
    data.bindValue(":snd",QVariant());
    data.exec();
    if( data.next() )
      return data.value(0);
    data.finish();
  }

  // ask for network specific
  if( !networkScope.isEmpty() )
  {
    data.bindValue(":rcv",QVariant());
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
 const QVariant &value, const QString &networkScope,
 const QString &receiverScope, const QString &senderScope )
{
  // set in database
  QSqlQuery data(db_);
  data.prepare("INSERT INTO properties (variable,value,network,channel,user)"
/*               " VALUES (:var, :val, :net, :rcv, :snd)");

  data.bindValue(":var", variable);
  data.bindValue(":val", value);
  data.bindValue(":net", networkScope.isEmpty()  ? QVariant() : networkScope);
  data.bindValue(":rcv", receiverScope.isEmpty() ? QVariant() : receiverScope);
  data.bindValue(":snd", senderScope.isEmpty()   ? QVariant() : senderScope);*/
                " VALUES (?, ?, ?, ?, ?)");
  data.addBindValue(variable);
  data.addBindValue(value);
  data.addBindValue(networkScope.isEmpty()  ? QVariant() : networkScope);
  data.addBindValue(receiverScope.isEmpty() ? QVariant() : receiverScope);
  data.addBindValue(senderScope.isEmpty()   ? QVariant() : senderScope);
  if( !data.exec() )
  {
    qWarning() << "Set property failed: " << data.lastError();
  }
}

bool Database::createTable()
{
  if( tableExists() )
    return true; // database already exists

  return false; // TODO: create it
  // variable | network | receiver | sender | value
}
bool Database::tableExists() const
{
  return database_->record("properties").count() > 0;
}

QString Database::typeToQtPlugin(const QString &type)
{
  if( type.toLower() == "sqlite" )
    return "QSQLITE";
  if( type.toLower() == "mysql" )
    return "QMYSQL";

  qWarning() << "typeToQtPlugin: Unknown type: " << type;
  return "";
}
