/**
 * Copyright (c) 2014 Ruben Nijveld, Aaron van Geffen
 * See LICENSE for license.
 */

#include "../config.h"
#include "sqlite.h"
#include <sqlite3.h>
#include <stdio.h>
#include <iostream>

namespace dazeus {
namespace db {

/**
 * Cleanup all prepared queries and close the connection.
 */
SQLiteDatabase::~SQLiteDatabase()
{
  if (conn_) {
    sqlite3_finalize(find_property);
    sqlite3_finalize(remove_property);
    sqlite3_finalize(update_property);
    sqlite3_finalize(properties);
    sqlite3_finalize(add_permission);
    sqlite3_finalize(remove_permission);
    sqlite3_finalize(has_permission);
    int res = sqlite3_close(conn_);
    assert(res == SQLITE_OK);
  }
}

void SQLiteDatabase::open()
{
  // Connect the lot!
  int fail = sqlite3_open_v2(dbc_.filename.c_str(), &conn_,
                             SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
                             NULL);
	if (fail) {
    auto error = "Can't open database (code " + std::to_string(fail) + "): " +
                 sqlite3_errmsg(conn_);
		throw exception(error);
	}
  upgradeDB();
  bootstrapDB();
}

/**
 * @brief Try to create a prepared statement on the SQLite3 connection.
 */
void SQLiteDatabase::tryPrepare(const std::string &stmt, sqlite3_stmt **target)
{
  int result = sqlite3_prepare_v2(conn_, stmt.c_str(), stmt.length(), target,
                                  NULL);
  if (result != SQLITE_OK) {
    throw exception(
        std::string("Failed to prepare SQL statement, got an error: ") +
        sqlite3_errmsg(conn_));
  }
}

/**
 * @brief Try to bind a parameter to a prepared statement.
 */
void SQLiteDatabase::tryBind(sqlite3_stmt *target, int param,
                             const std::string &value)
{
  int result = sqlite3_bind_text(target, param, value.c_str(), value.length(),
                                 SQLITE_TRANSIENT);
  if (result != SQLITE_OK) {
    throw exception("Failed to bind parameter " + std::to_string(param) +
                    " to query with error: " + sqlite3_errmsg(conn_) +
                    " (errorcode " + std::to_string(result) + ")");
  }
}

/**
 * @brief Prepare all SQL statements used by the database layer
 */
void SQLiteDatabase::bootstrapDB()
{
	tryPrepare(
      "SELECT value FROM dazeus_properties "
      "WHERE key = ?1 "
      "  AND network = ?2 AND receiver = ?3 AND sender = ?4",
      &find_property);
	tryPrepare(
      "DELETE FROM dazeus_properties "
      "WHERE key = ?1 "
      "  AND network = ?2 AND receiver = ?3 AND sender = ?3",
      &remove_property);
	tryPrepare(
  		"INSERT OR REPLACE INTO dazeus_properties "
  		"(key, value, network, receiver, sender) "
  		"VALUES (?1, ?2, ?3, :receiver, :sender) ",
      &update_property);
	tryPrepare(
      "SELECT key FROM dazeus_properties "
      "WHERE key LIKE :key || '%' "
      "  AND network = :network AND receiver = :receiver AND sender = :sender",
      &properties);
	tryPrepare(
  		"INSERT OR REPLACE INTO dazeus_permissions "
  		"(permission, network, receiver, sender) "
  		"VALUES (:permission, :network, :receiver, :sender) ",
      &add_permission);
	tryPrepare(
      "DELETE FROM dazeus_permissions "
      "WHERE permission = :permission "
      "  AND network = :network AND receiver = :receiver AND sender = :sender",
      &remove_permission);
	tryPrepare(
      "SELECT * FROM dazeus_permissions "
      "WHERE permission = :permission "
      "  AND network = :network AND receiver = :receiver AND sender = :sender",
      &has_permission);
}

/**
 * @brief Upgrade the database to the latest version.
 */
void SQLiteDatabase::upgradeDB()
{
  bool found = false;
  int errc = sqlite3_exec(conn_,
      "SELECT name FROM sqlite_master WHERE type = 'table' "
      "AND name = 'dazeus_properties'",
      [](void* found, int, char**, char**)
      {
        *(static_cast<bool*>(found)) = true;
        return 0;
      }, static_cast<void*>(&found), NULL);

  if (errc != SQLITE_OK) {
    throw exception(std::string("Failed to retrieve database version: ") +
        sqlite3_errmsg(conn_));
  }

  int db_version = 0;
  if (errc == SQLITE_OK) {
    if (found) {
      // table exists, query for latest version
      errc = sqlite3_exec(conn_,
          "SELECT value FROM dazeus_properties "
          "WHERE key = 'dazeus_version' LIMIT 1",
          [](void* dbver, int columns, char** values, char**)
          {
            if (columns > 0) {
              *(static_cast<int*>(dbver)) = std::stoi(values[0]);
            }
            return 0;
          }, static_cast<void*>(&db_version), NULL);
    }
  }

  const char *upgrades[] = {
    "CREATE TABLE dazeus_properties( "
      "key VARCHAR(255) NOT NULL, "
      "value TEXT NOT NULL, "
      "network VARCHAR(255) NOT NULL, "
      "receiver VARCHAR(255) NOT NULL, "
      "sender VARCHAR(255) NOT NULL, "
      "created TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP, "
      "updated TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP, "
            "PRIMARY KEY(key, network, receiver, sender) "
    ")"
    ,
    "CREATE TABLE dazeus_permissions( "
      "permission VARCHAR(255) NOT NULL, "
      "network VARCHAR(255) NOT NULL, "
      "receiver VARCHAR(255) NOT NULL, "
      "sender VARCHAR(255) NOT NULL, "
      "created TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP, "
      "updated TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP, "
            "PRIMARY KEY(permission, network, receiver, sender) "
        ")"
  };

  // run any outstanding updates
  static int target_version = std::end(upgrades) - std::begin(upgrades);
  if (db_version < target_version) {
  	std::cout << "Will now upgrade database from version " << db_version
              << " to version " << target_version << "." << std::endl;
  	for (int i = db_version; i < target_version; ++i) {
  		int result = sqlite3_exec(conn_, upgrades[i], NULL, NULL, NULL);
      if (result != SQLITE_OK) {
        throw exception("Could not run upgrade " + std::to_string(i) +
                            ", got error: " + sqlite3_errmsg(conn_));
      }
    }
    std::cout << "Upgrade completed. Will now update dazeus_version to "
              << target_version << std::endl;
    errc = sqlite3_exec(conn_,
        ("INSERT OR REPLACE INTO dazeus_properties "
         "(key, value, network, receiver, sender) "
         "VALUES ('dazeus_version', '" + std::to_string(target_version) + "', "
         "        '', '', '') ").c_str(),
        NULL, NULL, NULL);
    if (errc != SQLITE_OK) {
      throw exception(
          "Could not set dazeus_version to latest version, got error: " +
          std::string(sqlite3_errmsg(conn_)));
    }
  }
}

std::string SQLiteDatabase::property(const std::string &variable,
                                     const std::string &networkScope,
                                     const std::string &receiverScope,
                                     const std::string &senderScope)
{
  tryBind(find_property, 1, variable);
  tryBind(find_property, 2, networkScope);
  tryBind(find_property, 3, receiverScope);
  tryBind(find_property, 4, senderScope);
  int errc = sqlite3_step(find_property);

  if (errc == SQLITE_ROW) {
    std::string value = reinterpret_cast<const char *>(sqlite3_column_text(find_property, 0));
    sqlite3_reset(find_property);
    return value;
  } else if (errc == SQLITE_DONE) {
    // TODO: define behavior when no result is found
    sqlite3_reset(find_property);
    return "";
  } else {
    std::string msg = "Got an error while executing an SQL query (code " +
                      std::to_string(errc) + "): " + sqlite3_errmsg(conn_);
    sqlite3_reset(find_property);
    throw exception(msg);
  }
}

void SQLiteDatabase::setProperty(const std::string &variable,
		const std::string &value, const std::string &networkScope,
		const std::string &receiverScope,
		const std::string &senderScope)
{
  sqlite3_bind_text(update_property, 1, variable.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(update_property, 2, value.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(update_property, 3, networkScope.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(update_property, 4, receiverScope.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(update_property, 5, senderScope.c_str(), -1, SQLITE_STATIC);
  int errc = sqlite3_step(update_property);

  if (errc != SQLITE_OK && errc != SQLITE_DONE) {
    const char *zErrMsg = sqlite3_errmsg(conn_);
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free(&zErrMsg);
    throw new exception("Could not set property in SQLite database!");
  }

  sqlite3_reset(update_property);
}

std::vector<std::string> SQLiteDatabase::propertyKeys(const std::string &ns,
			const std::string &networkScope,
			const std::string &receiverScope,
			const std::string &senderScope)
{
  // Return a vector of all the property keys matching the criteria.
  std::vector<std::string> keys = std::vector<std::string>();

  sqlite3_bind_text(properties, 1, ns.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(properties, 2, networkScope.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(properties, 3, receiverScope.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(properties, 4, senderScope.c_str(), -1, SQLITE_STATIC);

  while (true) {
    int errc = sqlite3_step(properties);

    if (errc == SQLITE_ROW) {
      std::string value = reinterpret_cast<const char *>(sqlite3_column_text(properties, 0));
      keys.push_back(value);
    } else if (errc == SQLITE_DONE) {
      sqlite3_reset(properties);
      break;
    } else {
      std::string msg = "Got an error while executing an SQL query (code " +
                        std::to_string(errc) + "): " + sqlite3_errmsg(conn_);
      sqlite3_reset(properties);
      throw exception(msg);
    }
  }

  return keys;
}

bool SQLiteDatabase::hasPermission(const std::string &perm_name,
			const std::string &network, const std::string &channel,
			const std::string &sender, bool defaultPermission) const
{
    /*pqxx::work w(*conn_);
    pqxx::result r = w.prepared("ḥas_permission")(perm_name)(network)(channel)(sender).exec();
    return r.empty() ? defaultPermission : true;*/
    return false;
}

void SQLiteDatabase::unsetPermission(const std::string &perm_name,
			const std::string &network, const std::string &receiver,
			const std::string &sender)
{
  sqlite3_bind_text(remove_permission, 1, perm_name.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(remove_permission, 2, network.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(remove_permission, 3, receiver.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(remove_permission, 4, sender.c_str(), -1, SQLITE_STATIC);
  int errc = sqlite3_step(remove_permission);

  if (errc != SQLITE_OK && errc != SQLITE_DONE) {
    const char *zErrMsg = sqlite3_errmsg(conn_);
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free(&zErrMsg);
    throw new exception("Could not unset permission in SQLite database!");
  }

  sqlite3_reset(remove_permission);
}

void SQLiteDatabase::setPermission(bool permission, const std::string &perm_name,
			const std::string &network, const std::string &receiver,
			const std::string &sender)
{
  sqlite3_bind_text(add_permission, 1, perm_name.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(add_permission, 2, network.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(add_permission, 3, receiver.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(add_permission, 4, sender.c_str(), -1, SQLITE_STATIC);
  int errc = sqlite3_step(add_permission);

  if (errc != SQLITE_OK && errc != SQLITE_DONE) {
    const char *zErrMsg = sqlite3_errmsg(conn_);
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free(&zErrMsg);
    throw new exception("Could not set permission in SQLite database!");
  }

  sqlite3_reset(add_permission);
}

}  // namespace db
}  // namespace dazeus
