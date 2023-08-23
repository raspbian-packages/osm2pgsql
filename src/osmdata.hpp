#ifndef OSM2PGSQL_OSMDATA_HPP
#define OSM2PGSQL_OSMDATA_HPP

/**
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * This file is part of osm2pgsql (https://osm2pgsql.org/).
 *
 * Copyright (C) 2006-2023 by the osm2pgsql developer community.
 * For a full list of authors see the git log.
 */

/**
 * \file
 *
 * It contains the osmdata_t class.
 */

#include <memory>
#include <string>

#include <osmium/fwd.hpp>
#include <osmium/handler.hpp>
#include <osmium/osm/box.hpp>

#include "dependency-manager.hpp"
#include "osmtypes.hpp"

class middle_t;
class output_t;
struct options_t;

/**
 * This class guides the processing of the OSM data through its multiple
 * stages. It calls upon the major compontents of osm2pgsql, the dependency
 * manager, the middle, and the output to do their work.
 */
class osmdata_t : public osmium::handler::Handler
{
public:
    osmdata_t(std::unique_ptr<dependency_manager_t> dependency_manager,
              std::shared_ptr<middle_t> mid, std::shared_ptr<output_t> output,
              options_t const &options);

    void start() const;

    void node(osmium::Node const &node);
    void way(osmium::Way &way);
    void relation(osmium::Relation const &rel);

    void after_nodes();
    void after_ways();
    void after_relations();

    /**
     * Rest of the processing (stages 1b, 1c, 2, and database postprocessing).
     * This is called once after the input files are processed.
     */
    void stop() const;

private:
    /**
     * Run stage 1b and stage 1c processing: Process dependent objects in
     * append mode.
     */
    void process_dependents() const;

    /**
     * Run stage 2 processing: Reprocess objects marked in stage 1 (if any).
     */
    void reprocess_marked() const;

    /**
     * Run postprocessing on database: Clustering and index creation.
     */
    void postprocess_database() const;

    std::unique_ptr<dependency_manager_t> m_dependency_manager;
    std::shared_ptr<middle_t> m_mid;
    std::shared_ptr<output_t> m_output;

    std::string m_conninfo;

    // Bounding box for node import (or invalid Box if everything should be
    // imported).
    osmium::Box m_bbox;

    unsigned int m_num_procs;
    bool m_append;
    bool m_droptemp;
    bool m_with_extra_attrs;
    bool m_with_forward_dependencies;
};

#endif // OSM2PGSQL_OSMDATA_HPP
