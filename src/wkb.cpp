/**
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * This file is part of osm2pgsql (https://osm2pgsql.org/).
 *
 * Copyright (C) 2006-2024 by the osm2pgsql developer community.
 * For a full list of authors see the git log.
 */

#include "wkb.hpp"

#include "format.hpp"
#include "overloaded.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace ewkb {

namespace {

enum geometry_type : uint32_t
{
    wkb_point = 1,
    wkb_line = 2,
    wkb_polygon = 3,
    wkb_multi_point = 4,
    wkb_multi_line = 5,
    wkb_multi_polygon = 6,
    wkb_collection = 7,

    wkb_srid = 0x20000000 // SRID-presence flag (EWKB)
};

enum wkb_byte_order_type_t : uint8_t
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
    Endian = 1 // Little Endian
#else
    Endian = 0, // Big Endian
#endif
};

template <typename T>
void str_push(std::string *data, T value)
{
    data->append(reinterpret_cast<char const *const>(&value), sizeof(T));
}

/**
 * Add a EWKB header without length field to the string.
 *
 * This header is always 1 + 4 + 4 = 9 bytes long
 *
 * \pre \code data != nullptr \endcode
 */
void write_header(std::string *data, geometry_type type, uint32_t srid)
{
    str_push(data, Endian);
    if (srid) {
        str_push(data, type | geometry_type::wkb_srid);
        str_push(data, srid);
    } else {
        str_push(data, type);
    }
}

/**
 * Add a EWKB 32bit unsigned int length field to the string.
 *
 * This header is always 4 bytes long
 *
 * \pre \code data != nullptr \endcode
 */
void write_length(std::string *data, std::size_t length)
{
    str_push(data, static_cast<uint32_t>(length));
}

void write_point(std::string *data, geom::point_t const &geom,
                 uint32_t srid = 0)
{
    assert(data);

    write_header(data, wkb_point, srid);
    str_push(data, geom.x());
    str_push(data, geom.y());
}

void write_points(std::string *data, geom::point_list_t const &points)
{
    write_length(data, points.size());
    for (auto const &point : points) {
        str_push(data, point.x());
        str_push(data, point.y());
    }
}

void write_linestring(std::string *data, geom::linestring_t const &geom,
                      uint32_t srid = 0)
{
    assert(data);

    write_header(data, wkb_line, srid);
    write_points(data, geom);
}

void write_polygon(std::string *data, geom::polygon_t const &geom,
                   uint32_t srid = 0)
{
    assert(data);

    write_header(data, wkb_polygon, srid);
    write_length(data, geom.inners().size() + 1);
    write_points(data, geom.outer());
    for (auto const &ring : geom.inners()) {
        write_points(data, ring);
    }
}

void write_multipoint(std::string *data, geom::multipoint_t const &geom,
                      uint32_t srid = 0)
{
    assert(data);

    write_header(data, wkb_multi_point, srid);
    write_length(data, geom.num_geometries());

    for (auto const &point : geom) {
        write_point(data, point);
    }
}

void write_multilinestring(std::string *data,
                           geom::multilinestring_t const &geom,
                           uint32_t srid = 0)
{
    assert(data);

    write_header(data, wkb_multi_line, srid);
    write_length(data, geom.num_geometries());

    for (auto const &line : geom) {
        write_linestring(data, line);
    }
}

void write_multipolygon(std::string *data, geom::multipolygon_t const &geom,
                        uint32_t srid = 0)
{
    assert(data);

    write_header(data, wkb_multi_polygon, srid);
    write_length(data, geom.num_geometries());

    for (auto const &polygon : geom) {
        write_polygon(data, polygon);
    }
}

void write_collection(std::string *data, geom::collection_t const &geom,
                      uint32_t srid = 0)
{
    assert(data);

    write_header(data, wkb_collection, srid);
    write_length(data, geom.num_geometries());

    for (auto const &item : geom) {
        item.visit(overloaded{
            [&](geom::nullgeom_t const & /*input*/) {},
            [&](geom::point_t const &input) { write_point(data, input); },
            [&](geom::linestring_t const &input) {
                write_linestring(data, input);
            },
            [&](geom::polygon_t const &input) { write_polygon(data, input); },
            [&](geom::multipoint_t const &input) {
                write_multipoint(data, input);
            },
            [&](geom::multilinestring_t const &input) {
                write_multilinestring(data, input);
            },
            [&](geom::multipolygon_t const &input) {
                write_multipolygon(data, input);
            },
            [&](geom::collection_t const &input) {
                write_collection(data, input);
            }});
    }
}

