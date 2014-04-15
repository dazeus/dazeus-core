/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#ifndef DATABASE_H
#define DATABASE_H

#include <vector>
#include <string>
#include <stdint.h>

namespace dazeus {

struct DatabaseConfig {
  DatabaseConfig() : hostname("127.0.0.1"), port(27017), database("dazeus") {}

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
          Database(DatabaseConfig dbc);
         ~Database();

    struct exception : public std::runtime_error {
        exception(std::string error) : std::runtime_error(error) {}
    };

    // Connect or re-connect to the database. Throws an exception if connecting failed.
    void            open();

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

    bool            hasPermission(const std::string &perm_name,
                                  const std::string &network,
                                  const std::string &channel,
                                  const std::string &sender,
                                  bool defaultPermission) const;
    void          unsetPermission(const std::string &perm_name,
                                  const std::string &network,
                                  const std::string &receiver = std::string(),
                                  const std::string &sender   = std::string());
    void            setPermission(bool permission,
                                  const std::string &perm_name,
                                  const std::string &network,
                                  const std::string &receiver = std::string(),
                                  const std::string &sender   = std::string());

  private:
    // explicitly disable copy constructor
    Database(const Database&);
    void operator=(const Database&);

    void *m_;
    DatabaseConfig dbc_;
};

}

#endif
