#ifndef OSM2PGSQL_WKB_HPP
#define OSM2PGSQL_WKB_HPP

/**
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * This file is part of osm2pgsql (https://osm2pgsql.org/).
 *
 * Copyright (C) 2006-2022 by the osm2pgsql developer community.
 * For a full list of authors see the git log.
 */

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>

#include <osmium/geom/coordinates.hpp>
#include <osmium/geom/factory.hpp>

namespace ewkb {

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
static void str_push(std::string *str, T data)
{
    assert(str);
    str->append(reinterpret_cast<char const *const>(&data), sizeof(T));
}

/**
 * Add a EWKB header to the string.
 *
 * \pre \code str != nullptr \endcode
 */
inline std::size_t write_header(std::string *str, geometry_type type,
                                uint32_t srid)
{
    assert(str);

    str_push(str, Endian);
    str_push(str, type | wkb_srid);
    str_push(str, srid);

    return str->size();
}

/**
 * Add a EWKB header to the string and add a dummy placeholder length of 0.
 * This can later be replaced by the real length.
 *
 * \pre \code str != nullptr \endcode
 */
inline std::size_t write_header_with_length(std::string *str,
                                            geometry_type type, uint32_t srid)
{
    auto const offset = write_header(str, type, srid);
    str_push(str, static_cast<uint32_t>(0));
    return offset;
}

/// Create EWKB Point geometry.
inline std::string create_point(double x, double y, uint32_t srid = 4326)
{
    std::string data;
    data.reserve(25); // Point geometries are always 25 bytes

    write_header(&data, wkb_point, srid);
    str_push(&data, x);
    str_push(&data, y);

    return data;
}

/**
 *  Writer for EWKB data suitable for postgres.
 *
 *  Code has been largely derived from osmium::geom::WKBFactoryImpl.
 */
class writer_t
{
public:
    explicit writer_t(int srid) : m_srid(static_cast<uint32_t>(srid))
    {
        assert(srid > 0);
    }

    void add_sub_geometry(std::string const &part)
    {
        assert(!m_data.empty());
        m_data.append(part);
    }

    void add_location(osmium::geom::Coordinates const &xy)
    {
        assert(!m_data.empty());
        str_push(&m_data, xy.x);
        str_push(&m_data, xy.y);
    }

    /* Point */

    std::string make_point(osmium::geom::Coordinates const &xy) const
    {
        return create_point(xy.x, xy.y, m_srid);
    }

    /* LineString */

    void linestring_start()
    {
        assert(m_data.empty());
        m_geometry_size_offset =
            write_header_with_length(&m_data, wkb_line, m_srid);
    }

    std::string linestring_finish(std::size_t num_points)
    {
        set_size(m_geometry_size_offset, num_points);
        std::string data;

        using std::swap;
        swap(data, m_data);

        return data;
    }

    /* MultiLineString */

    void multilinestring_start()
    {
        assert(m_data.empty());
        m_multigeometry_size_offset =
            write_header_with_length(&m_data, wkb_multi_line, m_srid);
    }

    std::string multilinestring_finish(std::size_t num_lines)
    {
        set_size(m_multigeometry_size_offset, num_lines);
        std::string data;

        using std::swap;
        swap(data, m_data);

        return data;
    }

    /* Polygon */

    void polygon_start()
    {
        assert(m_data.empty());
        m_geometry_size_offset =
            write_header_with_length(&m_data, wkb_polygon, m_srid);
    }

    void polygon_ring_start()
    {
        m_ring_size_offset = m_data.size();
        str_push(&m_data, static_cast<uint32_t>(0));
    }

    void polygon_ring_finish(std::size_t num_points)
    {
        set_size(m_ring_size_offset, num_points);
    }

    std::string polygon_finish(std::size_t num_rings)
    {
        set_size(m_geometry_size_offset, num_rings);
        std::string data;

        using std::swap;
        swap(data, m_data);

        return data;
    }

    /* MultiPolygon */

    void multipolygon_start()
    {
        assert(m_data.empty());
        m_multigeometry_size_offset =
            write_header_with_length(&m_data, wkb_multi_polygon, m_srid);
    }

    std::string multipolygon_finish(std::size_t num_polygons)
    {
        set_size(m_multigeometry_size_offset, num_polygons);
        std::string data;

        using std::swap;
        swap(data, m_data);

        return data;
    }

private:
    void set_size(std::size_t offset, std::size_t size)
    {
        assert(m_data.size() >= offset + sizeof(uint32_t));
        auto const s = static_cast<uint32_t>(size);
        std::memcpy(&m_data[offset], reinterpret_cast<char const *>(&s),
                    sizeof(uint32_t));
    }

