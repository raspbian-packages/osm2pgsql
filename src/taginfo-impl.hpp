#ifndef OSM2PGSQL_TAGINFO_IMPL_HPP
#define OSM2PGSQL_TAGINFO_IMPL_HPP

#include "osmtypes.hpp"
#include "taginfo.hpp"
#include <string>
#include <utility>
#include <vector>

enum column_flags
{
    FLAG_POLYGON = 1, /* For polygon table */
    FLAG_LINEAR = 2,  /* For lines table */
    FLAG_NOCACHE = 4, /* Optimisation: don't bother remembering this one */
    FLAG_DELETE = 8,  /* These tags should be simply deleted on sight */
    FLAG_NOCOLUMN =
        16, /* objects without column but should be listed in database hstore column */
    FLAG_PHSTORE =
        17, /* same as FLAG_NOCOLUMN & FLAG_POLYGON to maintain compatibility */
    FLAG_INT_TYPE = 32, /* column value should be converted to integer */
    FLAG_REAL_TYPE = 64 /* column value should be converted to double */
};

struct taginfo
{
    taginfo();
    taginfo(taginfo const &);

    ColumnType column_type() const
    {
        if (flags & FLAG_INT_TYPE) {
            return COLUMN_TYPE_INT;
        }
        if (flags & FLAG_REAL_TYPE) {
            return COLUMN_TYPE_REAL;
        }
        return COLUMN_TYPE_TEXT;
    }

    std::string name, type;
    unsigned flags;
};

struct export_list
{
    void add(osmium::item_type id, taginfo const &info);
    std::vector<taginfo> &get(osmium::item_type id);
    std::vector<taginfo> const &get(osmium::item_type id) const;

    columns_t normal_columns(osmium::item_type id) const;
    bool has_column(osmium::item_type id, char const *name) const;

    std::vector<std::vector<taginfo>> exportList; /* Indexed osmium nwr index */
};

/* Parse a comma or whitespace delimited list of tags to apply to
 * a style file entry, returning the OR-ed set of flags.
 */
unsigned parse_tag_flags(std::string const &flags, int lineno);

/* Parse an osm2pgsql "pgsql" backend format style file, putting
 * the results in the `exlist` argument.
 *
 * Returns 1 if the 'way_area' column should (implicitly) exist, or
 * 0 if it should be suppressed.
 */
int read_style_file(std::string const &filename, export_list *exlist);

#endif // OSM2PGSQL_TAGINFO_IMPL_HPP
