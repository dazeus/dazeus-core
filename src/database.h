/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#ifndef DATABASE_H
#define DATABASE_H

#include <vector>
#include <string>
#include <stdint.h>

struct DatabaseConfig {
  DatabaseConfig(std::string t = std::string(), std::string h = std::string("127.0.0.1"),
    uint16_t p = 27017, std::string user = std::string(), std::string pass =
    std::string(), std::string db = std::string("dazeus"), std::string opt =
    std::string()) : type(t), hostname(h), port(p), username(user),
    password(pass), database(db), options(opt) {}
  DatabaseConfig(const DatabaseConfig &s) : type(s.type),
    hostname(s.hostname), port(s.port), username(s.username),
    password(s.password), database(s.database), options(s.options) {}
  const DatabaseConfig &operator=(const DatabaseConfig &s) { type = s.type;
    hostname = s.hostname; port = s.port; username = s.username; password =
    s.password; database = s.database; options = s.options; return *this; }

  std::string type;
  std::string hostname;
  uint16_t port;
  std::string username;
  std::string password;
  std::string database;
  std::string options;
};

/**
 * @class Database
 * @brief A database frontend.
 *
 * This frontend abstracts away from the SQL standard, and simply defines the
 * idea of 'properties'. Properties have a unique identifier and a QVariant
 * value. They can be saved inside global, network, channel or user scope.
 *
 * Properties are saved into a table 'properties' which may have multiple
 * entries - one per specific scope. The table is created by calling
 * createTable(). Calling tableExists() checks whether this table exists.
 *
 * @see setProperty()
 */
class Database
{
  public:
          Database( const std::string &hostname, uint16_t port,
                    const std::string &database = std::string("dazeus"),
                    const std::string &username = std::string(),
                    const std::string &password = std::string() );
         ~Database();

    bool            open();
    std::string     lastError() const;

    std::string     property( const std::string &variable,
                              const std::string &networkScope  = std::string(),
                              const std::string &receiverScope = std::string(),
                              const std::string &senderScope   = std::string() );
    void            setProperty( const std::string &variable,
                                 const std::string &value,
                                 const std::string &networkScope  = std::string(),
                                 const std::string &receiverScope = std::string(),
                                 const std::string &senderScope   = std::string());
    std::vector<std::string> propertyKeys( const std::string &ns,
                                  const std::string &networkScope  = std::string(),
                                  const std::string &receiverScope = std::string(),
                                  const std::string &senderScope   = std::string() );

  private:
    // explicitly disable copy constructor
    Database(const Database&);
    void operator=(const Database&);

    void *m_;
    std::string lastError_;
    std::string hostName_;
    std::string databaseName_;
    uint16_t    port_;
    std::string username_;
    std::string password_;
};

#endif
