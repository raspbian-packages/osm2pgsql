#ifndef OSM2PGSQL_GEOM_FROM_OSM_HPP
#define OSM2PGSQL_GEOM_FROM_OSM_HPP

/**
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * This file is part of osm2pgsql (https://osm2pgsql.org/).
 *
 * Copyright (C) 2006-2022 by the osm2pgsql developer community.
 * For a full list of authors see the git log.
 */

#include "geom.hpp"

#include <osmium/fwd.hpp>
#include <osmium/memory/buffer.hpp>

/**
 * \file
 *
 * Functions to create geometries from OSM data.
 *
 * All functions here are available in two versions, one taking a pointer
 * to an existing geometry as output parameter, the second which returns
 * a new geometry instead.
 */

namespace geom {

/**
 * Create a point geometry from a node.
 *
 * \param geom Pointer to an existing geometry which will be used as output.
 * \param node The input node.
 */
void create_point(geometry_t *geom, osmium::Node const &node);

/**
 * Create a point geometry from a node.
 *
 * \param node The input node.
 * \returns The created geometry.
 */
[[nodiscard]] geometry_t create_point(osmium::Node const &node);

/**
 * Create a linestring geometry from a way. Nodes without location are ignored.
 * Consecutive nodes with the same location will only end up once in the
 * linestring.
 *
 * If the resulting linestring would be invalid (< 1 nodes), a null geometry
 * is created.
 *
 * \param geom Pointer to an existing geometry which will be used as output.
 * \param way The input way.
 */
void create_linestring(geometry_t *geom, osmium::Way const &way);

/**
 * Create a linestring geometry from a way. Nodes without location are ignored.
 * Consecutive nodes with the same location will only end up once in the
 * linestring.
 *
 * If the resulting linestring would be invalid (< 1 nodes), a null geometry
 * is returned.
 *
 * \param way The input way.
 * \returns The created geometry.
 */
[[nodiscard]] geometry_t create_linestring(osmium::Way const &way);

/**
 * Create a polygon geometry from a way.
 *
 * If the resulting polygon would be invalid, a null geometry is returned.
 *
 * \param geom Pointer to an existing geometry which will be used as output.
 * \param way The input way.
 */
void create_polygon(geometry_t *geom, osmium::Way const &way);

/**
 * Create a polygon geometry from a way.
 *
 * If the resulting polygon would be invalid, a null geometry is returned.
 *
 * \param way The input way.
 * \returns The created geometry.
 */
[[nodiscard]] geometry_t create_polygon(osmium::Way const &way);

/**
 * Create a multilinestring geometry from a bunch of ways (usually this
 * would be used for member ways of a relation). The result is always a
 * multilinestring, even if it only contains one linestring.
 *
 * If the resulting multilinestring would be invalid, a null geometry is
 * returned.
 *
 * \param geom Pointer to an existing geometry which will be used as output.
 * \param ways Buffer containing all the input ways.
 */
void create_multilinestring(geometry_t *geom,
                            osmium::memory::Buffer const &ways,
                            bool force_multi = true);

/**
 * Create a multilinestring geometry from a bunch of ways (usually this
 * would be used for member ways of a relation). The result is always a
 * multilinestring, even if it only contains one linestring.
 *
 * If the resulting multilinestring would be invalid, a null geometry is
 * returned.
 *
 * \param ways Buffer containing all the input ways.
 * \returns The created geometry.
 */
[[nodiscard]] geometry_t
create_multilinestring(osmium::memory::Buffer const &ways,
                       bool force_multi = true);

/**
 * Create a (multi)polygon geometry from a relation and member ways.
 *
 * If the resulting (multi)polygon would be invalid, a null geometry is
 * returned.
 *
 * \param geom Pointer to an existing geometry which will be used as output.
 * \param relation The input relation.
 * \param way_buffer Buffer containing all member ways.
 */
void create_multipolygon(geometry_t *geom, osmium::Relation const &relation,
                         osmium::memory::Buffer const &way_buffer);

/**
 * Create a (multi)polygon geometry from a relation and member ways.
 *
 * If the resulting (multi)polygon would be invalid, a null geometry is
 * returned.
 *
 * \param relation The input relation.
 * \param way_buffer Buffer containing all member ways.
 * \returns The created geometry.
 */
[[nodiscard]] geometry_t
create_multipolygon(osmium::Relation const &relation,
                    osmium::memory::Buffer const &way_buffer);

/**
 * Create a geometry collection from a relation and node/way members.
 *
 * If the resulting geometry would be empty or invalid, a null geometry is
 * returned.
 *
 * \param geom Pointer to an existing geometry which will be used as output.
 * \param relation The input relation.
 * \param way_buffer Buffer containing all member nodes and ways.
 */
void create_collection(geometry_t *geom,
                       osmium::memory::Buffer const &member_buffer);

/**
 * Create a geometry collection from a relation and node/way members.
 *
 * If the resulting geometry would be empty or invalid, a null geometry is
 * returned.
 *
 * \param way_buffer Buffer containing all member nodes and ways.
 * \returns The created geometry.
 */
[[nodiscard]] geometry_t
create_collection(osmium::memory::Buffer const &member_buffer);

} // namespace geom

#endif // OSM2PGSQL_GEOM_FROM_OSM_HPP
