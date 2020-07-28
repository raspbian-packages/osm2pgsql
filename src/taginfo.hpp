#ifndef OSM2PGSQL_TAGINFO_HPP
#define OSM2PGSQL_TAGINFO_HPP

#include <string>
#include <vector>

enum ColumnType
{
    COLUMN_TYPE_INT,
    COLUMN_TYPE_REAL,
    COLUMN_TYPE_TEXT
};

struct Column
{
    Column(std::string const &n, std::string const &tn, ColumnType t)
    : name(n), type_name(tn), type(t)
    {}

    std::string name;
    std::string type_name;
    ColumnType type;
};

using columns_t = std::vector<Column>;

/* Table columns, representing key= tags */
struct taginfo;

/* list of exported tags */
struct export_list;

#endif // OSM2PGSQL_TAGINFO_HPP
