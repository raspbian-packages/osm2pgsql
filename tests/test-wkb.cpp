/**
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * This file is part of osm2pgsql (https://osm2pgsql.org/).
 *
 * Copyright (C) 2006-2023 by the osm2pgsql developer community.
 * For a full list of authors see the git log.
 */

#include <catch.hpp>

#include "geom.hpp"
#include "wkb.hpp"

TEST_CASE("wkb: nullgeom", "[NoDB]")
{
    geom::geometry_t const geom{};
    REQUIRE(geom.is_null());

    auto const wkb = geom_to_ewkb(geom);
    REQUIRE(wkb.empty());

    auto const result = ewkb_to_geom(wkb);
    REQUIRE(result.is_null());
}

TEST_CASE("wkb: point", "[NoDB]")
{
    geom::geometry_t const geom{geom::point_t{3.14, 2.17}, 42};

    auto const result = ewkb_to_geom(geom_to_ewkb(geom));
    REQUIRE(result.is_point());
    REQUIRE(result.srid() == 42);
    REQUIRE(result == geom);
}

TEST_CASE("wkb: linestring", "[NoDB]")
{
    geom::geometry_t const geom{
        geom::linestring_t{{1.2, 2.3}, {3.4, 4.5}, {5.6, 6.7}}, 43};

    auto const result = ewkb_to_geom(geom_to_ewkb(geom));
    REQUIRE(result.is_linestring());
    REQUIRE(result.srid() == 43);
    REQUIRE(result == geom);
}

TEST_CASE("wkb: polygon without inner ring", "[NoDB]")
{
    geom::geometry_t const geom{
        geom::polygon_t{geom::ring_t{
            {0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}, {0.0, 1.0}, {0.0, 0.0}}},
        44};

    auto const result = ewkb_to_geom(geom_to_ewkb(geom));
    REQUIRE(result.is_polygon());
    REQUIRE(result.srid() == 44);
    REQUIRE(result == geom);
}

TEST_CASE("wkb: polygon with inner rings", "[NoDB]")
{
    geom::geometry_t geom{
        geom::polygon_t{geom::ring_t{
            {0.0, 0.0}, {3.0, 0.0}, {3.0, 3.0}, {0.0, 3.0}, {0.0, 0.0}}},
        45};

    geom.get<geom::polygon_t>().add_inner_ring(geom::ring_t{
        {1.0, 1.0}, {2.0, 1.0}, {2.0, 2.0}, {1.0, 2.0}, {1.0, 1.0}});

    auto const result = ewkb_to_geom(geom_to_ewkb(geom));
    REQUIRE(result.is_polygon());
    REQUIRE(result.srid() == 45);
    REQUIRE(result == geom);
}

TEST_CASE("wkb: point as multipoint", "[NoDB]")
{
    geom::geometry_t const geom{geom::point_t{1.2, 2.3}, 47};

    auto const result = ewkb_to_geom(geom_to_ewkb(geom, true));
    REQUIRE(result.is_multipoint());
    REQUIRE(result.srid() == 47);

    auto const &rmp = result.get<geom::multipoint_t>();
    REQUIRE(rmp.num_geometries() == 1);
    REQUIRE(rmp[0] == geom.get<geom::point_t>());
}

TEST_CASE("wkb: multipoint", "[NoDB]")
{
    geom::geometry_t geom{geom::multipoint_t{}, 46};

    auto &mp = geom.get<geom::multipoint_t>();
    mp.add_geometry(geom::point_t{{1.2, 2.3}});
    mp.add_geometry(geom::point_t{{7.0, 7.0}});

    auto const result = ewkb_to_geom(geom_to_ewkb(geom));
    REQUIRE(result.is_multipoint());
    REQUIRE(result.srid() == 46);
    REQUIRE(result == geom);
}

TEST_CASE("wkb: linestring as multilinestring", "[NoDB]")
{
    geom::geometry_t const geom{
        geom::linestring_t{{1.2, 2.3}, {3.4, 4.5}, {5.6, 6.7}}, 43};

    auto const result = ewkb_to_geom(geom_to_ewkb(geom, true));
    REQUIRE(result.is_multilinestring());
    REQUIRE(result.srid() == 43);

    auto const &rml = result.get<geom::multilinestring_t>();
    REQUIRE(rml.num_geometries() == 1);
    REQUIRE(rml[0] == geom.get<geom::linestring_t>());
}

TEST_CASE("wkb: multilinestring", "[NoDB]")
{
    geom::geometry_t geom{geom::multilinestring_t{}, 46};

    auto &ml = geom.get<geom::multilinestring_t>();
    ml.add_geometry(geom::linestring_t{{1.2, 2.3}, {3.4, 4.5}, {5.6, 6.7}});
    ml.add_geometry(geom::linestring_t{{7.0, 7.0}, {8.0, 7.0}, {8.0, 8.0}});

    auto const result = ewkb_to_geom(geom_to_ewkb(geom));
    REQUIRE(result.is_multilinestring());
    REQUIRE(result.srid() == 46);
    REQUIRE(result == geom);
}

