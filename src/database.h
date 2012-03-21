/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#ifndef DATABASE_H
#define DATABASE_H

#include <vector>
#include <string>

struct DatabaseConfig;

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
    void *m_;
    std::string lastError_;
    std::string hostName_;
    std::string databaseName_;
    uint16_t    port_;
    std::string username_;
    std::string password_;
};

#endif
