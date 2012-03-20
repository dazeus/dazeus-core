/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#ifndef DATABASE_H
#define DATABASE_H

#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>

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
          Database( const QString &dbType,
                    const QString &databaseName, const QString &username,
                    const QString &password, const QString &hostname,
                    int port, const QString &options );
         ~Database();

    static Database *fromConfig(const DatabaseConfig *dbc);
    static QLatin1String typeToQtPlugin(const QString &type);

    bool            open();
    void            checkDatabaseConnection();

    bool            createTable();
    bool            tableExists();
    QSqlError       lastError() const;

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
    QSqlDatabase db_;

};

#endif
