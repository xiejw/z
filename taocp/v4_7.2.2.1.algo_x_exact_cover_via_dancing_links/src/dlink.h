// vim: ft=cpp
// forge:v1
//
// See README.md for data structure.
#pragma once

#include <vector>

#include <stdlib.h>

namespace taocp {

/// Node in the horizontal item list.
///
/// See Volume 4B, Page 68.
struct DLinkItem {
        //        size_t id;
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

        std::vector<DLinkItem> item_list;
        std::vector<DLinkNode> table;

      public:
        /// Creates a new dancing link table with all necessary memory
        /// allocations.
        ///
        /// User must specify the number of items (n_items), number of options
        /// (n_options), and number of nodes in all options (n_option_nodes)
        /// ahead of time. They cannot be changed.
        ///
        /// The number of spacers will be deduced.
        ///
        DLinkTable( size_t n_items, size_t n_options, size_t n_option_nodes );

      public:
        /// To append options with associated nodes, a callback function is
        /// provided to fill all nodes in one state machine.
        ///
        /// AppendOptions will call fn one option each time and stop once the
        /// whole table is filled (n_options reached).  Spacers will be
        /// inserted automatically.  Callback fn must ensure n_option_nodes,
        /// provided in the constructor, are respected.
        ///
        /// For each invocation of callback fn, the option_node_size and
        /// option_node_top_ids must be set so AppendOptions can fill the
        /// table.
        ///
        void AppendOptions( void ( *fn )( void    *user_data,
                                          size_t  *option_node_size,
                                          size_t **option_node_top_ids ),
                            void *user_data );

        /// Search all solutions.
        ///
        /// === --- visit_fn.
        ///
        /// The visit_fn is called for all solutions found. The user_data and
        /// user_solution are passed from SearchSolutions to visit_fn.
        ///
        /// If the visit_fn returns true, then the searching process stops
        /// immediately.  Otherwise, continues.
        ///
        /// === --- max_solution_size hint & user_solution buffer.
        ///
        /// Users pass the visit_fn, along with the buffer, called
        /// user_solution, to fill the results.
        ///
        /// [Solution Domain.] Each invocation of visit_fn is called with a
        /// user_solution buffer, provided by users, with and the number of
        /// filled results in the buffer, solution_size, i.e.,
        ///
        ///     user_solution[0...solution_size] is the valid.
        ///
        /// The user_solution array is unsorted. Each element's domain is
        /// 0-based option ID, i.e.,
        ///
        ///     [0, n_options - 1].
        ///
        /// [Ownership.] Users own the buffer and need to ensure it is big
        /// enough to hold max_solution_size (see below) data.
        ///
        /// [Efficiency.] The number of options to be filled in the
        /// user_solution buffer is upper bounded by the number of items. But
        /// that upper bound could be too large sometimes, so the users might
        /// know the bound more precisely and take opportunity save memory
        /// spaces.
        ///
        /// To support that, users can provide a smaller buffer and hint the
        /// maximum size to fill the user_solution. This function,
        /// SearchSolutions, will copy at most max_solution_size to the buffer
        /// and stop even this could be partial user_solution.
        ///
        /// [Partial Solution.] To detect this partial case, the buffer could
        /// be allocate +1 size, set a SENT value, and increase the
        /// max_solution_size +1 as well before passing to SearchSolutions.
        /// Inside visit_fn, check whether that SENT has been modified.
        ///
        void SearchSolutions( bool ( *visit_fn )( void   *user_data,
                                                  size_t  solution_size,
                                                  size_t *user_solution ),
                              void *user_data, size_t max_solution_size,
                              size_t *user_solution );
};
}  // namespace taocp
