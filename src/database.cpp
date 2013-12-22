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
#include <stdio.h>

#define M (mongo_sync_connection*)m_

/**
 * @brief Constructor.
 */
dazeus::Database::Database(DatabaseConfig dbc)
: m_(0)
, dbc_(dbc)
{
}


/**
 * @brief Destructor.
 */
dazeus::Database::~Database()
{
	if(m_)
		mongo_sync_disconnect(M);
}

void dazeus::Database::open()
{
	m_ = (void*)mongo_sync_connect(dbc_.hostname.c_str(), dbc_.port, TRUE);
	if(!m_) {
		throw exception("Failed to connect to database: " + std::string(strerror(errno)));
	}
	if(!mongo_sync_conn_set_auto_reconnect(M, TRUE)) {
		mongo_sync_disconnect(M);
		m_ = 0;
		throw exception("Failed to set autoreconnect");
	}

	bson *index = bson_build(
		BSON_TYPE_INT32, "network", 1,
		BSON_TYPE_INT32, "receiver", 1,
		BSON_TYPE_INT32, "sender", 1,
		BSON_TYPE_NONE
	);
	bson_finish(index);

	std::string properties = dbc_.database + ".properties";
	if(!mongo_sync_cmd_index_create(M, properties.c_str(), index, 0))
	{
		bson_free(index);
		throw exception("Failed to create property index");
	}

	bson_free(index);
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
std::vector<std::string> dazeus::Database::propertyKeys( const std::string &ns, const std::string &networkScope,
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

	std::string properties = dbc_.database + ".properties";
	mongo_packet *p = mongo_sync_cmd_query(M, properties.c_str(), 0, 0, 0, selector, NULL /* TODO: variable only? */);

	bson_free(selector);

	if(!p) {
		throw exception("Failed to execute query");
	}

	mongo_sync_cursor *cursor = mongo_sync_cursor_new(M, properties.c_str(), p);
	if(!cursor) {
		throw exception("Failed to create cursor");
	}

	std::vector<std::string> res;
	while(mongo_sync_cursor_next(cursor)) {
		bson *result = mongo_sync_cursor_get_data(cursor);
		if(!result) {
			mongo_sync_cursor_free(cursor);
			throw exception("Failed to get data from cursor");
		}

		bson_cursor *c = bson_find(result, "variable");
		const char *value;
		if(!bson_cursor_get_string(c, &value)) {
			mongo_sync_cursor_free(cursor);
			bson_cursor_free(c);
			throw exception("Failed to get string form cursor");
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
std::string dazeus::Database::property( const std::string &variable,
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

	std::string properties = dbc_.database + ".properties";
	mongo_packet *p = mongo_sync_cmd_query(M, properties.c_str(), 0, 0, 1, query, NULL /* TODO: value only? */);

	bson_free(query);
	bson_free(selector);
	if(network) bson_free(network);
	if(receiver) bson_free(receiver);
	if(sender) bson_free(sender);

	if(!p) {
		// error asking, or no results, assume it's the second
		return std::string();
	}

	mongo_sync_cursor *cursor = mongo_sync_cursor_new(M, properties.c_str(), p);
	if(!cursor) {
		throw exception("Failed to allocate cursor");
	}

	mongo_sync_cursor_next(cursor);

	bson *result = mongo_sync_cursor_get_data(cursor);
	if(!result) {
		mongo_sync_cursor_free(cursor);
		throw exception("Failed to retrieve data from cursor");
	}

	bson_cursor *c = bson_find(result, "value");
	const char *value;
	if(!bson_cursor_get_string(c, &value)) {
		mongo_sync_cursor_free(cursor);
		bson_cursor_free(c);
		throw exception("Failed to retrieve data from cursor");
	}

	std::string res(value);

	bson_free(result);
	bson_cursor_free(c);

	// only one result
	assert(!mongo_sync_cursor_next(cursor));

	mongo_sync_cursor_free(cursor);
	return res;
}

void dazeus::Database::setProperty( const std::string &variable,
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

	std::string properties = dbc_.database + ".properties";
	std::string error;
	// if the value length is zero, run a delete instead
	if(value.length() == 0) {
		if(!mongo_sync_cmd_delete(M, properties.c_str(),
			0, selector))
		{
			error = "Failed to delete property";
		}
	} else if(!mongo_sync_cmd_update(M, properties.c_str(),
		MONGO_WIRE_FLAG_UPDATE_UPSERT, selector, object))
	{
		error = "Failed to update property";
	}
	bson_free(object);
	bson_free(selector);
	if(!error.empty()) {
		throw exception(error);
	}
}
