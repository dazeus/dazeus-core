/**
 * Copyright (c) 2014 Ruben Nijveld, Aaron van Geffen
 * See LICENSE for license.
 */

#include "../config.h"
#include "sqlite.h"
#include <sqlite3.h>
#include <stdio.h>

namespace dazeus {
namespace db {

SQLiteDatabase::~SQLiteDatabase()
{
    if (conn_) {
        sqlite3_finalize(find_table);
        sqlite3_finalize(find_property);
        sqlite3_finalize(remove_property);
        sqlite3_finalize(update_property);
        sqlite3_finalize(properties);
        sqlite3_finalize(add_permission);
        sqlite3_finalize(remove_permission);
        sqlite3_finalize(has_permission);
        sqlite3_close_v2(conn_);
    }
}

void SQLiteDatabase::open()
{
    // Connect the lot!
    int result = sqlite3_open_v2(dbc_.filename.c_str(), &conn_, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (result == 0) {
        bootstrapDB();
        upgradeDB();
  }
    else {
        fprintf(stderr, "An error occured (code %d) whilst connecting to SQLite database %s", result, dbc_.filename.c_str());
        throw new exception("Could not connect to SQLite database!");
  }
}

void SQLiteDatabase::bootstrapDB()
{
    sqlite3_prepare_v2(conn_, "SELECT name FROM sqlite_master WHERE type = 'table' AND name = :name", -1, &find_table, NULL);

    sqlite3_prepare_v2(conn_, "SELECT value FROM dazeus_properties WHERE key = :key AND network = :network AND receiver = :receiver AND sender = :sender", -1, &find_property, NULL);

    sqlite3_prepare_v2(conn_, "DELETE FROM dazeus_properties WHERE key = :key AND network = :network AND receiver = :receiver AND sender = :sender", -1, &remove_property, NULL);

    sqlite3_prepare_v2(conn_,
        "INSERT OR REPLACE INTO dazeus_properties "
        "(key, value, network, receiver, sender) "
        "VALUES "
        "(:key, :value, :network, :receiver, :sender) "
        , -1, &update_property, NULL);

    sqlite3_prepare_v2(conn_, "SELECT key FROM dazeus_properties WHERE key LIKE :key || '%' AND network = :network AND receiver = :receiver AND sender = :sender", -1, &properties, NULL);

    sqlite3_prepare_v2(conn_,
        "INSERT OR REPLACE INTO dazeus_permissions "
        "(permission, network, receiver, sender) "
        "VALUES "
        "(:permission, :network, :receiver, :sender) "
        , -1, &add_permission, NULL);

    sqlite3_prepare_v2(conn_, "DELETE FROM dazeus_permissions WHERE permission = :permission AND network = :network AND receiver = :receiver AND sender = :sender", -1, &remove_permission, NULL);

    sqlite3_prepare_v2(conn_, "SELECT * FROM dazeus_permissions WHERE permission = :permission AND network = :network AND receiver = :receiver AND sender = :sender", -1, &has_permission, NULL);
}

void SQLiteDatabase::upgradeDB()
{
    int errc = 0;

    // Find out whether there's a properties table in place already.
    sqlite3_bind_text(find_table, 1, "dazeus_properties", -1, SQLITE_STATIC);
    errc = sqlite3_step(find_table);
    if (errc != SQLITE_OK && errc != SQLITE_ROW && errc != SQLITE_DONE) {
        const char *zErrMsg = sqlite3_errmsg(conn_);
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(&zErrMsg);
        throw new exception("Could not query SQLite database!");
    }

    // If there is a properties table, find out what version we're on.
    int db_version = 0;
    if (errc != SQLITE_DONE) {
        std::string table_name = reinterpret_cast<const char *>(sqlite3_column_text(find_table, 0));
        if (table_name == "dazeus_properties") {
            sqlite3_bind_text(find_property, 1, "dazeus_version", -1, SQLITE_STATIC);
            sqlite3_bind_text(find_property, 2, "", -1, SQLITE_STATIC);
            sqlite3_bind_text(find_property, 3, "", -1, SQLITE_STATIC);
            sqlite3_bind_text(find_property, 4, "", -1, SQLITE_STATIC);

            errc = sqlite3_step(find_property);

            if (errc != SQLITE_OK && errc != SQLITE_ROW && errc != SQLITE_DONE) {
                const char *zErrMsg = sqlite3_errmsg(conn_);
                fprintf(stderr, "SQL error: %s\n", zErrMsg);
                sqlite3_free(&zErrMsg);
                throw new exception("Could not get database version!");
            }

            db_version = sqlite3_column_int(find_property, 0);
            sqlite3_reset(find_property);
        }

        // Free the lot.
        sqlite3_reset(find_table);
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
            "PRIMARY KEY(key, value, network, receiver, sender) "
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

    static int current_db_version = std::end(upgrades) - std::begin(upgrades);
    if (db_version < current_db_version) {
        std::cout << "Will now upgrade database from version " << db_version << " to version " << current_db_version << "." << std::endl;
        for (int i = db_version; i < current_db_version; ++i) {
            sqlite3_exec(conn_, upgrades[i], NULL, NULL, NULL);
        }
        std::cout << "Upgrade completed. Will now update dazeus_version to " << current_db_version << std::endl;
        //w.prepared("update_property")("dazeus_version")(current_db_version)("")("")("").exec();
        //w.commit();
    }
}

std::string SQLiteDatabase::property(const std::string &variable,
      const std::string &networkScope,
      const std::string &receiverScope,
      const std::string &senderScope)
{
/*  // TODO: handle errors
  pqxx::work w(*conn_);
  pqxx::result r = w.prepared("find_property")(variable)(networkScope)(receiverScope)(senderScope).exec();
  if (!r.empty()) {
    return r[0]["value"].as<std::string>();
  }*/
  // TODO: what should we do if there is no result for us?
  return "";
}

void SQLiteDatabase::setProperty(const std::string &variable,
      const std::string &value, const std::string &networkScope,
      const std::string &receiverScope,
      const std::string &senderScope)
{
    /*
  // TODO: handle errors
  pqxx::work w(*conn_);
  if (value == "") {
    w.prepared("remove_property")(variable)(networkScope)(receiverScope)(senderScope).exec();
  } else {
    w.prepared("update_property")(variable)(value)(networkScope)(receiverScope)(senderScope).exec();
  }
  w.commit();*/
}

std::vector<std::string> SQLiteDatabase::propertyKeys(const std::string &ns,
      const std::string &networkScope,
      const std::string &receiverScope,
      const std::string &senderScope)
{
    /*pqxx::work w(*conn_);
    pqxx::result r = w.prepared("properties")(ns)(networkScope)(receiverScope)(senderScope).exec(); */

    // Return a vector of all the property keys matching the criteria.
    std::vector<std::string> keys = std::vector<std::string>();
//    for (auto&& x : r) {
  //      keys.push_back(x["key"].as<std::string>());
    //}
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
//    pqxx::work w(*conn_);
//    pqxx::result r = w.prepared("remove_permission")(perm_name)(network)(receiver)(sender).exec();
//    w.commit();
}

void SQLiteDatabase::setPermission(bool permission, const std::string &perm_name,
      const std::string &network, const std::string &receiver,
      const std::string &sender)
{
//    pqxx::work w(*conn_);
//    pqxx::result r = w.prepared("add_permission")(perm_name)(network)(receiver)(sender).exec();
//    w.commit();
}

}  // namespace db
}  // namespace dazeus