    std::string m_data;

    std::size_t m_geometry_size_offset = 0;
    std::size_t m_multigeometry_size_offset = 0;
    std::size_t m_ring_size_offset = 0;

    uint32_t m_srid;
}; // class writer_t

/**
 * Class that allows to iterate over the elements of a ewkb geometry.
 *
 * Note: this class assumes that the wkb was created by ewkb::writer_t.
 *       It implements the exact opposite decoding.
 */
class parser_t
{
public:
    inline static std::string wkb_from_hex(std::string const &wkb)
    {
        std::string out;

        bool front = true;
        char outc;
        for (char c : wkb) {
            c -= 48;
            if (c > 9) {
                c -= 7;
            }
            if (front) {
                outc = char(c << 4);
                front = false;
            } else {
                out += outc | c;
                front = true;
            }
        }

        if (out[0] != Endian) {
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
        return out;
    }

    explicit parser_t(std::string const &wkb) noexcept : m_wkb(&wkb) {}

    std::size_t save_pos() const noexcept { return m_pos; }

    void rewind(std::size_t pos) noexcept { m_pos = pos; }

    uint32_t read_header()
    {
        m_pos += sizeof(uint8_t); // skip endianess marker

        auto const type = read_data<uint32_t>();

        if (type & wkb_srid) {
            m_pos += sizeof(uint32_t); // skip SRID
        }

        return type & 0xffU;
    }

    uint32_t read_length() { return read_data<uint32_t>(); }

    osmium::geom::Coordinates read_point()
    {
        auto const x = read_data<double>();
        auto const y = read_data<double>();

        return osmium::geom::Coordinates{x, y};
    }

    void skip_points(std::size_t num)
    {
        auto const length = sizeof(double) * 2 * num;
        check_available(length);
        m_pos += length;
    }

    template <typename PROJ>
    double get_area(PROJ *proj = nullptr)
    {
        double total = 0;

        auto const type = read_header();

        if (type == wkb_polygon) {
            total = get_polygon_area(proj);
        } else if (type == wkb_multi_polygon) {
            auto const num_poly = read_length();
            for (unsigned i = 0; i < num_poly; ++i) {
                auto ptype = read_header();
                (void)ptype;
                assert(ptype == wkb_polygon);

                total += get_polygon_area(proj);
            }
        }

        return total;
    }

private:
    template <typename PROJ>
    double get_polygon_area(PROJ *proj)
    {
        auto const num_rings = read_length();
        assert(num_rings > 0);

        double total = get_ring_area(proj);

        for (unsigned i = 1; i < num_rings; ++i) {
            total -= get_ring_area(proj);
        }

        return total;
    }

    template <typename PROJ>
    double get_ring_area(PROJ *proj)
    {
        // Algorithm borrowed from
        // http://stackoverflow.com/questions/451426/how-do-i-calculate-the-area-of-a-2d-polygon
        // XXX numerically not stable (useless for latlon)
        auto const num_pts = read_length();
        assert(num_pts > 3);

        double total = 0;

        auto prev = proj->target_to_tile(read_point());
        for (unsigned i = 1; i < num_pts; ++i) {
            auto const cur = proj->target_to_tile(read_point());
            total += prev.x * cur.y - cur.x * prev.y;
            prev = cur;
        }

        return std::abs(total) * 0.5;
    }

    double get_ring_area(osmium::geom::IdentityProjection *)
    {
        // Algorithm borrowed from
        // http://stackoverflow.com/questions/451426/how-do-i-calculate-the-area-of-a-2d-polygon
        auto const num_pts = read_length();
        assert(num_pts > 3);

        double total = 0;

        auto prev = read_point();
        for (unsigned i = 1; i < num_pts; ++i) {
            auto const cur = read_point();
            total += prev.x * cur.y - cur.x * prev.y;
            prev = cur;
        }

        return std::abs(total) * 0.5;
    }

    void check_available(std::size_t length)
    {
        if (m_pos + length > m_wkb->size()) {
            throw std::runtime_error{"Invalid EWKB geometry found"};
        }
    }

    template <typename T>
    T read_data()
    {
        check_available(sizeof(T));

        T data;
        std::memcpy(&data, m_wkb->data() + m_pos, sizeof(T));
        m_pos += sizeof(T);

        return data;
    }

    std::string const *m_wkb;
    std::size_t m_pos = 0;
};

} // namespace ewkb

#endif // OSM2PGSQL_WKB_HPP
