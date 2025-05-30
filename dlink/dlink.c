#ifndef ADT_DLINK_H_
#define ADT_DLINK_H_

#include <stdlib.h>

// === --- Inject Common Types all ADTs Use ------------------------------------
//
// Assumes this code is same everywhere.
//
#ifndef ADT_TYPES_H_
#define ADT_TYPES_H_

// Primitive Types
#include <stdint.h>

typedef uint64_t u64;
typedef int64_t  i64;
typedef uint32_t u32;
typedef int32_t  i32;
typedef uint8_t  u8;
typedef float    f32;
typedef double   f64;

// Result and Error Codes
typedef int error_t;

#define OK        0
#define ERROR     -1
#define EMALLOC   -2
#define ENOTEXIST -3
#define ENOTIMPL  -4

// Function Parameter Annotations
#define _MUT_       // The field might be mutated if new address is allocated
#define _OUT_       // The field will be set with the output
#define _INOUT_     // The field will be passed in and then be set as output
#define _MOVED_IN_  // The ownership is moved into the method
#define _NULLABLE_  // The field is Nullable

// Function Annotations
#define ADT_UNUSED_FN __attribute__( ( unused ) )

#endif /* ADT_TYPES_H_ */

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

// // Return the solutions as header ids.
// auto SearchSolution( ) -> std::optional<std::vector<std::size_t>>;

/* === --- Implementation --------------------------------------------------- */

struct dlink_node {
        size_t Id;
        size_t L;
        size_t R;
        size_t U;
        size_t D;
        union {
                size_t C;  // head Id of the vertical col. used by non-head.
                size_t S;  // count of the vertical col. used by head.
        };
        void *data;  // unowned.
};
//
// The dancing link table used to solve the problem.
struct dlink_tbl {
        size_t             num_nodes_total;
        size_t             num_nodes_added;
        struct dlink_node *nodes;
};

//      public:
//      public:
//      private:
//        auto FillNode( Node &node, std::size_t Id ) -> void;
//        auto LinkLR( Node *h, size_t end, size_t Id ) -> void;
//        auto LinkUD( Node *h, size_t id_c, size_t Id ) -> void;
//        auto CoverColumn( Node *h, size_t c ) -> void;
//        auto UncoverColumn( Node *h, size_t c ) -> void;
//        auto Search( std::vector<std::size_t> &sols, std::size_t depth )
//            -> bool;
//};

void *
dlink_get_node_data( struct dlink_tbl *p, size_t id )
{
        return p->nodes[id].data;
}

#endif /* ADT_DLINK_H_ */

/* === --- Test Code -------------------------------------------------------- */

#ifdef ADT_TEST_H_
#include <stdio.h>

int
main( void )
{
        printf( "Test passed.\n" );
}
#endif /* ADT_TEST_H_ */
