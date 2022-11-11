/**
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * This file is part of osm2pgsql (https://osm2pgsql.org/).
 *
 * Copyright (C) 2006-2022 by the osm2pgsql developer community.
 * For a full list of authors see the git log.
 */

#include <catch.hpp>

#include "common-buffer.hpp"

#include "geom-from-osm.hpp"
#include "geom-functions.hpp"
#include "geom-output.hpp"
#include "geom.hpp"

#include <array>

TEST_CASE("multipoint_t with a single point", "[NoDB]")
{
    geom::point_t const expected{1, 1};
    geom::point_t point = expected;

    geom::geometry_t geom{geom::multipoint_t{}};
    auto &mp = geom.get<geom::multipoint_t>();
    mp.add_geometry({1, 1});

    REQUIRE(geom.is_multipoint());
    REQUIRE(geometry_type(geom) == "MULTIPOINT");
    REQUIRE(dimension(geom) == 0);
    REQUIRE(num_geometries(geom) == 1);
    REQUIRE(area(geom) == Approx(0.0));
    REQUIRE(length(geom) == Approx(0.0));
    REQUIRE(reverse(geom) == geom);
    REQUIRE(centroid(geom) == geom::geometry_t{point});

    REQUIRE(mp[0] == expected);
}

TEST_CASE("multipoint_t with several points", "[NoDB]")
{
    geom::point_t p0{1, 1};
    geom::point_t p1{2, 1};
    geom::point_t p2{3, 1};

    geom::geometry_t geom{geom::multipoint_t{}};
    auto &mp = geom.get<geom::multipoint_t>();
    mp.add_geometry({1, 1});
    mp.add_geometry({2, 1});
    mp.add_geometry({3, 1});

    REQUIRE(geom.is_multipoint());
    REQUIRE(geometry_type(geom) == "MULTIPOINT");
    REQUIRE(num_geometries(geom) == 3);
    REQUIRE(area(geom) == Approx(0.0));
    REQUIRE(length(geom) == Approx(0.0));
    REQUIRE(reverse(geom) == geom);
    REQUIRE(centroid(geom) == geom::geometry_t{geom::point_t{2, 1}});

    REQUIRE(mp[0] == p0);
    REQUIRE(mp[1] == p1);
    REQUIRE(mp[2] == p2);

    REQUIRE(geometry_n(geom, 1) == geom::geometry_t{p0});
    REQUIRE(geometry_n(geom, 2) == geom::geometry_t{p1});
    REQUIRE(geometry_n(geom, 3) == geom::geometry_t{p2});
}

TEST_CASE("create_multipoint from OSM data", "[NoDB]")
{
    test_buffer_t buffer;
    buffer.add_node("n10 x1 y0");
    buffer.add_way("w20 Nn1x1y1,n2x2y1");
    buffer.add_node("n11 x1 y1");
    buffer.add_node("n12 x3 y2");
    buffer.add_way("w21 Nn3x10y10,n4x10y11");
    buffer.add_node("n13 x3 y1");
    buffer.add_relation("r30 Mw20@");

    auto const geom = geom::create_multipoint(buffer.buffer());

    REQUIRE(geometry_type(geom) == "MULTIPOINT");
    REQUIRE(dimension(geom) == 0);
    REQUIRE(num_geometries(geom) == 4);

    auto const &c = geom.get<geom::multipoint_t>();
    REQUIRE(c[0] == geom::point_t{1, 0});
    REQUIRE(c[1] == geom::point_t{1, 1});
    REQUIRE(c[2] == geom::point_t{3, 2});
    REQUIRE(c[3] == geom::point_t{3, 1});

    REQUIRE(area(geom) == Approx(0.0));
    REQUIRE(length(geom) == Approx(0.0));
    REQUIRE(centroid(geom) == geom::geometry_t{geom::point_t{2, 1}});
}

TEST_CASE("create_multipoint from OSM data with only a single point", "[NoDB]")
{
    test_buffer_t buffer;

    SECTION("only single node in relation")
    {
        buffer.add_node("n10 x1 y0");
    }

    SECTION("two nodes in relation, but one with missing location")
    {
        buffer.add_node("n10 x1 y0");
        buffer.add_node("n11");
    }

    auto const geom = geom::create_multipoint(buffer.buffer());

    REQUIRE(geometry_type(geom) == "POINT");
    REQUIRE(dimension(geom) == 0);
    REQUIRE(num_geometries(geom) == 1);
    REQUIRE(geom.get<geom::point_t>() == geom::point_t{1, 0});
    REQUIRE(area(geom) == Approx(0.0));
    REQUIRE(length(geom) == Approx(0.0));
    REQUIRE(centroid(geom) == geom::geometry_t{geom::point_t{1, 0}});
}
