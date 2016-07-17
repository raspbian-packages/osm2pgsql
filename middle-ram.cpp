/* Implements the mid-layer processing for osm2pgsql
 * using several arrays in RAM. This is fastest if you
 * have sufficient RAM+Swap.
 *
 * This layer stores data read in from the planet.osm file
 * and is then read by the backend processing code to
 * emit the final geometry-enabled output formats
*/

#include <stdexcept>

#include <cassert>
#include <cstdio>

#include "id-tracker.hpp"
#include "middle-ram.hpp"
#include "node-ram-cache.hpp"
#include "options.hpp"

/* Object storage now uses 2 levels of storage arrays.
 *
 * - Low level storage of 2^16 (~65k) objects in an indexed array
 *   These are allocated dynamically when we need to first store data with
 *   an ID in this block
 *
 * - Fixed array of 2^(32 - 16) = 65k pointers to the dynamically allocated arrays.
 *
 * This allows memory usage to be efficient and scale dynamically without needing to
 * hard code maximum IDs. We now support an ID  range of -2^31 to +2^31.
 * The negative IDs often occur in non-uploaded JOSM data or other data import scripts.
 *
 */


void middle_ram_t::nodes_set(osmid_t id, double lat, double lon, const taglist_t &tags) {
    cache->set(id, lat, lon, tags);
}

void middle_ram_t::ways_set(osmid_t id, const idlist_t &nds, const taglist_t &tags)
{
    ways.set(id, new ramWay(tags, nds));
}

void middle_ram_t::relations_set(osmid_t id, const memberlist_t &members, const taglist_t &tags)
{
    rels.set(id, new ramRel(tags, members));
}

size_t middle_ram_t::nodes_get_list(nodelist_t &out, const idlist_t nds) const
{
    for (idlist_t::const_iterator it = nds.begin(); it != nds.end(); ++it) {
        osmNode n;
        if (!cache->get(&n, *it))
            out.push_back(n);
    }

    return int(out.size());
}

void middle_ram_t::iterate_relations(pending_processor& pf)
{
    //TODO: just dont do anything

    //let the outputs enqueue everything they have the non slim middle
    //has nothing of its own to enqueue as it doesnt have pending anything
    pf.enqueue_relations(id_tracker::max());

    //let the threads process the relations
    pf.process_relations();
}

size_t middle_ram_t::pending_count() const
{
    return 0;
}

void middle_ram_t::iterate_ways(middle_t::pending_processor& pf)
{
    //let the outputs enqueue everything they have the non slim middle
    //has nothing of its own to enqueue as it doesnt have pending anything
    pf.enqueue_ways(id_tracker::max());

    //let the threads process the ways
    pf.process_ways();
}

void middle_ram_t::release_relations()
{
    rels.clear();
}

void middle_ram_t::release_ways()
{
    ways.clear();
}

bool middle_ram_t::ways_get(osmid_t id, taglist_t &tags, nodelist_t &nodes) const
{
    if (simulate_ways_deleted) {
        return false;
    }

    auto const *ele = ways.get(id);

    if (!ele) {
        return false;
    }

    tags = ele->tags;
    nodes_get_list(nodes, ele->ndids);

    return true;
}

size_t middle_ram_t::ways_get_list(const idlist_t &ids, idlist_t &way_ids,
                                multitaglist_t &tags, multinodelist_t &nodes) const
{
    if (ids.empty())
    {
        return 0;
    }

    assert(way_ids.empty());
    tags.assign(ids.size(), taglist_t());
    nodes.assign(ids.size(), nodelist_t());

    size_t count = 0;
    for (idlist_t::const_iterator it = ids.begin(); it != ids.end(); ++it) {
        if (ways_get(*it, tags[count], nodes[count])) {
            way_ids.push_back(*it);
            count++;
        } else {
            tags[count].clear();
            nodes[count].clear();
        }
    }

    if (count < ids.size()) {
        tags.resize(count);
        nodes.resize(count);
    }

    return int(count);
}

bool middle_ram_t::relations_get(osmid_t id, memberlist_t &members, taglist_t &tags) const
{
    auto const *ele = rels.get(id);

    if (!ele) {
        return false;
    }

    tags = ele->tags;
    members = ele->members;

    return true;
}

void middle_ram_t::analyze(void)
{
    /* No need */
}

void middle_ram_t::end(void)
{
    /* No need */
}

void middle_ram_t::start(const options_t *out_options_)
{
    out_options = out_options_;
    /* latlong has a range of +-180, mercator +-20000
       The fixed poing scaling needs adjusting accordingly to
       be stored accurately in an int */
    cache.reset(new node_ram_cache(out_options->alloc_chunkwise, out_options->cache, out_options->scale));

    fprintf( stderr, "Mid: Ram, scale=%d\n", out_options->scale );
}

void middle_ram_t::stop(void)
{
    cache.reset(nullptr);

    release_ways();
    release_relations();
}

void middle_ram_t::commit(void) {
}

middle_ram_t::middle_ram_t():
    ways(), rels(), cache(), simulate_ways_deleted(false)
{
}

middle_ram_t::~middle_ram_t() {
    //instance.reset();
}

std::vector<osmid_t> middle_ram_t::relations_using_way(osmid_t way_id) const
{
    // this function shouldn't be called - relations_using_way is only used in
    // slim mode, and a middle_ram_t shouldn't be constructed if the slim mode
    // option is set.
    throw std::runtime_error("middle_ram_t::relations_using_way is unimlpemented, and "
                             "should not have been called. This is probably a bug, please "
                             "report it at https://github.com/openstreetmap/osm2pgsql/issues");
}

namespace {

void no_delete(const middle_ram_t * middle) {
    // boost::shared_ptr thinks we are going to delete
    // the middle object, but we are not. Heh heh heh.
    // So yeah, this is a hack...
}

}

std::shared_ptr<const middle_query_t> middle_ram_t::get_instance() const {
    //shallow copy here because readonly access is thread safe
    return std::shared_ptr<const middle_query_t>(this, no_delete);
}