class make_ewkb_visitor
{
public:
    make_ewkb_visitor(uint32_t srid, bool ensure_multi) noexcept
    : m_srid(srid), m_ensure_multi(ensure_multi)
    {}

    std::string operator()(geom::nullgeom_t const & /*geom*/) const
    {
        return {};
    }

    std::string operator()(geom::point_t const &geom) const
    {
        std::string data;

        if (m_ensure_multi) {
            write_header(&data, wkb_multi_point, m_srid);
            write_length(&data, 1);
            write_point(&data, geom);
        } else {
            // 9 byte header plus one set of coordinates
            constexpr const std::size_t size = 9 + 2 * 8;
            data.reserve(size);
            write_point(&data, geom, m_srid);
            assert(data.size() == size);
        }

        return data;
    }

    std::string operator()(geom::linestring_t const &geom) const
    {
        std::string data;

        if (m_ensure_multi) {
            // Two 13 bytes headers plus n sets of coordinates
            data.reserve(2UL * 13UL + geom.size() * (2UL * 8UL));
            write_header(&data, wkb_multi_line, m_srid);
            write_length(&data, 1);
            write_linestring(&data, geom);
        } else {
            // 13 byte header plus n sets of coordinates
            data.reserve(13UL + geom.size() * (2UL * 8UL));
            write_linestring(&data, geom, m_srid);
        }

        return data;
    }

    std::string operator()(geom::polygon_t const &geom) const
    {
        std::string data;

        if (m_ensure_multi) {
            write_header(&data, wkb_multi_polygon, m_srid);
            write_length(&data, 1);
            write_polygon(&data, geom);
        } else {
            write_polygon(&data, geom, m_srid);
        }

        return data;
    }

    std::string operator()(geom::multipoint_t const &geom) const
    {
        std::string data;
        write_multipoint(&data, geom, m_srid);
        return data;
    }

    std::string operator()(geom::multilinestring_t const &geom) const
    {
        std::string data;
        write_multilinestring(&data, geom, m_srid);
        return data;
    }

    std::string operator()(geom::multipolygon_t const &geom) const
    {
        std::string data;
        write_multipolygon(&data, geom, m_srid);
        return data;
    }

    std::string operator()(geom::collection_t const &geom) const
    {
        std::string data;
        write_collection(&data, geom, m_srid);
        return data;
    }

private:
    uint32_t m_srid;
    bool m_ensure_multi;

}; // class make_ewkb_visitor

/**
 * Parser for (E)WKB.
 *
 * Empty Multi* geometries and Geometry Collections will return a NULL
 * geometry object.
 *
 * Call is_done() to check whether the parser read all available data.
 *
 * Can only parse (E)WKB in native byte order.
 */
class ewkb_parser_t
{
public:
    explicit ewkb_parser_t(std::string_view input)
    : m_data(input),
      m_max_length(static_cast<uint32_t>(input.size()) / (sizeof(double) * 2))
    {}

    geom::geometry_t operator()()
    {
        geom::geometry_t geom;

        if (m_data.empty()) {
            return geom;
        }

        uint32_t const type = parse_header(&geom);

        switch (type) {
        case geometry_type::wkb_point:
            parse_point(&geom.set<geom::point_t>());
            break;
        case geometry_type::wkb_line:
            parse_point_list(&geom.set<geom::linestring_t>(), 2);
            break;
        case geometry_type::wkb_polygon:
            parse_polygon(&geom.set<geom::polygon_t>());
            break;
        case geometry_type::wkb_multi_point:
            parse_multi_point(&geom);
            break;
        case geometry_type::wkb_multi_line:
            parse_multi_linestring(&geom);
            break;
        case geometry_type::wkb_multi_polygon:
            parse_multi_polygon(&geom);
            break;
        case geometry_type::wkb_collection:
            parse_collection(&geom);
            break;
        default:
            throw fmt_error("Invalid WKB geometry: Unknown geometry type {}",
                            type);
        }

        return geom;
    }

