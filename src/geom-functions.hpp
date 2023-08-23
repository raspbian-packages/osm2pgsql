#ifndef OSM2PGSQL_GEOM_FUNCTIONS_HPP
#define OSM2PGSQL_GEOM_FUNCTIONS_HPP

/**
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * This file is part of osm2pgsql (https://osm2pgsql.org/).
 *
 * Copyright (C) 2006-2023 by the osm2pgsql developer community.
 * For a full list of authors see the git log.
 */

#include "geom.hpp"
#include "reprojection.hpp"

#include <string_view>
#include <utility>
#include <vector>

/**
 * \file
 *
 * Functions on geometries.
 */

namespace geom {

/// Calculate the Euclidean distance between two points
double distance(point_t p1, point_t p2) noexcept;

/**
 * Calculate a point on the vector p1 -> p2 that is a fraction of the distance
 * between those points.
 */
point_t interpolate(point_t p1, point_t p2, double frac) noexcept;

/**
 * Iterate over all segments (connections between two points) in a point list
 * and call a function with both points.
 *
 * \pre \code !list.empty() \endcode
 */
template <typename FUNC>
void for_each_segment(point_list_t const &list, FUNC &&func)
{
    assert(!list.empty());
    auto it = list.cbegin();
    auto prev = it;
    for (++it; it != list.cend(); ++it) {
        std::forward<FUNC>(func)(*prev, *it);
        prev = it;
    }
}

/**
 * Return the type of a geometry as string: NULL, (MULTI)POINT,
 * (MULTI)LINESTRING, (MULTI)POLYGON, or GEOMETRYCOLLECTION.
 */
std::string_view geometry_type(geometry_t const &geom);

/**
 * Return the number of geometries in this geometry. For the null geometry
 * this is always 0, for other non-multi geometries this is always 1.
 */
std::size_t num_geometries(geometry_t const &geom);

/**
 * Get a copy of the nth geometry out of a (multi)geometry.
 *
 * For non-multi geometries and n==1 a copy of the single geometry is returned.
 * If n is out or range a null geometry is returned.
 *
 * \param output Pointer to output geometry.
 * \param input Input geometry.
 * \param n Number of geometry to get (1-based).
 */
void geometry_n(geometry_t *output, geometry_t const &input, std::size_t n);

/**
 * Get a copy of the nth geometry out of a (multi)geometry.
 *
 * For non-multi geometries and n==1 a copy of the single geometry is returned.
 * If n is out or range a null geometry is returned.
 *
 * \param input Input geometry.
 * \param n Number of geometry to get (1-based).
 * \returns The geometry.
 */
geometry_t geometry_n(geometry_t const &input, std::size_t n);

/**
 * Transform a geometry in 4326 into some other projection.
 *
 * \param output Pointer to output geometry.
 * \param input Input geometry.
 * \param reprojection Target projection.
 *
 * \pre \code geom.srid() == 4326 \endcode
 */
void transform(geometry_t *output, geometry_t const &input,
               reprojection const &reprojection);

/**
 * Transform a geometry in 4326 into some other projection.
 *
 * \param input Input geometry.
 * \param reprojection Target projection.
 * \returns Reprojected geometry.
 *
 * \pre \code geom.srid() == 4326 \endcode
 */
geometry_t transform(geometry_t const &input, reprojection const &reprojection);

/**
 * Returns a modified geometry having no segment longer than the given
 * max_segment_length.
 *
 * \param output Pointer to output geometry. Will be a multilinestring
 *        geometry, or nullgeom_t on error.
 * \param input The input geometry, must be a linestring or multilinestring.
 * \param max_segment_length The maximum length (using Euclidean distance
 *        in the length unit of the SRS of the geometry) of each resulting
 *        linestring.
 */
void segmentize(geometry_t *output, geometry_t const &input,
                double max_segment_length);

/**
 * Returns a modified geometry having no segment longer than the given
 * max_segment_length.
 *
 * \param input The input geometry, must be a linestring or multilinestring.
 * \param max_segment_length The maximum length (using Euclidean distance
 *        in the length unit of the SRS of the geometry) of each resulting
 *        linestring.
 * \returns Resulting multilinestring geometry, nullgeom_t on error.
 */
geometry_t segmentize(geometry_t const &input, double max_segment_length);

/**
 * Calculate area of geometry.
 * For geometry types other than polygon or multipolygon this will always
 * return 0.
 *
 * \param geom Input geometry.
 * \returns Area.
 */
double area(geometry_t const &geom);

/**
 * Calculate area of geometry on the spheroid.
 * For geometry types other than polygon or multipolygon this will always
 * return 0.
 *
 * \param geom Input geometry.
 * \returns Area in m².
 * \pre \code geom.srid() == 4326 \endcode
 */
double spherical_area(geometry_t const &geom);

/**
 * Split multigeometries into their parts. Non-multi geometries are left
 * alone and will end up as the only geometry in the result vector. If the
 * input geometry is a nullgeom_t, the result vector will be empty.
 *
 * \param geom Input geometry (will be moved from).
 * \param split_multi Only split of this is set to true.
 * \returns Vector of result geometries.
 */
std::vector<geometry_t> split_multi(geometry_t &&geom, bool split_multi = true);

/**
 * Calculate length of geometry.
 * For geometry types other than linestring or multilinestring this will always
 * return 0.
 *
 * \param geom Input geometry.
 * \returns Length.
 **/
double length(geometry_t const &geom);

/**
 * Reverses the order of the vertices in geometry.
 *
 * \param output Pointer to output geometry.
 * \param input Input geometry.
 */
void reverse(geometry_t *output, geometry_t const &input);

/**
 * Reverses the order of the vertices in geometry.
 *
 * \param input Input geometry.
 * \returns Result geometry.
 */
geometry_t reverse(geometry_t const &input);

/**
 * Merge lines in a multilinestring end-to-end as far as possible.
 *
 * * Returns a multilinestring if the input is a multilinestring.
 * * Returns a copy of the input if it is a linestring.
 * * Returns nullgeom_t otherwise.
 *
 * \param output Pointer to output geometry.
 * \param input Input geometry.
 */
void line_merge(geometry_t *output, geometry_t const &input);

/**
 * Merge lines in a multilinestring end-to-end as far as possible.
 *
 * * Returns a multilinestring if the input is a multilinestring.
 * * Returns a copy of the input if it is a linestring.
 * * Returns nullgeom_t otherwise.
 *
 * \param input Input geometry.
 * \returns Result multilinestring.
 */
geometry_t line_merge(geometry_t const &input);

/**
 * Calculate the centroid of a geometry.
 *
 * \param geom Input geometry.
 * \returns Resulting point geometry.
 */
geometry_t centroid(geometry_t const &geom);

/**
 * Simplify a linestring geometry using Douglas-Peucker algorithm.
 *
 * Geometries might become invalid by using simplify. The simplification
 * process might create self-intersections.
 *
 * Returns null geometry for anything but linestring geometries!
 *
 * \param output Pointer to output geometry.
 * \param input Input geometry.
 * \param tolerance Max distance (in units of input coordinates) of a point
 *                  to other segments to be removed.
 */
void simplify(geometry_t *output, geometry_t const &input, double tolerance);

/**
 * Simplify a linestring geometry using Douglas-Peucker algorithm.
 *
 * Geometries might become invalid by using simplify. The simplification
 * process might create self-intersections.
 *
 * Returns null geometry for anything but linestring geometries!
 *
 * \param input Input geometry.
 * \param tolerance Max distance (in units of input coordinates) of a point
 *                  to other segments to be removed.
 * \returns Simplified geometry.
 */
geometry_t simplify(geometry_t const &input, double tolerance);

} // namespace geom

#endif // OSM2PGSQL_GEOM_FUNCTIONS_HPP
