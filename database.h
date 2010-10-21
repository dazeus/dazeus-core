/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#ifndef DATABASE_H
#define DATABASE_H

#include <QtCore/QObject>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>

class QVariant;
struct DatabaseConfig;

enum DatabaseScope
{
  UnknownScope = 0x0,
  NetworkScope = 0x1,
  ChannelScope = 0x2,
  UserScope    = 0x3
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
class Database : public QObject
{
  Q_OBJECT

  public:
          Database( const QString &network, const QString &dbType,
                    const QString &databaseName, const QString &username,
                    const QString &password, const QString &hostname,
                    int port, const QString &options );
         ~Database();

    static Database *fromConfig(const DatabaseConfig *dbc);
    static QString   typeToQtPlugin(const QString &type);

    const QString  &network() const;
    bool            open();

    bool            createTable();
    bool            tableExists() const;
    QSqlError       lastError() const;

    QVariant    property( const QString &variable,
                          const QString &userScope = QString(),
                          const QString &channelScope = QString() ) const;
    void        setProperty( const QString &variable,
                             const QVariant &value,
                             const QString &userScope = QString(),
                             const QString &channelScope = QString());

  public slots:


  private:
    QString network_;
    QSqlDatabase db_;

};

#endif
