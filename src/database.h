/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#ifndef DATABASE_H
#define DATABASE_H

#include <QtCore/QStringList>
#include <QtCore/QVariant>

class QVariant;
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
          Database( const std::string &hostname, uint16_t port, const std::string &database = std::string("dazeus"), const std::string &username = std::string(), const std::string &password = std::string() );
         ~Database();

    bool            open();

    std::string     lastError() const;

    QVariant    property( const QString &variable,
                          const QString &networkScope  = QString(),
                          const QString &receiverScope = QString(),
                          const QString &senderScope   = QString() );
    void        setProperty( const QString &variable,
                             const QVariant &value,
                             const QString &networkScope  = QString(),
                             const QString &receiverScope = QString(),
                             const QString &senderScope   = QString());
    QStringList propertyKeys( const QString &ns,
                              const QString &networkScope  = QString(),
                              const QString &receiverScope = QString(),
                              const QString &senderScope   = QString() );

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
