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
#include <QtSql/QSqlRecord>

// #define DEBUG

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
#warning network is empty?
  return new Database( QString(), dbc->type, dbc->database, dbc->username,
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
 * @brief Retrieve all properties under a certain namespace.
 *
 * All properties whose names start with the given namespace name, then a
 * single dot, and that match the given scope values, are returned from this
 * method.
 *
 * This method guarantees that if it returns a certain key, property() will
 * return the correct value of that key given that network, receiver and
 * sender scope are the same.
 */
QStringList Database::propertyKeys( const QString &ns, const QString &networkScope,
 const QString &receiverScope, const QString &senderScope )
{
  checkDatabaseConnection();

  QString nsQuery = ns + ".%";
  QSqlQuery data(db_);
  data.prepare(QLatin1String("SELECT variable,network,receiver,sender"
    " FROM properties WHERE variable LIKE ?"));
  data.addBindValue(nsQuery);

  QStringList k;
  if( !data.exec() )
  {
    qWarning() << "Property retrieve keys failed: " << data.lastError();
    return k;
  }

  // index the results
  while( data.next() )
  {
    // Check if the name really matches
    QString actualName = data.value(0).toString();
    if( !actualName.startsWith(ns + '.') )
       continue;
    actualName = actualName.mid(ns.length() + 1);

    if( data.value(3).isNull() )
    {
      if( data.value(2).isNull() )
      {
        if( data.value(1).isNull() )
          k.append(actualName);
        else if( data.value(1).toString() == networkScope )
          k.append(actualName);
      }
      else if( data.value(1).toString() == networkScope
            && data.value(2).toString() == receiverScope )
        k.append(actualName);
    }
    else if( data.value(1).toString() == networkScope
          && data.value(2).toString() == receiverScope
          && data.value(3).toString() == senderScope )
      k.append(actualName);
  }

  return k;
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
 const QString &senderScope )
{
  checkDatabaseConnection();

  // retrieve from database
  QSqlQuery data(db_);
  data.prepare(QLatin1String("SELECT value,network,receiver,sender"
     " FROM properties WHERE variable=?"));
  data.addBindValue(variable);
  if( !data.exec() )
  {
    qWarning() << "Property retrieve query failed: " << data.lastError();
    return QVariant();
  }

  // index the results
  QVariant global, network, receiver, sender;
  while( data.next() )
  {
    if( data.value(3).isNull() )
    {
      if( data.value(2).isNull() )
      {
        if( data.value(1).isNull() )
          global = data.value(0);
        else if( data.value(1).toString() == networkScope )
          network = data.value(0);
      }
      else if( data.value(1).toString() == networkScope
            && data.value(2).toString() == receiverScope )
        receiver = data.value(0);
    }
    else if( data.value(1).toString() == networkScope
          && data.value(2).toString() == receiverScope
          && data.value(3).toString() == senderScope )
      sender = data.value(0);
  }

#ifdef DEBUG
  qDebug() << "For variable " << variable;
  qDebug() << "Global: " << global;
  qDebug() << "Network: " << network;
  qDebug() << "Receiver: " << receiver;
  qDebug() << "Sender: " << sender;
#endif

  if( !sender.isNull() )
    return sender;
  if( !receiver.isNull() )
    return receiver;
  if( !network.isNull() )
    return network;
  return global;
}

void Database::setProperty( const QString &variable,
 const QVariant &value, const QString &networkScope,
 const QString &receiverScope, const QString &senderScope )
{
#ifdef DEBUG
  qDebug() << "Setting property " << variable << "to" << value;
#endif
  checkDatabaseConnection();

  // first, select it from the database, to see whether to update or insert
  // (not all database engines support "on duplicate key update".)
  QSqlQuery finder(db_);
  // because size() always returns -1 on an SQLite backend, we use COUNT.
  finder.prepare(
      QString(QLatin1String("SELECT id,COUNT(id) FROM properties WHERE "
                "variable=? AND network %1 AND receiver %2 AND sender %3"))
      .arg( QLatin1String(networkScope.isEmpty()  ? "IS NULL" : "=?") )
      .arg( QLatin1String(receiverScope.isEmpty() ? "IS NULL" : "=?") )
      .arg( QLatin1String(senderScope.isEmpty()   ? "IS NULL" : "=?") )
  );
  // This ugly hack seems necessary as MySQL does not allow =NULL, requires IS NULL.
  finder.addBindValue(variable);
  if( !networkScope.isEmpty() )
    finder.addBindValue(networkScope);
  if( !receiverScope.isEmpty() )
    finder.addBindValue(receiverScope);
  if( !senderScope.isEmpty() )
    finder.addBindValue(senderScope);

  if( !finder.exec() )
  {
    qWarning() << "Select property before setting failed: " << finder.lastError();
    return;
  }

  if( !finder.next() )
  {
    qWarning() << "Could not select first value row before setting: " << finder.lastError();
    return;
  }

  int resultSize = finder.value(1).isValid() ? finder.value(1).toInt() : 0;
  int returnedId = finder.value(0).isValid() ? finder.value(0).toInt() : -1;

  if( resultSize > 1 )
    qWarning() << "*** WARNING: More than one result to variable retrieve! This is a bug. ***";

  QSqlQuery data(db_);

  if( !resultSize )
  {
    // insert
    if( !value.isValid() )
    {
      // Don't insert null values.
      return;
    }
#ifdef DEBUG
    qDebug() << "Seeing property " << variable << "for the first time, inserting.";
    qDebug() << finder.executedQuery();
    QList<QVariant> list = finder.boundValues().values();
    for (int i = 0; i < list.size(); ++i)
      qDebug() << i << ": " << list.at(i).toString().toAscii().data() << endl;
#endif
    data.prepare(QLatin1String("INSERT INTO properties (variable,value,"
                "network,receiver,sender) VALUES (?, ?, ?, ?, ?)"));
    data.addBindValue(variable);
    data.addBindValue(value);
    data.addBindValue(networkScope.isEmpty()  ? QVariant() : networkScope);
    data.addBindValue(receiverScope.isEmpty() ? QVariant() : receiverScope);
    data.addBindValue(senderScope.isEmpty()   ? QVariant() : senderScope);
  }
  else
  {
    // update
    if( !value.isValid() )
    {
      // Don't insert null values - delete the old one
      data.prepare(QLatin1String("DELETE FROM properties WHERE id=?"));
    }
    else
    {
      // update the old one
      data.prepare(QLatin1String("UPDATE properties SET value=? WHERE id=?"));
      data.addBindValue(value);
    }
    data.addBindValue(returnedId);
  }

#ifdef DEBUG
  qDebug() << "Executing data query: " << data.lastQuery();
  qDebug() << "Bound data query values: " << data.boundValues();
#endif

  if( !data.exec() )
  {
    qWarning() << "Set property failed: " << data.lastError();
  }
}

/**
 * This method checks whether there is still an active database connection, by
 * executing a simple 'dummy query' (SELECT 1).
 * 
 * By calling this function before most other functions in this file, a failed
 * connection can be detected, and maybe even fixed, before any data is lost.
 */
void Database::checkDatabaseConnection()
{
#ifdef DEBUG
  qDebug() << "Check database connection:" << db_.isOpen();
#endif

  QSqlQuery query(QLatin1String("SELECT 1"), db_);
  if( !query.exec() )
  {
    qWarning() << "Database ping failed: " << query.lastError();
    bool res = db_.open();
    qWarning() << "Trying to revive: " << res;
  }
}

bool Database::createTable()
{
  checkDatabaseConnection();
  if( tableExists() )
    return true; // database already exists

  QSqlQuery data(db_);
  QLatin1String idrow( "INT(10) PRIMARY KEY AUTO_INCREMENT" );
  if( db_.driverName().toLower().startsWith(QLatin1String("qsqlite")) )
  {
    idrow = QLatin1String("INTEGER PRIMARY KEY AUTOINCREMENT");
  }
  bool ok = data.exec(QString(QLatin1String("CREATE TABLE properties ("
                   "  id      %1,"
                   "  variable VARCHAR(150),"
                   "  network VARCHAR(50)," // enforced limit in config.cpp
                   "  receiver VARCHAR(50)," // usually, max = 9 or 30
                   "  sender VARCHAR(50)," //  usually, max = 9 or 30
                   "  value TEXT"
                   ");")).arg(idrow));
  if( !ok )
  {
    qWarning() << data.lastError();
  }
  return ok;
}
bool Database::tableExists()
{
  checkDatabaseConnection();
  return db_.record(QLatin1String("properties")).count() > 0;
}

QLatin1String Database::typeToQtPlugin(const QString &type)
{
  if( type.toLower() == QLatin1String("sqlite") )
    return QLatin1String("QSQLITE");
  if( type.toLower() == QLatin1String("mysql") )
    return QLatin1String("QMYSQL");

  qWarning() << "typeToQtPlugin: Unknown type: " << type;
  return QLatin1String("");
}
