/**
 * Copyright (c) 2014 Sjors Gielen, Ruben Nijveld, Aaron van Geffen
 * See LICENSE for license.
 */

#ifndef DB_POSTGRES_H_
#define DB_POSTGRES_H_

#include <stdint.h>
#include <string>
#include <vector>

#include <pqxx/connection.hxx>
#include "database.h"

namespace dazeus {
namespace db {

class PostgreSQLDatabase : public Database {
 public:
  explicit PostgreSQLDatabase(DatabaseConfig dbc) : Database(dbc) {}
  ~PostgreSQLDatabase();

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
  explicit PostgreSQLDatabase(const PostgreSQLDatabase&);
  void operator=(const PostgreSQLDatabase&);

  void bootstrapDB();
  void upgradeDB();

  pqxx::connection *conn_;
  std::string hostName_;
  std::string databaseName_;
  uint16_t port_;
  std::string username_;
  std::string password_;
};

}  // namespace db
}  // namespace dazeus

#endif  // DB_POSTGRES_H_
