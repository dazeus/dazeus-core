/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#ifndef DATABASE_H
#define DATABASE_H

#include <vector>
#include <string>
#include <stdint.h>

#ifdef DBA_POSTGRES
#include "pqxx/connection.hxx"
#endif

namespace dazeus {

struct DatabaseConfig {
	DatabaseConfig(std::string t = std::string(),
			std::string h = std::string("127.0.0.1"), uint16_t p = 27017,
			std::string user = std::string(), std::string pass = std::string(),
			std::string file = std::string(), std::string db = std::string("dazeus"),
			std::string opt = std::string()) :
			type(t), hostname(h), port(p), username(user), password(pass), database(
					db), options(opt) {
	}

	DatabaseConfig(const DatabaseConfig &s) :
			type(s.type), hostname(s.hostname), port(s.port), username(s.username), password(
					s.password), database(s.database), options(s.options) {
	}

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
class Database {
public:
    Database(DatabaseConfig dbc) : dbc_(dbc) {}
    virtual ~Database() {}

	struct exception : public std::runtime_error {
    exception(std::string error) : std::runtime_error(error) {}
  };

	virtual void open() = 0;
	virtual std::string property(const std::string &variable,
			const std::string &networkScope = std::string(),
			const std::string &receiverScope = std::string(),
			const std::string &senderScope = std::string()) = 0;
	virtual void setProperty(const std::string &variable,
			const std::string &value, const std::string &networkScope = std::string(),
			const std::string &receiverScope = std::string(),
			const std::string &senderScope = std::string()) = 0;
	virtual std::vector<std::string> propertyKeys(const std::string &ns,
			const std::string &networkScope = std::string(),
			const std::string &receiverScope = std::string(),
			const std::string &senderScope = std::string()) = 0;
	virtual bool hasPermission(const std::string &perm_name,
			const std::string &network, const std::string &channel,
			const std::string &sender, bool defaultPermission) const = 0;
	virtual void unsetPermission(const std::string &perm_name,
			const std::string &network, const std::string &receiver = std::string(),
			const std::string &sender   = std::string()) = 0;
	virtual void setPermission(bool permission, const std::string &perm_name,
			const std::string &network, const std::string &receiver = std::string(),
			const std::string &sender   = std::string()) = 0;
protected:
	DatabaseConfig dbc_;
};

#ifdef DBA_POSTGRES
class PostgreSQLDatabase: public Database {
public:
    PostgreSQLDatabase(DatabaseConfig dbc) : Database(dbc) {}
	~PostgreSQLDatabase();

	void open();
	std::string property(const std::string &variable,
				const std::string &networkScope = std::string(),
				const std::string &receiverScope = std::string(),
				const std::string &senderScope = std::string());
	void setProperty(const std::string &variable,
				const std::string &value, const std::string &networkScope = std::string(),
				const std::string &receiverScope = std::string(),
				const std::string &senderScope = std::string());
	std::vector<std::string> propertyKeys(const std::string &ns,
				const std::string &networkScope = std::string(),
				const std::string &receiverScope = std::string(),
				const std::string &senderScope = std::string());
	bool hasPermission(const std::string &perm_name,
				const std::string &network, const std::string &channel,
				const std::string &sender, bool defaultPermission) const;
	void unsetPermission(const std::string &perm_name,
				const std::string &network, const std::string &receiver = std::string(),
				const std::string &sender   = std::string());
	void setPermission(bool permission, const std::string &perm_name,
				const std::string &network, const std::string &receiver = std::string(),
				const std::string &sender   = std::string());
private:
	// explicitly disable copy constructor
	PostgreSQLDatabase(const PostgreSQLDatabase&);
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
#endif

#ifdef DBA_MONGO
class MongoDatabase: public Database {
public:
    MongoDatabase(DatabaseConfig dbc) : Database(dbc) {}
	~MongoDatabase();

	void open();
	std::string property(const std::string &variable,
				const std::string &networkScope = std::string(),
				const std::string &receiverScope = std::string(),
				const std::string &senderScope = std::string());
	void setProperty(const std::string &variable,
				const std::string &value, const std::string &networkScope = std::string(),
				const std::string &receiverScope = std::string(),
				const std::string &senderScope = std::string());
	std::vector<std::string> propertyKeys(const std::string &ns,
				const std::string &networkScope = std::string(),
				const std::string &receiverScope = std::string(),
				const std::string &senderScope = std::string());
	bool hasPermission(const std::string &perm_name,
				const std::string &network, const std::string &channel,
				const std::string &sender, bool defaultPermission) const;
	void unsetPermission(const std::string &perm_name,
				const std::string &network, const std::string &receiver = std::string(),
				const std::string &sender   = std::string());
	void setPermission(bool permission, const std::string &perm_name,
				const std::string &network, const std::string &receiver = std::string(),
				const std::string &sender   = std::string());
private:
	// explicitly disable copy constructor
	MongoDatabase(const MongoDatabase&);
	void operator=(const MongoDatabase&);

	void *m_;
	std::string lastError_;
	std::string hostName_;
	std::string databaseName_;
	uint16_t port_;
	std::string username_;
	std::string password_;
};
#endif

}

#endif
