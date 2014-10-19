/**
 * Copyright (c) 2014 Sjors Gielen, Ruben Nijveld, Aaron van Geffen
 * See LICENSE for license.
 */

#ifndef DB_SQLITE_H_
#define DB_SQLITE_H_

#include <sqlite3.h>

#include <stdint.h>
#include <string>
#include <vector>


namespace dazeus {
namespace db {

class SQLiteDatabase : public Database {
 public:
  explicit SQLiteDatabase(DatabaseConfig dbc) : Database(dbc) {}
  ~SQLiteDatabase();

  void open();
  std::string property(const std::string &variable,
                       const std::string &networkScope = "",
                       const std::string &receiverScope = "",
                       const std::string &senderScope = "");
  void setProperty(const std::string &variable, const std::string &value,
                   const std::string &networkScope = "",
                   const std::string &receiverScope = "",
                   const std::string &senderScope = "");
  std::vector<std::string> propertyKeys(
      const std::string &ns,
      const std::string &networkScope = "",
      const std::string &receiverScope = "",
      const std::string &senderScope = "");
  bool hasPermission(const std::string &perm_name, const std::string &network,
                     const std::string &channel, const std::string &sender,
                     bool defaultPermission) const;
  void unsetPermission(const std::string &perm_name, const std::string &network,
                       const std::string &receiver = "",
                       const std::string &sender = "");
  void setPermission(bool permission, const std::string &perm_name,
                     const std::string &network,
                     const std::string &receiver = "",
                     const std::string &sender   = "");
private:
    // explicitly disable copy constructor
    explicit SQLiteDatabase(const SQLiteDatabase&);
    void operator=(const SQLiteDatabase&);

    void bootstrapDB();
    void upgradeDB();

    sqlite3 *conn_;
    sqlite3_stmt *find_table;
    sqlite3_stmt *find_property;
    sqlite3_stmt *remove_property;
    sqlite3_stmt *update_property;
    sqlite3_stmt *properties;
    sqlite3_stmt *add_permission;
    sqlite3_stmt *remove_permission;
    sqlite3_stmt *has_permission;
};

}  // namespace db
}  // namespace dazeus

#endif  // DB_SQLITE_H_