    bool is_done() const noexcept { return m_data.empty(); }

private:
    void check_bytes(uint32_t bytes) const
    {
        if (m_data.size() < bytes) {
            throw std::runtime_error{"Invalid WKB geometry: Incomplete"};
        }
    }

    uint32_t parse_uint32()
    {
        check_bytes(sizeof(uint32_t));

        uint32_t data = 0;
        std::memcpy(&data, m_data.data(), sizeof(uint32_t));
        m_data.remove_prefix(sizeof(uint32_t));

        return data;
    }

    /**
     * Get length field and check it against an upper bound based on the
     * maximum number of points which could theoretically be stored in a string
     * of the size of the input string (each point takes up at least
     * 2*sizeof(double) bytes. This prevents an invalid WKB from leading us to
     * reserve huge amounts of memory without having to define a constant upper
     * bound.
     */
    uint32_t parse_length()
    {
        auto const length = parse_uint32();
        if (length > m_max_length) {
            throw std::runtime_error{"Invalid WKB geometry: Length too large"};
        }
        return length;
    }

    uint32_t parse_header(geom::geometry_t *geom = nullptr)
    {
        if (static_cast<uint8_t>(m_data.front()) !=
            ewkb::wkb_byte_order_type_t::Endian) {
            throw std::runtime_error
            {
#if __BYTE_ORDER == __LITTLE_ENDIAN
                "Geometries in the database are returned in big-endian byte "
                "order. "
#else
                "Geometries in the database are returned in little-endian byte "
                "order. "
#endif
                "osm2pgsql can only process geometries in native byte order."
            };
        }
        m_data.remove_prefix(1);

        auto type = parse_uint32();
        if (type & ewkb::geometry_type::wkb_srid) {
            if (!geom) {
                // If geom is not set than this is one of the geometries
                // in a GeometryCollection or a Multi... geometry. They
                // should not have a SRID set, because the SRID is already
                // on the outer geometry.
                throw std::runtime_error{
                    "Invalid WKB geometry: SRID set in geometry of collection"};
            }
            type &= ~ewkb::geometry_type::wkb_srid;
            geom->set_srid(static_cast<int>(parse_uint32()));
        }
        return type;
    }

    void parse_point(geom::point_t *point)
    {
        check_bytes(sizeof(double) * 2);

        std::array<double, 2> data{};
        std::memcpy(data.data(), m_data.data(), sizeof(double) * 2);
        m_data.remove_prefix(sizeof(double) * 2);

        point->set_x(data[0]);
        point->set_y(data[1]);
    }

    void parse_point_list(geom::point_list_t *points, uint32_t min_points)
    {
        auto const num_points = parse_length();
        if (num_points < min_points) {
            throw fmt_error("Invalid WKB geometry: {} are not"
                            " enough points (must be at least {})",
                            num_points, min_points);
        }
        points->reserve(num_points);
        for (uint32_t i = 0; i < num_points; ++i) {
            parse_point(&points->emplace_back());
        }
    }

    void parse_polygon(geom::polygon_t *polygon)
    {
        auto const num_rings = parse_length();
        if (num_rings == 0) {
            throw std::runtime_error{
                "Invalid WKB geometry: Polygon without rings"};
        }
        parse_point_list(&polygon->outer(), 4);

        polygon->inners().reserve(num_rings - 1);
        for (uint32_t i = 1; i < num_rings; ++i) {
            parse_point_list(&polygon->inners().emplace_back(), 4);
        }
    }

    void parse_multi_point(geom::geometry_t *geom)
    {
        auto &multipoint = geom->set<geom::multipoint_t>();
        auto const num_geoms = parse_length();
        if (num_geoms == 0) {
            geom->reset();
            return;
        }

        multipoint.reserve(num_geoms);
        for (uint32_t i = 0; i < num_geoms; ++i) {
            auto &point = multipoint.add_geometry();
            uint32_t const type = parse_header();
            if (type != geometry_type::wkb_point) {
                throw fmt_error("Invalid WKB geometry: Multipoint containing"
                                " something other than point: {}",
                                type);
            }
            parse_point(&point);
        }
    }

