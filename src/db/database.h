/**
 * Copyright (c) 2014 Sjors Gielen, Ruben Nijveld, Aaron van Geffen
 * See LICENSE for license.
 */

#ifndef DB_DATABASE_H_
#define DB_DATABASE_H_

#include <stdint.h>
#include <vector>
#include <string>
#include <stdexcept>

namespace dazeus {
namespace db {

/**
 * @brief Database configuration container
 */
struct DatabaseConfig {
  DatabaseConfig(const std::string &t = "", const std::string &h = "127.0.0.1",
                 uint16_t p = 27017, const std::string &user = "",
                 const std::string &pass = "", const std::string &fname = "",
                 const std::string &db = "dazeus",
                 const std::string &opt = "")
      : type(t), hostname(h), port(p), username(user), password(pass),
        filename(fname), database(db), options(opt) {}

  DatabaseConfig(const DatabaseConfig &s)
      : type(s.type), hostname(s.hostname), port(s.port), username(s.username),
        password(s.password), filename(s.filename), database(s.database),
        options(s.options) {}

  const DatabaseConfig &operator=(const DatabaseConfig &s) {
    type = s.type;
    hostname = s.hostname;
    port = s.port;
    username = s.username;
    password = s.password;
    filename = s.filename;
    database = s.database;
    options = s.options;
    return *this;
  }

  std::string type;
  std::string hostname;
  uint16_t port;
  std::string username;
  std::string password;
  std::string filename;
  std::string database;
  std::string options;
};

/**
 * @class Database
 * @brief A database frontend.
 *
 * This frontend abstracts away from the database used, and simply defines the
 * idea of 'properties' and 'permissions'. Properties consist of a key-value
 * pair, whereas permissions only have assigned a boolean for each key.
 * Properties and permissions can be saved inside global, network, channel or
 * user scope.
 *
 * @see setProperty()
 */
class Database {
 public:
    explicit Database(DatabaseConfig dbc) : dbc_(dbc) {}
    virtual ~Database() {}

  virtual void open() = 0;
  virtual std::string property(const std::string &variable,
                               const std::string &networkScope = "",
                               const std::string &receiverScope = "",
                               const std::string &senderScope = "") = 0;
  virtual void setProperty(const std::string &variable,
                           const std::string &value,
                           const std::string &networkScope = "",
                           const std::string &receiverScope = "",
                           const std::string &senderScope = "") = 0;
  virtual std::vector<std::string> propertyKeys(
      const std::string &ns, const std::string &networkScope = "",
      const std::string &receiverScope = "",
      const std::string &senderScope = "") = 0;
  virtual bool hasPermission(const std::string &perm_name,
                             const std::string &network,
                             const std::string &channel,
                             const std::string &sender,
                             bool defaultPermission) const = 0;
  virtual void unsetPermission(const std::string &perm_name,
                               const std::string &network,
                               const std::string &receiver = "",
                               const std::string &sender = "") = 0;
  virtual void setPermission(bool permission, const std::string &perm_name,
                             const std::string &network,
                             const std::string &receiver = "",
                             const std::string &sender = "") = 0;

 protected:
  DatabaseConfig dbc_;
};

struct exception : public std::runtime_error {
  explicit exception(const std::string &error) : std::runtime_error(error) {}
};

}  // namespace db
}  // namespace dazeus

#endif  // DB_DATABASE_H_
