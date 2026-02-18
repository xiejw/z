// vim: ft=cpp
// forge:v1
//
// See README.md for data structure.
#pragma once

#include <vector>

#include <stdlib.h>

// === --- APIs ------------------------------------------------------------ ===
namespace taocp {

/// Node in the horizontal item list.
///
/// See Volume 4B, Page 68.
struct DLinkItem {
        size_t id;
        size_t l;  // LLINK.
        size_t r;  // RLINK.
};

/// Node in the table.
///
/// See Volume 4B, Page 68.
struct DLinkNode {
        size_t u;  // ULINK.
        size_t d;  // DLINK.
        union {
                // Count of the vertical col. Used by vertical header.
                size_t len;
                // Item Id of the vertical col. Used by option node.
                ssize_t top;
        };
};

struct DLinkTable {
      private:
        size_t n_items;
        size_t n_options;
        size_t n_option_nodes;

        std::vector<DLinkItem> item_list;
        std::vector<DLinkNode> table;

      public:
        // Creates a new dancing link table with all necessary memory
        // allocations.
        //
        // User must specify the number of items (n_items), number of options
        // (n_options), and number of nodes in all options (n_option_nodes)
        // ahead of time. They cannot be changed.
        //
        // The number of spacers will be deduced.
        DLinkTable( size_t n_items, size_t n_options, size_t n_option_nodes );

      public:
        DLinkItem *GetHorizontalItem( size_t i );

        // To append options with associated nodes, a callback function is
        // provided to fill all nodes in one state machine.
        //
        // AppendOptions will call fn one option each time and stop once the
        // whole table is filled (n_options reached).  Spacers will be inserted
        // automatically.  Callback fn must ensure n_option_nodes, provided in
        // the constructor, are respected.
        //
        // For each invocation of callback fn, the option_node_size and
        // option_node_top_ids must be set so AppendOptions can fill the table.
        //
        void AppendOptions( void ( *fn )( void    *user_data,
                                          size_t  *option_node_size,
                                          size_t **option_node_top_ids ),
                            void *user_data );

        /// Search all solutions.
        ///
        /// The visit_fn is called for all solutions found. If the visit_fn
        /// returns true, then the searching process stops immediately.
        /// Otherwise, continues.
        ///
        /// Users pass the visit_fn, along with the buffer to fill the
        /// solution.  The number of solution is upper bounded by the number of
        /// items. But that upper bound could be too large, and the users might
        /// know the bound more precisely and save memory spaces. To suppor
        /// that, users can provide a smaller buffer and hint the maximum size
        /// to set the solution. This function will copy at most
        /// max_solution_size to the buffer and stop even this could be partial
        /// solution. To detect this partial case, the buffer could be allocate
        /// +1 size, set a SENT value, and increase the max_solution_size +1 as
        /// well. Then check whether that SENT has been modified.
        ///
        /// Also note users own the buffer and need to ensure it is big enough
        /// to hold max_solution_size data.
        ///
        /// Each invocation of visit_fn is called with a solution buffer,
        /// provided by users, with and the number of filled results in the
        /// buffer, solution_size.
        ///
        ///     solution[0...solution_size] is the valid (partial) solution
        ///
        /// The solution array is unsorted. Each element's domain is [1,
        /// n_options], i.e., the 1-based option ID.
        ///
        void SearchSolutions( bool ( *visit_fn )( void   *user_data,
                                                  size_t  solution_size,
                                                  size_t *solution ),
                              void *user_data, size_t max_solution_size,
                              size_t *solution );

      private:
        DLinkNode *GetTableItem( size_t i );
};
}  // namespace taocp

/*

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

 */
