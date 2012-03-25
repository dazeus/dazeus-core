/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#include "database.h"
#include "config.h"
#include "utils.h"
#include <mongo.h>
#include <cerrno>
#include <cassert>
#include <sstream>

// #define DEBUG

#define M (mongo_sync_connection*)m_
#define PROPERTIES std::string(databaseName_ + ".properties").c_str()

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

	bson *index = bson_build(
		BSON_TYPE_INT32, "network", 1,
		BSON_TYPE_INT32, "receiver", 1,
		BSON_TYPE_INT32, "sender", 1,
		BSON_TYPE_NONE
	);
	bson_finish(index);

	if(!mongo_sync_cmd_index_create(M, PROPERTIES, index, 0))
	{
#ifdef DEBUG
		fprintf(stderr, "Index create error: %s\n", strerror(errno));
#endif
		lastError_ = strerror(errno);
		bson_free(index);
		return false;
	}

	bson_free(index);
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
std::vector<std::string> Database::propertyKeys( const std::string &ns, const std::string &networkScope,
 const std::string &receiverScope, const std::string &senderScope )
{
	std::stringstream regexStr;
	regexStr << "^\\Q" << ns << "\\E\\.";
	std::string regex = regexStr.str();

	bson *selector = bson_new();
	bson_append_regex(selector, "variable", regex.c_str(), "");
	if(networkScope.length() > 0) {
		bson_append_string(selector, "network", networkScope.c_str(), -1);
		if(receiverScope.length() > 0) {
			bson_append_string(selector, "receiver", receiverScope.c_str(), -1);
			if(senderScope.length() > 0) {
				bson_append_string(selector, "sender", senderScope.c_str(), -1);
			} else {
				bson_append_null(selector, "sender");
			}
		} else {
			bson_append_null(selector, "receiver");
			bson_append_null(selector, "sender");
		}
	} else {
		bson_append_null(selector, "network");
		bson_append_null(selector, "receiver");
		bson_append_null(selector, "sender");
	}
	bson_finish(selector);

	mongo_packet *p = mongo_sync_cmd_query(M, PROPERTIES, 0, 0, 0, selector, NULL /* TODO: variable only? */);

	bson_free(selector);

	if(!p) {
		lastError_ = strerror(errno);
#ifdef DEBUG
		fprintf(stderr, "Database error: %s\n", lastError_.c_str());
#endif
		return std::vector<std::string>();
	}

	mongo_sync_cursor *cursor = mongo_sync_cursor_new(M, PROPERTIES, p);
	if(!cursor) {
		lastError_ = strerror(errno);
#ifdef DEBUG
		fprintf(stderr, "Database error: %s\n", lastError_.c_str());
#endif
		return std::vector<std::string>();
	}

	std::vector<std::string> res;
	while(mongo_sync_cursor_next(cursor)) {
		bson *result = mongo_sync_cursor_get_data(cursor);
		if(!result) {
			lastError_ = strerror(errno);
#ifdef DEBUG
			fprintf(stderr, "Database error: %s\n", lastError_.c_str());
#endif
			mongo_sync_cursor_free(cursor);
			return res;
		}

		bson_cursor *c = bson_find(result, "variable");
		const char *value;
		if(!bson_cursor_get_string(c, &value)) {
			lastError_ = strerror(errno);
#ifdef DEBUG
			fprintf(stderr, "Database error: %s\n", lastError_.c_str());
#endif
			mongo_sync_cursor_free(cursor);
			bson_cursor_free(c);
			return res;
		}

		value += ns.length() + 1;
		res.push_back(value);
	}

	return res;
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
std::string Database::property( const std::string &variable,
 const std::string &networkScope, const std::string &receiverScope,
 const std::string &senderScope )
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
	bson_append_string(selector, "variable", variable.c_str(), -1);
	if(networkScope.length() > 0) {
		network = bson_build_full(
			BSON_TYPE_ARRAY, "$in", TRUE,
			bson_build(BSON_TYPE_STRING, "1", networkScope.c_str(), -1,
			           BSON_TYPE_NULL, "2", BSON_TYPE_NONE),
			BSON_TYPE_NONE );
		bson_finish(network);
		bson_append_document(selector, "network", network);
		if(receiverScope.length() > 0) {
			receiver = bson_build_full(
				BSON_TYPE_ARRAY, "$in", TRUE,
				bson_build(BSON_TYPE_STRING, "1", receiverScope.c_str(), -1,
				           BSON_TYPE_NULL, "2", BSON_TYPE_NONE),
				BSON_TYPE_NONE );
			bson_finish(receiver);
			bson_append_document(selector, "receiver", receiver);
			if(senderScope.length() > 0) {
				sender = bson_build_full(
					BSON_TYPE_ARRAY, "$in", TRUE,
					bson_build(BSON_TYPE_STRING, "1", senderScope.c_str(), -1,
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

	mongo_packet *p = mongo_sync_cmd_query(M, PROPERTIES, 0, 0, 1, query, NULL /* TODO: value only? */);

	bson_free(query);
	bson_free(selector);
	if(network) bson_free(network);
	if(receiver) bson_free(receiver);
	if(sender) bson_free(sender);

	if(!p) {
		lastError_ = strerror(errno);
		return std::string();
	}

	mongo_sync_cursor *cursor = mongo_sync_cursor_new(M, PROPERTIES, p);
	if(!cursor) {
		lastError_ = strerror(errno);
		return std::string();
	}

	if(!mongo_sync_cursor_next(cursor)) {
#ifdef DEBUG
		fprintf(stderr, "Variable %s not found within given scope.\n", variable.c_str());
#endif
	}

	bson *result = mongo_sync_cursor_get_data(cursor);
	if(!result) {
		lastError_ = strerror(errno);
		mongo_sync_cursor_free(cursor);
		return std::string();
	}

	bson_cursor *c = bson_find(result, "value");
	const char *value;
	if(!bson_cursor_get_string(c, &value)) {
		lastError_ = strerror(errno);
#ifdef DEBUG
		fprintf(stderr, "Database error: %s\n", lastError_.c_str());
#endif
		mongo_sync_cursor_free(cursor);
		bson_cursor_free(c);
		return std::string();
	}

	std::string res(value);

	bson_free(result);
	bson_cursor_free(c);

	// only one result
	assert(!mongo_sync_cursor_next(cursor));

	mongo_sync_cursor_free(cursor);
	return res;
}

void Database::setProperty( const std::string &variable,
 const std::string &value, const std::string &networkScope,
 const std::string &receiverScope, const std::string &senderScope )
{
	/**
		selector: {network:'foo',receiver:'bar',sender:null,variable:'bla.blob'}
		object:   {'$set':{ 'value': 'bla'}}
	*/
	bson *object = bson_build_full(
		BSON_TYPE_DOCUMENT, "$set", TRUE,
		bson_build(
			BSON_TYPE_STRING, "value", value.c_str(), -1,
			BSON_TYPE_NONE),
		BSON_TYPE_NONE);
	bson_finish(object);
	bson *selector = bson_build(
		BSON_TYPE_STRING, "variable", variable.c_str(), -1,
		BSON_TYPE_NONE);
	if(networkScope.length() > 0) {
		bson_append_string(selector, "network", networkScope.c_str(), -1);

		if(receiverScope.length() > 0) {
			bson_append_string(selector, "receiver", receiverScope.c_str(), -1);

			if(senderScope.length() > 0) {
				bson_append_string(selector, "sender", senderScope.c_str(), -1);
			} else {
				bson_append_null(selector, "sender");
			}
		} else {
			bson_append_null(selector, "receiver");
			bson_append_null(selector, "sender");
		}
	} else {
		bson_append_null(selector, "network");
		bson_append_null(selector, "receiver");
		bson_append_null(selector, "sender");
	}
	bson_finish(selector);

	// if the value length is zero, run a delete instead
	if(value.length() == 0) {
		if(!mongo_sync_cmd_delete(M, PROPERTIES,
			0, selector))
		{
			lastError_ = strerror(errno);
#ifdef DEBUG
			fprintf(stderr, "Error: %s\n", lastError_.c_str());
#endif
		}
	} else if(!mongo_sync_cmd_update(M, PROPERTIES,
		MONGO_WIRE_FLAG_UPDATE_UPSERT, selector, object))
	{
		lastError_ = strerror(errno);
#ifdef DEBUG
		fprintf(stderr, "Error: %s\n", lastError_.c_str());
#endif
	}
	bson_free(object);
	bson_free(selector);
}
