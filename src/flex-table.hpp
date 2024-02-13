#ifndef OSM2PGSQL_FLEX_TABLE_HPP
#define OSM2PGSQL_FLEX_TABLE_HPP

/**
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * This file is part of osm2pgsql (https://osm2pgsql.org/).
 *
 * Copyright (C) 2006-2024 by the osm2pgsql developer community.
 * For a full list of authors see the git log.
 */

#include "db-copy-mgr.hpp"
#include "flex-index.hpp"
#include "flex-table-column.hpp"
#include "pgsql.hpp"
#include "reprojection.hpp"
#include "thread-pool.hpp"
#include "util.hpp"

#include <osmium/osm/item_type.hpp>

#include <cstddef>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

/**
 * This defines the type of "primary key" for the tables generated in the flex
 * output. This is not a real primary key, because the values are not
 * necessarily unique.
 */
enum class flex_table_index_type {
    no_index,
    node, // index by node id
    way, // index by way id
    relation, // index by relation id
    area, // index by way (positive) or relation (negative) id
    any_object, // any OSM object, two columns for type and id
    tile // index by tile with x and y columns (used for generalized data)
};

/**
 * An output table (in the SQL sense) for the flex backend.
 */
class flex_table_t
{

public:

    /**
     * Table creation type: interim tables are created as UNLOGGED and with
     * autovacuum disabled.
     */
    enum class table_type {
        interim,
        permanent
    };

    flex_table_t(std::string schema, std::string name, std::size_t num)
    : m_schema(std::move(schema)), m_name(std::move(name)), m_table_num(num)
    {
    }

    std::string const &name() const noexcept { return m_name; }

    std::string const &schema() const noexcept { return m_schema; }

    bool cluster_by_geom() const noexcept
    {
        return has_geom_column() && m_cluster_by_geom;
    }

    std::string const &data_tablespace() const noexcept
    {
        return m_data_tablespace;
    }

    std::string const &index_tablespace() const noexcept
    {
        return m_index_tablespace;
    }

    void set_schema(std::string schema) noexcept
    {
        m_schema = std::move(schema);
    }

    void set_cluster_by_geom(bool cluster) noexcept
    {
        m_cluster_by_geom = cluster;
    }

    void set_data_tablespace(std::string tablespace) noexcept
    {
        m_data_tablespace = std::move(tablespace);
    }

    void set_index_tablespace(std::string tablespace) noexcept
    {
        m_index_tablespace = std::move(tablespace);
    }

    flex_table_index_type id_type() const noexcept { return m_id_type; }

    void set_id_type(flex_table_index_type type) noexcept { m_id_type = type; }

    bool has_id_column() const noexcept;

    std::size_t num_columns() const noexcept { return m_columns.size(); }

    std::vector<flex_table_column_t>::const_iterator begin() const noexcept
    {
        return m_columns.begin();
    }

    std::vector<flex_table_column_t>::const_iterator end() const noexcept
    {
        return m_columns.end();
    }

    flex_table_column_t *find_column_by_name(std::string const &name)
    {
        return util::find_by_name(m_columns, name);
    }

    bool has_geom_column() const noexcept
    {
        return m_geom_column != std::numeric_limits<std::size_t>::max();
    }

    /// Get the (first, if there are multiple) geometry column.
    flex_table_column_t const &geom_column() const noexcept
    {
        assert(has_geom_column());
        return m_columns[m_geom_column];
    }

    flex_table_column_t &geom_column() noexcept
    {
        assert(has_geom_column());
        return m_columns[m_geom_column];
    }

    int srid() const noexcept
    {
        return has_geom_column() ? geom_column().srid() : 4326;
    }

    std::string build_sql_prepare_get_wkb() const;

    std::string build_sql_create_table(table_type ttype,
                                       std::string const &table_name) const;

    std::string build_sql_column_list() const;

    std::string build_sql_create_id_index() const;

    /// Does this table take objects of the specified type?
    bool matches_type(osmium::item_type type) const noexcept;

    /// Map way/node/relation ID to id value used in database table column
    osmid_t map_id(osmium::item_type type, osmid_t id) const noexcept;

    flex_table_column_t &add_column(std::string const &name,
                                    std::string const &type,
                                    std::string const &sql_type);

    bool has_multicolumn_id_index() const noexcept;
    std::string id_column_names() const;
    std::string full_name() const;
    std::string full_tmp_name() const;

