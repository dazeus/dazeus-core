/**
 * Copyright (c) 2014 Sjors Gielen, Ruben Nijveld, Aaron van Geffen
 * See LICENSE for license.
 */

#ifndef DB_MONGO_H_
#define DB_MONGO_H_

#include "database.h"
#include <stdint.h>
#include <string>
#include <vector>

namespace dazeus {
namespace db {

class MongoDatabase : public Database {
 public:
  explicit MongoDatabase(DatabaseConfig dbc) : Database(dbc) {}
  ~MongoDatabase();

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
  explicit MongoDatabase(const MongoDatabase&);
  void operator=(const MongoDatabase&);

  void *m_;
  std::string lastError_;
  std::string hostName_;
  std::string databaseName_;
  uint16_t port_;
  std::string username_;
  std::string password_;
};

}  // namespace db
}  // namespace dazeus

#endif  // DB_MONGO_H_
