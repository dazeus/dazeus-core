/**
 * Copyright (c) Ruben Nijveld, Aaron van Geffen, 2014
 * See LICENSE for license.
 */

#include "../config.h"
#include "postgres.h"
#include <pqxx/pqxx>
#include <iostream>

namespace dazeus {
namespace db {

PostgreSQLDatabase::~PostgreSQLDatabase()
{
	if (conn_) {
		conn_->disconnect();
	}
}

void PostgreSQLDatabase::open()
{
	std::stringstream connection_string;

	// Hostnames
	if (dbc_.hostname.length() > 0) {
		connection_string << "host = " << dbc_.hostname << " ";
	}

	// Port
	if (dbc_.port > 0) {
		connection_string << "port = " << dbc_.port << " ";
	}

	// Database name
	if (dbc_.database.length() > 0) {
		connection_string << "dbname = " << dbc_.database << " ";
	}

	// Username
    if (dbc_.username.length() > 0) {
		connection_string << "user = " << dbc_.username << " ";
	}

	// Password
	if (dbc_.password.length() > 0) {
		connection_string << "password = " << dbc_.password << " ";
	}

	// Other options
	connection_string << dbc_.options;

	// Connect the lot!
    try {
        conn_ = new pqxx::connection(connection_string.str());
        bootstrapDB();
        upgradeDB();
	}
	catch (pqxx::pqxx_exception& e) {
		fprintf(stderr, "Error: %s", e.base().what());
		throw exception(e.base().what());
	}
}

void PostgreSQLDatabase::bootstrapDB()
{
	// start by preparing all queries we might eventually need
	conn_->prepare("find_table", "SELECT * FROM pg_catalog.pg_tables WHERE tablename = $1");
	conn_->prepare("find_property",
			"SELECT * FROM dazeus_properties WHERE key = $1 "
			"AND (network = $2 OR network = '') "
			"AND (receiver = $3 OR receiver = '') "
			"AND (sender = $4 OR sender = '') "
			"ORDER BY network DESC, receiver DESC, sender DESC "
			"LIMIT 1"
		);
	conn_->prepare("remove_property", "DELETE FROM dazeus_properties WHERE key = $1 AND network = $2 AND receiver = $3 AND sender = $4");

	// this is an update-or-insert-query
	conn_->prepare("update_property",
			"WITH "
				"new_values(key, value, network, receiver, sender) AS (VALUES ($1, $2, $3, $4, $5)), "
				"upsert AS ( "
					"UPDATE dazeus_properties AS p "
					"SET value = nv.value "
          "FROM new_values AS nv "
					"WHERE p.key = nv.key AND p.network = nv.network AND p.receiver = nv.receiver AND p.sender = nv.sender "
					"RETURNING p.* "
				") "
			"INSERT INTO dazeus_properties(key, value, network, receiver, sender) "
			"SELECT key, value, network, sender, receiver "
			"FROM new_values AS n "
			"WHERE NOT EXISTS ( "
				"SELECT 1 FROM upsert AS up "
				"WHERE up.key = n.key AND up.network = n.network AND up.receiver = n.receiver AND up.sender = n.sender "
			")"
    );

	// get a list of keys starting with the given string
    conn_->prepare("properties",
				"SELECT key FROM dazeus_properties "
				"WHERE SUBSTRING(key FROM 1 FOR CHAR_LENGTH($1)) = $1 "
				"AND network = $2 AND receiver = $3 AND sender = $4"
			);

	// create permission, but only if it doesn't already exist
	conn_->prepare("add_permission",
			"WITH new_values(permission, network, receiver, sender) AS (VALUES ($1, $2, $3, $4)) "
			"INSERT INTO dazeus_permissions(permission, network, receiver, sender) "
			"SELECT permission, network, receiver, sender "
			"FROM new_values AS n "
			"WHERE NOT EXISTS ( "
				"SELECT 1 FROM dazeus_permissions dp "
				"WHERE dp.permission = n.permission AND dp.network = n.network AND dp.receiver = n.receiver AND dp.sender = n.sender "
			")"
	);
	conn_->prepare("remove_permission", "DELETE FROM dazeus_permissions WHERE permission = $1 AND network = $2 AND receiver = $3 AND sender = $4");
	conn_->prepare("has_permission", "SELECT * FROM dazeus_permissions WHERE permission = $1 AND network = $2 AND receiver = $3 AND sender= $4");
}

void PostgreSQLDatabase::upgradeDB()
{
	pqxx::work w(*conn_);
	pqxx::result r = w.prepared("find_table")("dazeus_properties").exec();

    int db_version = 0;
	if (!r.empty()) {
        pqxx::result v = w.prepared("find_property")("dazeus_version")("")("")("").exec();
		if (!v.empty()) {
            db_version = v[0]["value"].as<int>();
		}
	}

	std::string upgrades[] = {
		"CREATE TABLE dazeus_properties( "
			"key VARCHAR(255) NOT NULL, "
			"value TEXT NOT NULL, "
			"network VARCHAR(255) NOT NULL, "
			"receiver VARCHAR(255) NOT NULL, "
			"sender VARCHAR(255) NOT NULL, "
			"created TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP, "
			"updated TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP, "
			"CONSTRAINT dazeus_properties_pk PRIMARY KEY(key, value, network, receiver, sender) "
		")"
		,
		"CREATE TABLE dazeus_permissions( "
			"permission VARCHAR(255) NOT NULL, "
			"network VARCHAR(255) NOT NULL, "
			"receiver VARCHAR(255) NOT NULL, "
			"sender VARCHAR(255) NOT NULL, "
			"created TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP, "
			"updated TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP, "
			"CONSTRAINT dazeus_permissions_pk PRIMARY KEY(permission, network, receiver, sender) "
		")"
		,
		"CREATE OR REPLACE FUNCTION dazeus_update_timestamp_column() "
		"RETURNS TRIGGER AS ' "
		"BEGIN NEW.updated = CURRENT_TIMESTAMP; RETURN NEW; END; "
		"' LANGUAGE 'plpgsql' "
		,
		"CREATE TRIGGER dazeus_update_timestamp_properties "
		"BEFORE UPDATE ON dazeus_properties "
		"FOR EACH ROW "
		"EXECUTE PROCEDURE dazeus_update_timestamp_column() "
		,
		"CREATE TRIGGER dazeus_update_timestamp_permissions "
		"BEFORE UPDATE ON dazeus_permissions "
		"FOR EACH ROW "
		"EXECUTE PROCEDURE dazeus_update_timestamp_column() "
	};

    static int current_db_version = std::end(upgrades) - std::begin(upgrades);
    if (db_version < current_db_version) {
        std::cout << "Will now upgrade database from version " << db_version << " to version " << current_db_version << "." << std::endl;
        for (int i = db_version; i < current_db_version; ++i) {
            w.exec(upgrades[i]);
        }
        std::cout << "Upgrade completed. Will now update dazeus_version to " << current_db_version << std::endl;
        w.prepared("update_property")("dazeus_version")(current_db_version)("")("")("").exec();
        w.commit();
    }
}

std::string PostgreSQLDatabase::property(const std::string &variable,
			const std::string &networkScope,
			const std::string &receiverScope,
			const std::string &senderScope)
{
  // TODO: handle errors
  pqxx::work w(*conn_);
  pqxx::result r = w.prepared("find_property")(variable)(networkScope)(receiverScope)(senderScope).exec();
  if (!r.empty()) {
    return r[0]["value"].as<std::string>();
  }
  // TODO: what should we do if there is no result for us?
	return "";
}

void PostgreSQLDatabase::setProperty(const std::string &variable,
			const std::string &value, const std::string &networkScope,
			const std::string &receiverScope,
			const std::string &senderScope)
{
  // TODO: handle errors
  pqxx::work w(*conn_);
  if (value == "") {
    w.prepared("remove_property")(variable)(networkScope)(receiverScope)(senderScope).exec();
  } else {
    w.prepared("update_property")(variable)(value)(networkScope)(receiverScope)(senderScope).exec();
  }
  w.commit();
}

std::vector<std::string> PostgreSQLDatabase::propertyKeys(const std::string &prefix,
			const std::string &networkScope,
			const std::string &receiverScope,
			const std::string &senderScope)
{
    pqxx::work w(*conn_);
    pqxx::result r = w.prepared("properties")(prefix)(networkScope)(receiverScope)(senderScope).exec();

    // Return a vector of all the property keys matching the criteria.
    std::vector<std::string> keys = std::vector<std::string>();
    for (auto&& x : r) {
        keys.push_back(x["key"].as<std::string>());
    }
    return keys;
}

bool PostgreSQLDatabase::hasPermission(const std::string &perm_name,
			const std::string &network, const std::string &channel,
			const std::string &sender, bool defaultPermission) const
{
	pqxx::work w(*conn_);
    pqxx::result r = w.prepared("ḥas_permission")(perm_name)(network)(channel)(sender).exec();
    return r.empty() ? defaultPermission : true;
}

void PostgreSQLDatabase::unsetPermission(const std::string &perm_name,
			const std::string &network, const std::string &receiver,
			const std::string &sender)
{
    pqxx::work w(*conn_);
    pqxx::result r = w.prepared("remove_permission")(perm_name)(network)(receiver)(sender).exec();
    w.commit();
}

void PostgreSQLDatabase::setPermission(bool /* TODO: permission */, const std::string &perm_name,
			const std::string &network, const std::string &receiver,
			const std::string &sender)
{
    pqxx::work w(*conn_);
    pqxx::result r = w.prepared("add_permission")(perm_name)(network)(receiver)(sender).exec();
    w.commit();
}

}  // namespace db
}  // namespace dazeus
