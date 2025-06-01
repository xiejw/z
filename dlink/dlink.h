#ifndef ADT_DLINK_H_
#define ADT_DLINK_H_

#include <stdlib.h>

#include <adt/types.h>

// === --- APIs ----------------------------------------------------------------

// Forward declaration.
struct dlink_tbl;

// One time allocation to reserve in total 1 + n_col_heads +
// n_options_total nodesin the dlink table.
//
// It must cover 1 system header, all column heads (items) and all
// options. In addition, also allocate n_col_heads column heads and link
// them as the horizantal link list.
struct dlink_tbl *dlink_new( size_t n_col_heads, size_t n_options_total );
void              dlink_free( struct dlink_tbl * );

// Cover all nodes in a column. Also unlink nodes from their columns
// belonging to the same option.
void dlink_cover_col( struct dlink_tbl *p, size_t c );

// Append a group of options which are mutually exclusive.
//
// All nodes will be linked to the vertical list of column head and
// horizantal list of this group.
//
// The `priv_data` is not owned by this table.
void dlink_append_opt( struct dlink_tbl *p, size_t num_ids, size_t *col_ids,
                       void *priv_data );

void *dlink_get_node_data( struct dlink_tbl *p, size_t id );

// Attempt to search a solution.
//
// - Return OK if find a solution and fill the header ids in sol. The
//   sol must be pre-allocated. Number of header ids filled is set in num_sol.
// - Return ENOTEXIST if no solution
error_t dlink_search( struct dlink_tbl *p, size_t *sol, size_t *num_sol );

#endif /* ADT_DLINK_H_ */