    void parse_multi_linestring(geom::geometry_t *geom)
    {
        auto &multilinestring = geom->set<geom::multilinestring_t>();
        auto const num_geoms = parse_length();
        if (num_geoms == 0) {
            geom->reset();
            return;
        }

        multilinestring.reserve(num_geoms);
        for (uint32_t i = 0; i < num_geoms; ++i) {
            auto &linestring = multilinestring.add_geometry();
            uint32_t const type = parse_header();
            if (type != geometry_type::wkb_line) {
                throw fmt_error(
                    "Invalid WKB geometry: Multilinestring containing"
                    " something other than linestring: {}",
                    type);
            }
            parse_point_list(&linestring, 2);
        }
    }

    void parse_multi_polygon(geom::geometry_t *geom)
    {
        auto &multipolygon = geom->set<geom::multipolygon_t>();
        auto const num_geoms = parse_length();
        if (num_geoms == 0) {
            geom->reset();
            return;
        }

        multipolygon.reserve(num_geoms);
        for (uint32_t i = 0; i < num_geoms; ++i) {
            auto &polygon = multipolygon.add_geometry();
            uint32_t const type = parse_header();
            if (type != geometry_type::wkb_polygon) {
                throw fmt_error("Invalid WKB geometry: Multipolygon containing"
                                " something other than polygon: {}",
                                type);
            }
            parse_polygon(&polygon);
        }
    }

    void parse_collection(geom::geometry_t *geom)
    {
        auto &collection = geom->set<geom::collection_t>();
        auto const num_geoms = parse_length();
        if (num_geoms == 0) {
            geom->reset();
            return;
        }

        collection.reserve(num_geoms);
        for (uint32_t i = 0; i < num_geoms; ++i) {
            ewkb_parser_t parser{m_data};
            collection.add_geometry(parser());
            m_data = parser.m_data;
        }
    }

    std::string_view m_data;
    uint32_t m_max_length;

}; // class ewkb_parser_t

} // anonymous namespace

} // namespace ewkb

std::string geom_to_ewkb(geom::geometry_t const &geom, bool ensure_multi)
{
    return geom.visit(ewkb::make_ewkb_visitor{
        static_cast<uint32_t>(geom.srid()), ensure_multi});
}

geom::geometry_t ewkb_to_geom(std::string_view wkb)
{
    ewkb::ewkb_parser_t parser{wkb};
    auto geom = parser();

    if (!parser.is_done()) {
        throw std::runtime_error{"Invalid WKB geometry: Extra data at end"};
    }

    return geom;
}

static constexpr std::array<char, 256> const hex_table = {
    0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,
    0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,
    0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,
    0, 1, 2, 3,   4, 5, 6, 7,   8, 9, 0, 0,   0, 0, 0, 0,

    0, 10, 11, 12,   13, 14, 15, 0,   0, 0, 0, 0,   0, 0, 0, 0,
    0,  0,  0,  0,    0,  0,  0, 0,   0, 0, 0, 0,   0, 0, 0, 0,
    0, 10, 11, 12,   13, 14, 15, 0,   0, 0, 0, 0,   0, 0, 0, 0,
};

unsigned char decode_hex_char(char c) noexcept
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    return hex_table[static_cast<std::size_t>(static_cast<unsigned char>(c))];
}

std::string decode_hex(std::string_view hex_string)
{
    if (hex_string.size() % 2 != 0) {
        throw std::runtime_error{"Invalid wkb: Not a valid hex string"};
    }

    std::string wkb;
    wkb.reserve(hex_string.size() / 2);

    // NOLINTNEXTLINE(llvm-qualified-auto, readability-qualified-auto)
    for (auto hex = hex_string.begin(); hex != hex_string.end();) {
        unsigned int const c = decode_hex_char(*hex++);
        wkb += static_cast<char>((c << 4U) | decode_hex_char(*hex++));
    }

    return wkb;
}
