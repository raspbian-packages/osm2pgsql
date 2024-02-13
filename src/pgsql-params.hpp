#ifndef OSM2PGSQL_PGSQL_PARAMS_HPP
#define OSM2PGSQL_PGSQL_PARAMS_HPP

/**
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * This file is part of osm2pgsql (https://osm2pgsql.org/).
 *
 * Copyright (C) 2006-2024 by the osm2pgsql developer community.
 * For a full list of authors see the git log.
 */

#include "format.hpp"

#include <map>
#include <string>
#include <utility>
#include <vector>

/**
 * PostgreSQL connection parameters.
 */
class connection_params_t
{
public:
    connection_params_t() { m_params["client_encoding"] = "UTF8"; }

    void set(std::string const &param, std::string const &value)
    {
        m_params[param] = value;
    }

    auto begin() const noexcept { return m_params.begin(); }

    auto end() const noexcept { return m_params.end(); }

private:
    std::map<std::string, std::string> m_params;

}; // class connection_params_t

#endif // OSM2PGSQL_PGSQL_PARAMS_HPP
