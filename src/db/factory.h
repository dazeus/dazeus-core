#ifndef DB_FACTORY_H_
#define DB_FACTORY_H_

#include "database.h"

#ifdef DB_POSTGRES
#include "postgres.h"
#endif

#ifdef DB_SQLITE
#include "sqlite.h"
#endif

#ifdef DB_MONGO
#include "mongo.h"
#endif

namespace dazeus {
namespace db {

class Factory {
 public:
  static Database *createDb(const DatabaseConfig &dbc)
  {
    Database *instance = NULL;

    #ifdef DB_POSTGRES
    if (dbc.type == "postgres" || dbc.type == "postgresql"
        || dbc.type == "pq" || dbc.type == "psql") {
      instance = new PostgreSQLDatabase(dbc);
    }
    #endif

    #ifdef DB_SQLITE
    if (dbc.type == "sqlite" || dbc.type == "sqlite3") {
      instance = new SQLiteDatabase(dbc);
    }
    #endif

    #ifdef DB_MONGO
    if (dbc.type == "mongo" || dbc.type == "mongodb") {
      instance = new MongoDatabase(dbc);
    }
    #endif

    if (!instance) {
      throw dazeus::db::exception(
          "Database of type '" + dbc.type + "' not supported");
    }

    return instance;
  }
};

}  // namespace db
}  // namespace dazeus

#endif  // DB_FACTORY_H_