    bool has_multiple_geom_columns() const noexcept
    {
        return m_has_multiple_geom_columns;
    }

    std::vector<flex_index_t> const &indexes() const noexcept
    {
        return m_indexes;
    }

    flex_index_t &add_index(std::string method);

    void set_always_build_id_index() noexcept
    {
        m_always_build_id_index = true;
    }

    bool always_build_id_index() const noexcept
    {
        return m_always_build_id_index;
    }

    bool has_columns_with_expire() const noexcept;

    std::size_t num() const noexcept { return m_table_num; }

private:
    /// The schema this table is in
    std::string m_schema;

    /// The name of the table
    std::string m_name;

    /// The table space used for this table (empty for default tablespace)
    std::string m_data_tablespace;

    /**
     * The table space used for indexes on this table (empty for default
     * tablespace)
     */
    std::string m_index_tablespace;

    /**
     * The columns in this table (The first zero, one or two columns are always
     * the id columns).
     */
    std::vector<flex_table_column_t> m_columns;

    /**
     * The indexes defined on this table. Does not include the id index.
     */
    std::vector<flex_index_t> m_indexes;

    /**
     * Index of the (first) geometry column in m_columns. Default means no
     * geometry column.
     */
    std::size_t m_geom_column = std::numeric_limits<std::size_t>::max();

    /// Unique number for each table.
    std::size_t m_table_num;

    /**
     * Type of id stored in this table.
     */
    flex_table_index_type m_id_type = flex_table_index_type::no_index;

    /// Cluster the table by geometry.
    bool m_cluster_by_geom = true;

    /// Does this table have more than one geometry column?
    bool m_has_multiple_geom_columns = false;

    /// Always build the id index, not only when it is needed for updates?
    bool m_always_build_id_index = false;

}; // class flex_table_t

class table_connection_t
{
public:
    table_connection_t(flex_table_t *table,
                       std::shared_ptr<db_copy_thread_t> const &copy_thread)
    : m_proj(reprojection::create_projection(table->srid())), m_table(table),
      m_target(std::make_shared<db_target_descr_t>(
          table->schema(), table->name(), table->id_column_names(),
          table->build_sql_column_list())),
      m_copy_mgr(copy_thread)
    {
    }

    void start(pg_conn_t const &db_connection, bool append);

    void stop(pg_conn_t const &db_connection, bool updateable, bool append);

    flex_table_t const &table() const noexcept { return *m_table; }

    void prepare(pg_conn_t const &db_connection);

    void analyze(pg_conn_t const &db_connection);

    void create_id_index(pg_conn_t const &db_connection);

    /**
     * Get all geometries that have at least one expire config defined
     * from the database and return the result set.
     */
    pg_result_t get_geoms_by_id(pg_conn_t const &db_connection,
                                osmium::item_type type, osmid_t id) const;

    void flush() { m_copy_mgr.flush(); }

    void sync() { m_copy_mgr.sync(); }

    void new_line() { m_copy_mgr.new_line(m_target); }

    db_copy_mgr_t<db_deleter_by_type_and_id_t> *copy_mgr() noexcept
    {
        return &m_copy_mgr;
    }

    void delete_rows_with(osmium::item_type type, osmid_t id);

    reprojection const &proj() const noexcept
    {
        assert(m_proj);
        return *m_proj;
    }

    void task_set(std::future<std::chrono::microseconds> &&future)
    {
        m_task_result.set(std::move(future));
    }

    void task_wait();

    void increment_insert_counter() noexcept { ++m_count_insert; }

    void increment_not_null_error_counter() noexcept
    {
        ++m_count_not_null_error;
    }

private:
    std::shared_ptr<reprojection> m_proj;

    flex_table_t *m_table;

    std::shared_ptr<db_target_descr_t> m_target;

    /**
     * The copy manager responsible for sending data through the COPY mechanism
     * to the database server.
     */
    db_copy_mgr_t<db_deleter_by_type_and_id_t> m_copy_mgr;

    task_result_t m_task_result;

    std::size_t m_count_insert = 0;
    std::size_t m_count_not_null_error = 0;

    /// Has the Id index already been created?
    bool m_id_index_created = false;

}; // class table_connection_t

char const *type_to_char(osmium::item_type type) noexcept;

#endif // OSM2PGSQL_FLEX_TABLE_HPP
