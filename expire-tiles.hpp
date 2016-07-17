#ifndef EXPIRE_TILES_H
#define EXPIRE_TILES_H

#include "osmtypes.hpp"

#include <boost/noncopyable.hpp>

class table_t;
struct options_t;

struct expire_tiles : public boost::noncopyable {
    explicit expire_tiles(const options_t *options);
    ~expire_tiles();

    //TODO: copy constructor

    int from_bbox(double min_lon, double min_lat, double max_lon, double max_lat);
    void from_nodes_line(const nodelist_t &nodes);
    void from_nodes_poly(const nodelist_t &nodes, osmid_t osm_id);
    void from_wkb(const char* wkb, osmid_t osm_id);
    int from_db(table_t* table, osmid_t osm_id);

    struct tile {
        int	complete[2][2];
        struct tile* subtiles[2][2];
    };

    /* customisable tile output. this can be passed into the
     * `output_and_destroy` function to override output to a file.
     * this is primarily useful for testing.
     */
    struct tile_output {
        virtual ~tile_output() {}
        // dirty a tile at x, y & zoom, and all descendants of that
        // tile at the given zoom if zoom < min_zoom.
        virtual void output_dirty_tile(int x, int y, int zoom, int min_zoom) = 0;
    };

    // output the list of expired tiles to a file. note that this
    // consumes the list of expired tiles destructively.
    void output_and_destroy();

    // output the list of expired tiles using a `tile_output`
    // functor. this consumes the list of expired tiles destructively.
    void output_and_destroy(tile_output *output);

    // merge the list of expired tiles in the other object into this
    // object, destroying the list in the other object.
    void merge_and_destroy(expire_tiles &);

private:
    void expire_tile(int x, int y);
    int normalise_tile_x_coord(int x);
    void from_line(double lon_a, double lat_a, double lon_b, double lat_b);
    void from_xnodes_poly(const multinodelist_t &xnodes, osmid_t osm_id);
    void from_xnodes_line(const multinodelist_t &xnodes);

    int map_width;
    double tile_width;
    const options_t *Options;
    struct tile *dirty;
};

#endif
