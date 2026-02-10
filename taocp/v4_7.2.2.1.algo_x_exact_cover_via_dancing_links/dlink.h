#ifndef ADT_DLINK_H_
#define ADT_DLINK_H_

#include <stdlib.h>

#include <zion/zion.h>

// === --- APIs ----------------------------------------------------------------

// Forward declaration.
struct dlink_tbl;

// Creates a new dancing link (dlink) table with all necessary memory
// allocations.
//
// User must specify the number of items (n_items) and number of nodes in all
// options (n_opt_nodes) ahead of time. They cannot be changed.
//
// Memory and Initialization:
// - One time allocation to reserve in total 1 + n_items + n_opt_nodes
//   nodes in the dlink table.
// - All item nodes are linked as the horizontal link list.
struct dlink_tbl *dlink_new( size_t n_items, size_t n_opt_nodes );
void              dlink_free( struct dlink_tbl * );

// Cover all nodes in a column for item c. Also unlink nodes from their columns
// belonging to the same option.
void dlink_cover_item( struct dlink_tbl *p, size_t c );

// Append a group of nodes belonging to one option.
//
// All nodes will be linked to the vertical list of item column and
// horizontal list of this group.
//
// The priv_data is set for all nodes and not owned by dlink table. It can be
// later obtained by dlink_get_node_data.
void dlink_append_opt( struct dlink_tbl *p, size_t num_ids, size_t *item_ids,
                       void *priv_data );

// Attempt to search a solution.
//
// - Return OK if find a solution and fill the (option) node ids in sol. User's
//   private data can be extracted via dlink_get_node_data. The sol must be
//   pre-allocated. Number of node ids filled is filled in num_sol. For each
//   option, only one node is filled in sol.
// - Return ENOTEXIST if no solution
error_t dlink_search( struct dlink_tbl *p, size_t *sol, _OUT_ size_t *num_sol );

// Extract the priv_data from the node indexed by the (optiona) node_id.
// Typically, node_id is obtained from the sol array filled by dlink_search.
void *dlink_get_node_data( struct dlink_tbl *p, size_t node_id );

#endif /* ADT_DLINK_H_ */
