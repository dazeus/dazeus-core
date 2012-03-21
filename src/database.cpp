/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#include "database.h"
#include "config.h"
#include <QtCore/QVariant>
#include <QtCore/QDebug>
#include <mongo.h>
#include <cerrno>
#include <cassert>

// #define DEBUG

#define M (mongo_sync_connection*)m_

/**
 * @brief Constructor.
 */
Database::Database( const std::string &hostname, uint16_t port, const std::string &database, const std::string &username, const std::string &password )
: m_(0)
{
	hostName_ = hostname;
	port_ = port;
	databaseName_ = database;
	username_ = username;
	password_ = password;
}


/**
 * @brief Destructor.
 */
Database::~Database()
{
	if(m_)
		mongo_sync_disconnect(M);
}

/**
 * @brief Returns the last error in the QSqlDatabase object.
 */
std::string Database::lastError() const
{
	return lastError_;
}

bool Database::open()
{
	m_ = (void*)mongo_sync_connect(hostName_.c_str(), port_, TRUE);
	if(!m_) {
		lastError_ = strerror(errno);
		return false;
	}
	if(!mongo_sync_conn_set_auto_reconnect(M, TRUE)) {
		lastError_ = strerror(errno);
		mongo_sync_disconnect(M);
		return false;
	}
#warning TODO: db.properties.ensureIndex({'network':1,'receiver':1,'sender':1})
	return true;
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
	// variable, [networkScope, [receiverScope, [senderScope]]]
	// (empty if not given)

	// db.c.
	//   find() // for everything
	//   find({network:{$in: ['NETWORKNAME',null]}}) // for network scope
	//   find({network:{$in: ['NETWORKNAME',null]}, receiver:{$in: ['RECEIVER',null]}}) // for receiver scope
	//   find({network:..., receiver:..., sender:{$in: ['SENDER',null]}}) // for sender scope
	// .sort({'a':-1,'b':-1,'c':-1}).limit(1)
	// Reverse sort and limit(1) will make sure we only receive the most
	// specific result. This works, as long as the assumption is true that
	// no specific fields are set without their less specific fields also
	// being set (i.e. receiver can't be empty if sender is non-empty).

	bson *query = bson_build_full(
		BSON_TYPE_DOCUMENT, "$orderby", TRUE,
		bson_build(BSON_TYPE_INT32, "network", -1, BSON_TYPE_INT32, "receiver", -1, BSON_TYPE_INT32, "sender", -1, BSON_TYPE_NONE),
		BSON_TYPE_NONE
	);

	/*
		'$query':{
			'variable':'foo',
			'network':{'$in':['bla',null]},
			'receiver':{'$in':['moo', null]},
		}
	*/

	bson *selector = bson_new();
	bson *network = 0, *receiver = 0, *sender = 0;
	bson_append_string(selector, "variable", variable.toUtf8().constData(), -1);
	if(networkScope.length() > 0) {
		network = bson_build_full(
			BSON_TYPE_ARRAY, "$in", TRUE,
			bson_build(BSON_TYPE_STRING, "1", networkScope.toUtf8().constData(), -1,
			           BSON_TYPE_NULL, "2", BSON_TYPE_NONE),
			BSON_TYPE_NONE );
		bson_finish(network);
		bson_append_document(selector, "network", network);
		if(receiverScope.length() > 0) {
			receiver = bson_build_full(
				BSON_TYPE_ARRAY, "$in", TRUE,
				bson_build(BSON_TYPE_STRING, "1", receiverScope.toUtf8().constData(), -1,
				           BSON_TYPE_NULL, "2", BSON_TYPE_NONE),
				BSON_TYPE_NONE );
			bson_finish(receiver);
			bson_append_document(selector, "receiver", receiver);
			if(senderScope.length() > 0) {
				sender = bson_build_full(
					BSON_TYPE_ARRAY, "$in", TRUE,
					bson_build(BSON_TYPE_STRING, "1", senderScope.toUtf8().constData(), -1,
						   BSON_TYPE_NULL, "2", BSON_TYPE_NONE),
					BSON_TYPE_NONE );
				bson_finish(sender);
				bson_append_document(selector, "sender", sender);
			}
		}
	}
	bson_finish(selector);
	bson_append_document(query, "$query", selector);
	bson_finish(query);

	mongo_packet *p = mongo_sync_cmd_query(M, std::string(databaseName_ + ".properties").c_str(), 0, 0, 1, query, NULL /* TODO: value only? */);

	bson_free(query);
	bson_free(selector);
	if(network) bson_free(network);
	if(receiver) bson_free(receiver);
	if(sender) bson_free(sender);

	if(!p) {
		lastError_ = strerror(errno);
		return QVariant();
	}

	mongo_sync_cursor *cursor = mongo_sync_cursor_new(M, std::string(databaseName_ + ".properties").c_str(), p);
	if(!cursor) {
		lastError_ = strerror(errno);
		return QVariant();
	}

	if(!mongo_sync_cursor_next(cursor)) {
#ifdef DEBUG
		fprintf(stderr, "Variable %s not found within given scope.\n", variable.toUtf8().constData());
#endif
	}

	bson *result = mongo_sync_cursor_get_data(cursor);
	if(!result) {
		lastError_ = strerror(errno);
		mongo_sync_cursor_free(cursor);
		return QVariant();
	}

	bson_cursor *c = bson_find(result, "value");
	const char *value;
	if(!bson_cursor_get_string(c, &value)) {
		lastError_ = strerror(errno);
		mongo_sync_cursor_free(cursor);
		bson_cursor_free(c);
		return QVariant();
	}

	bson_free(result);
	bson_cursor_free(c);

	// only one result
	assert(!mongo_sync_cursor_next(cursor));

	mongo_sync_cursor_free(cursor);
	return QVariant(value);
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
