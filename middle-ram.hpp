/* Implements the mid-layer processing for osm2pgsql
 * using data structures in RAM.
 *
 * This layer stores data read in from the planet.osm file
 * and is then read by the backend processing code to
 * emit the final geometry-enabled output formats
*/

#ifndef MIDDLE_RAM_H
#define MIDDLE_RAM_H

#include <memory>

#include "middle.hpp"
#include <vector>
#include <array>

struct node_ram_cache;
struct options_t;

template <typename T, size_t N>
class cache_block_t
{
    std::array<std::unique_ptr<T>, N> arr;
public:
    void set(size_t idx, T *ele) { arr[idx].reset(ele); }

    T const *get(size_t idx) const { return arr[idx].get(); }
};

template <typename T, size_t BLOCK_SHIFT>
class elem_cache_t
{
    constexpr static size_t per_block() { return 1 << BLOCK_SHIFT; }
    constexpr static size_t num_blocks() { return 1 << (32 - BLOCK_SHIFT); }

    constexpr static size_t id2block(osmid_t id)
    {
        /* + NUM_BLOCKS/2 allows for negative IDs */
        return (id >> BLOCK_SHIFT) + num_blocks()/2;
    }

    constexpr static size_t id2offset(osmid_t id)
    {
        return id & (per_block()-1);
    }

    typedef cache_block_t<T, 1 << BLOCK_SHIFT> element_t;
    std::vector<std::unique_ptr<element_t>> arr;
public:
    elem_cache_t() : arr(num_blocks()) {}

    void set(osmid_t id, T *ele)
    {
        const size_t block = id2block(id);

        if (!arr[block]) {
            arr[block].reset(new element_t());
        }

        arr[block]->set(id2offset(id), ele);
    }

    T const *get(osmid_t id) const
    {
        const size_t block = id2block(id);

        if (!arr[block]) {
            return 0;
        }

        return arr[block]->get(id2offset(id));
    }

    void clear()
    {
        for (auto &ele : arr) {
            ele.release();
        }
    }
};

struct middle_ram_t : public middle_t {
    middle_ram_t();
    virtual ~middle_ram_t();

    void start(const options_t *out_options_);
    void stop(void);
    void analyze(void);
    void end(void);
    void commit(void);

    void nodes_set(osmid_t id, double lat, double lon, const taglist_t &tags);
    size_t nodes_get_list(nodelist_t &out, const idlist_t nds) const;
    int nodes_delete(osmid_t id);
    int node_changed(osmid_t id);

    void ways_set(osmid_t id, const idlist_t &nds, const taglist_t &tags);
    bool ways_get(osmid_t id, taglist_t &tags, nodelist_t &nodes) const;
    size_t ways_get_list(const idlist_t &ids, idlist_t &way_ids,
                      multitaglist_t &tags, multinodelist_t &nodes) const;

    int ways_delete(osmid_t id);
    int way_changed(osmid_t id);

    bool relations_get(osmid_t id, memberlist_t &members, taglist_t &tags) const;
    void relations_set(osmid_t id, const memberlist_t &members, const taglist_t &tags);
    int relations_delete(osmid_t id);
    int relation_changed(osmid_t id);

    std::vector<osmid_t> relations_using_way(osmid_t way_id) const;

    void iterate_ways(middle_t::pending_processor& pf);
    void iterate_relations(pending_processor& pf);

    size_t pending_count() const;

    virtual std::shared_ptr<const middle_query_t> get_instance() const;
private:

    void release_ways();
    void release_relations();

    struct ramWay {
        taglist_t tags;
        idlist_t ndids;

        ramWay(const taglist_t &t, const idlist_t &n) : tags(t), ndids(n) {}
    };

    struct ramRel {
        taglist_t tags;
        memberlist_t members;

        ramRel(const taglist_t &t, const memberlist_t &m) : tags(t), members(m) {}
    };

    elem_cache_t<ramWay, 10> ways;
    elem_cache_t<ramRel, 10> rels;

    std::unique_ptr<node_ram_cache> cache;

    /* the previous behaviour of iterate_ways was to delete all ways as they
     * were being iterated. this doesn't work now that the output handles its
     * own "done" status and output-specific "pending" status. however, the
     * tests depend on the behaviour that ways will be unavailable once
     * iterate_ways is complete, so this flag emulates that. */
    bool simulate_ways_deleted;
};

#endif
