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
  data.prepare("SELECT value,network,receiver,sender"
     " FROM properties WHERE variable=?");
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

  // set in database
  QSqlQuery finder(db_);
  finder.prepare("SELECT id FROM properties WHERE variable=? AND network"
                 + QString(networkScope.isEmpty()  ? " IS NULL" : "=?")
                 + " AND receiver"
                 + QString(receiverScope.isEmpty() ? " IS NULL" : "=?")
                 + " AND sender"
                 + QString(senderScope.isEmpty()   ? " IS NULL" : "=?"));
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
  if( finder.size() < 0 )
  {
    qWarning() << "Select property before setting gave no results: " << finder.lastError();
    return;
  }

  QSqlQuery data(db_);

  if( finder.size() == 0 )
  {
#ifdef DEBUG
    qDebug() << "Seeing property " << variable << "for the first time, inserting.";
    qDebug() << finder.executedQuery();
    QList<QVariant> list = finder.boundValues().values();
    for (int i = 0; i < list.size(); ++i)
      qDebug() << i << ": " << list.at(i).toString().toAscii().data() << endl;
#endif
    // insert
    data.prepare("INSERT INTO properties (variable,value,network,receiver,sender)"
                 " VALUES (?, ?, ?, ?, ?)");
    data.addBindValue(variable);
    data.addBindValue(value);
    data.addBindValue(networkScope.isEmpty()  ? QVariant() : networkScope);
    data.addBindValue(receiverScope.isEmpty() ? QVariant() : receiverScope);
    data.addBindValue(senderScope.isEmpty()   ? QVariant() : senderScope);
  }
  else
  {
    // update
    finder.next();
    int returnedId = finder.value(0).toInt();
    data.prepare("UPDATE properties SET value=? WHERE id=?");
    data.addBindValue(value);
    data.addBindValue(returnedId);
  }

  if( !data.exec() )
  {
    qWarning() << "Set property failed: " << data.lastError();
  }
}

bool Database::createTable()
{
  if( tableExists() )
    return true; // database already exists

  QSqlQuery data(db_);
  return data.exec("CREATE TABLE properties ("
                   "  id       INT(10) PRIMARY KEY AUTO_INCREMENT,"
                   "  variable VARCHAR(150),"
                   "  network VARCHAR(50)," // enforced limit in config.cpp
                   "  receiver VARCHAR(50)," // usually, max = 9 or 30
                   "  sender VARCHAR(50)," //  usually, max = 9 or 30
                   "  value TEXT"
                   ");");
}
bool Database::tableExists() const
{
  return db_.record("properties").count() > 0;
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