TEST_CASE("wkb: polygon as multipolygon", "[NoDB]")
{
    geom::geometry_t const geom{
        geom::polygon_t{geom::ring_t{
            {0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}, {0.0, 1.0}, {0.0, 0.0}}},
        44};

    auto const result = ewkb_to_geom(geom_to_ewkb(geom, true));
    REQUIRE(result.is_multipolygon());
    REQUIRE(result.srid() == 44);

    auto const &rmp = result.get<geom::multipolygon_t>();
    REQUIRE(rmp.num_geometries() == 1);
    REQUIRE(rmp[0] == geom.get<geom::polygon_t>());
}

TEST_CASE("wkb: multipolygon", "[NoDB]")
{
    geom::geometry_t geom{geom::multipolygon_t{}, 47};

    auto &mp = geom.get<geom::multipolygon_t>();
    {
        auto &p0 = mp.add_geometry(geom::polygon_t{geom::ring_t{
            {0.0, 0.0}, {3.0, 0.0}, {3.0, 3.0}, {0.0, 3.0}, {0.0, 0.0}}});
        p0.add_inner_ring(geom::ring_t{
            {1.0, 1.0}, {2.0, 1.0}, {2.0, 2.0}, {1.0, 2.0}, {1.0, 1.0}});
    }

    mp.add_geometry(geom::polygon_t{geom::ring_t{
        {4.0, 4.0}, {5.0, 4.0}, {5.0, 5.0}, {4.0, 5.0}, {4.0, 4.0}}});

    auto const result = ewkb_to_geom(geom_to_ewkb(geom));
    REQUIRE(result.is_multipolygon());
    REQUIRE(result.srid() == 47);
    REQUIRE(result == geom);
}

TEST_CASE("wkb: geometrycollection", "[NoDB]")
{
    geom::geometry_t geom1{geom::point_t{1.0, 2.0}};
    geom::geometry_t geom2{geom::linestring_t{{1.2, 2.3}, {3.4, 4.5}}};
    geom::geometry_t geom3{geom::multipolygon_t{}};

    geom3.get<geom::multipolygon_t>().add_geometry(geom::polygon_t{geom::ring_t{
        {4.0, 4.0}, {5.0, 4.0}, {5.0, 5.0}, {4.0, 5.0}, {4.0, 4.0}}});

    geom::geometry_t geom{geom::collection_t{}, 49};
    auto &c = geom.get<geom::collection_t>();
    c.add_geometry(std::move(geom1));
    c.add_geometry(std::move(geom2));
    c.add_geometry(std::move(geom3));

    auto const result = ewkb_to_geom(geom_to_ewkb(geom));
    REQUIRE(result.is_collection());
    REQUIRE(result.srid() == 49);
    REQUIRE(result == geom);
}

TEST_CASE("wkb: invalid", "[NoDB]") { REQUIRE_THROWS(ewkb_to_geom("INVALID")); }

TEST_CASE("wkb hex decode of valid hex characters")
{
    REQUIRE(decode_hex_char('0') == 0);
    REQUIRE(decode_hex_char('9') == 9);
    REQUIRE(decode_hex_char('a') == 0x0a);
    REQUIRE(decode_hex_char('f') == 0x0f);
    REQUIRE(decode_hex_char('A') == 0x0a);
    REQUIRE(decode_hex_char('F') == 0x0f);
    REQUIRE_THROWS(decode_hex_char('x'));
}

TEST_CASE("wkb hex decode of valid hex string")
{
    std::string const hex{"0001020F1099FF"};
    std::string const data = {0x00,
                              0x01,
                              0x02,
                              0x0f,
                              0x10,
                              static_cast<char>(0x99),
                              static_cast<char>(0xff)};

    auto const result = decode_hex(hex.c_str());
    REQUIRE(result.size() == hex.size() / 2);
    REQUIRE(result == data);
}

TEST_CASE("wkb hex decode of invalid hex string")
{
    REQUIRE_THROWS(decode_hex("no"));
}

TEST_CASE("wkb hex decode of empty string is okay")
{
    std::string const hex{};
    REQUIRE(decode_hex(hex.c_str()).empty());
}

TEST_CASE("wkb hex decode of string with odd number of characters fails")
{
    REQUIRE_THROWS(decode_hex("a"));
    REQUIRE_THROWS(decode_hex("abc"));
    REQUIRE_THROWS(decode_hex("00000"));
}
