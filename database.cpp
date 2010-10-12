/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#include "database.h"

/**
 * @brief Constructor.
 */
Database::Database( const QString &network )
: network_( network.toLower() )
{
}


/**
 * @brief Destructor.
 */
Database::~Database()
{
}

/**
 * @brief Returns the network for which this Database was created.
 */
const QString &Database::network() const
{
  return network_;
}
