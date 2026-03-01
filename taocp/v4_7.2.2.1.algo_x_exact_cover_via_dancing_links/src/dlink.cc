// forge:v1
#include "dlink.h"

#include <assert.h>
#include <stdio.h>

#define DEBUG 0
#define DEBUG_PRINT \
        if ( DEBUG ) printf

#define MAYBE_UNUSED( x ) (void)( x )

namespace taocp {

// === --- Data Structures ------------------------------------------------- ===

namespace {

// Link the `id` with item `top_id` (vertical double link).
void
LinkUD( DLinkNode *h, size_t top_id, size_t id )
{
        DLinkNode *t = &h[top_id];
        DLinkNode *p = &h[id];

        p->top = ssize_t( top_id );
        t->len += 1;

        size_t id_end = t->u;
        t->u          = id;
        h[id_end].d   = id;
        p->d          = top_id;
        p->u          = id_end;
}

/// (13) on Volume 4B, Page 69
void
HideNode( DLinkNode *nodes, size_t p )
{
        size_t q = p + 1;
        while ( q != p ) {
                DLinkNode *n = &nodes[q];
                ssize_t    x = n->top;
                size_t     u = n->u;
                size_t     d = n->d;
                if ( x <= 0 ) {  // A spacer
                        q = u;
                } else {
                        nodes[u].d = d;
                        nodes[d].u = u;
                        nodes[x].len--;
                        q++;
                }
        }
}

/// Covers a item in the table and unlink all options linked with this item.
///
/// (12) on Volume 4B, Page 68
void
CoverItem( DLinkItem *items, size_t item_id, DLinkNode *nodes )
{
        for ( size_t p = nodes[item_id].d; p != item_id; p = nodes[p].d ) {
                HideNode( nodes, p );
        }

        size_t l   = items[item_id].l;
        size_t r   = items[item_id].r;
        items[l].r = r;
        items[r].l = l;
}

/// (15) on Volume 4B, Page 69
void
UnhideNode( DLinkNode *nodes, size_t p )
{
        size_t q = p - 1;
        while ( q != p ) {
                DLinkNode *n = &nodes[q];
                ssize_t    x = n->top;
                size_t     u = n->u;
                size_t     d = n->d;
                if ( x <= 0 ) {  // A spacer
                        q = d;
                } else {
                        nodes[u].d = q;
                        nodes[d].u = q;
                        nodes[x].len++;
                        q--;
                }
        }
}

/// Uncovers a item in the table and link all options linked with this item.
///
/// (14) on Volume 4B, Page 69
void
UncoverItem( DLinkItem *items, size_t item_id, DLinkNode *nodes )
{
        size_t l   = items[item_id].l;
        size_t r   = items[item_id].r;
        items[l].r = item_id;
        items[r].l = item_id;

        for ( size_t p = nodes[item_id].u; p != item_id; p = nodes[p].u ) {
                UnhideNode( nodes, p );
        }
}

// Visit a solution and call the user passed-in visit_fn.
//
// NOTE: The X array records the node in the DLinkTable. We need to deduce the
// 0-based Option ID and then set Option ID in the solution buffer. To do so,
// there are two approaches
//
// 1. Store the Option ID in each node of the DLinkTable. Then the access is
//    O(1) but it takes n_option_nodes spaces
// 2. Avoid this memory cost but linearly scan the spacer (top <= 0). Spacer
//    has the Option ID stored in the top.
// 3. We could internally maintain a small Option ID array to store the spacer
//    ID in the table. Then a binary search could find the Option ID very
//    quickly. Space cost is O(n_options), and time cost is O(log(n_options)).
//
// We would avoid (1), and now use (2). (3) can be used later.
//
bool
VisitSolution( size_t n_items, size_t n_options, DLinkNode *nodes, size_t *X,
               size_t l,
               bool ( *visit_fn )( void *user_data, size_t solution_size,
                                   size_t *solution ),
               void *user_data, size_t max_solution_size, size_t *solution )
{
        MAYBE_UNUSED( n_items );
        MAYBE_UNUSED( n_options );

        if ( l == 0 ) {  // Special case
                return visit_fn( user_data, 0, solution );
        }

        // Translate result
        for ( size_t x = 0; x < l; x++ ) {
                if ( x >= max_solution_size ) {  // Partial solution case.
                        return visit_fn( user_data, max_solution_size,
                                         solution );
                }

                size_t id = X[x];
                assert( id > n_items );

                // Scan the spacers to get the option ID.
                size_t p = id;
                do {
                        p--;
                        assert( p > n_items );
                } while ( nodes[p].top > 0 );

                size_t option_id = size_t( -1 * ( nodes[p].top ) );
                assert( option_id < n_options );
                solution[x] = option_id;
                DEBUG_PRINT( "Sol[%d] = %d\n", (int)x, (int)solution[x] );
        }

        return visit_fn( user_data, l, solution );
}

// Algorithm X: Volume 4B, Page 69
void
SearchSolutions( size_t n_items, size_t n_options, DLinkItem *items,
                 DLinkNode *nodes, size_t *X,
                 bool ( *visit_fn )( void *user_data, size_t solution_size,
                                     size_t *solution ),
                 void *user_data, size_t max_solution_size, size_t *solution )
{
        size_t i;
        size_t l;
X1:  // Initialize.
        l = 0;

X2:  // Enter level l.

        DEBUG_PRINT( "enter level = %d items.r %d\n", int( l ),
                     int( items[0].r ) );

        if ( items[0].r == 0 ) {
                if ( VisitSolution( n_items, n_options, nodes, X, l, visit_fn,
                                    user_data, max_solution_size, solution ) ) {
                        // Early Terminate
                        return;
                }
                goto X8;
        }

X3:  // Choose i
        // TODO: MEV heuristic. Exercise 9.
        i = items[0].r;

X4:  // Cover i
        CoverItem( items, i, nodes );
        X[l] = nodes[i].d;

X5:  // Try x_l
        if ( X[l] == i ) goto X7;

        // Cover all items in Option that contains x_l
        {
                size_t x_l = X[l];
                size_t p   = x_l + 1;
                while ( p != x_l ) {
                        ssize_t j = nodes[p].top;
                        if ( j <= 0 ) {
                                p = nodes[p].u;  // A spacer
                        } else {
                                CoverItem( items, size_t( j ), nodes );
                                p++;
                        }
                }
        }

        l++;
        goto X2;

X6:  // Try again.

        // Undo X5
        {
                size_t x_l = X[l];
                size_t p   = x_l - 1;
                while ( p != x_l ) {
                        ssize_t j = nodes[p].top;
                        if ( j <= 0 ) {
                                p = nodes[p].d;  // A spacer
                        } else {
                                UncoverItem( items, size_t( j ), nodes );
                                p--;
                        }
                }
        }

        i    = size_t( nodes[X[l]].top );
        X[l] = nodes[X[l]].d;
        goto X5;

X7:  // Backtrack

        UncoverItem( items, i, nodes );

X8:  // Leave level l
        DEBUG_PRINT( "leave level = %d\n", int( l ) );

        if ( l == 0 ) {
                return;
        }
        l--;
        goto X6;
}

}  // namespace

DLinkTable::DLinkTable( size_t n_items, size_t n_options,
                        size_t n_option_nodes )
    : n_items( n_items ),
      n_options( n_options ),
      item_list( 1 + n_items ),  // horizontal list
      table( 1 + n_items +
             // spacer nodes
             1 + n_options +
             // option nodes
             n_option_nodes )  // vertical headers
{
        assert( n_items >= 1 );
        assert( n_options >= 1 );
        assert( n_option_nodes >= 1 );

        // Link the horizontal item list.
        auto *item_list = this->item_list.data( );
        // item_list[0].id       = 0;
        item_list[0].l = n_items;  // Same as this->item_list_size - 1;
        item_list[0].r = 1;
        // item_list[n_items].id = n_items;
        item_list[n_items].l = n_items - 1;
        item_list[n_items].r = 0;
        for ( size_t i = 1; i < n_items; i++ ) {
                // item_list[i].id = i;
                item_list[i].l = i - 1;
                item_list[i].r = i + 1;
        }

        // Link the vertical headers.
        for ( size_t i = 1; i <= n_items; i++ ) {
                auto *n = &this->table[i];
                n->len  = 0;
                n->u    = i;
                n->d    = i;
        }
}

void
DLinkTable::AppendOptions( void ( *fn )( void    *user_data,
                                         size_t  *option_node_size,
                                         size_t **option_node_top_ids ),
                           void *user_data )
{
        DLinkNode *table = this->table.data( );

        size_t  option_node_size;
        size_t *option_node_top_ids;

        // Spacers start with position at 1 + num_items;
        size_t     spacer_count = 0;
        size_t     spacer_id    = 1 + n_items;
        DLinkNode *last_spacer  = &table[spacer_id];
        last_spacer->top        = -1 * ssize_t( spacer_count );

        size_t current_id = spacer_id + 1;

        while ( true ) {
                if ( current_id >= this->table.size( ) ) {
                        assert( current_id == this->table.size( ) );
                        break;  // End of filling options;
                }

                // Get next row of option nodes.
                fn( user_data, &option_node_size, &option_node_top_ids );

                // Validate size
                assert( option_node_size >= 1 );
                assert( current_id + option_node_size + /*spacer*/ 1 <=
                            this->table.size( ) &&
                        "overflow table size" );

                // Fill option nodes
                for ( size_t x = 0; x < option_node_size; x++ ) {
                        size_t top_id = option_node_top_ids[x];
                        LinkUD( table, top_id, current_id + x );
                }

                // Update last spacer
                last_spacer->d = current_id + option_node_size - 1;

                // Move to new spacer.
                last_spacer += option_node_size + 1;
                spacer_count++;
                last_spacer->top = -1 * ssize_t( spacer_count );
                last_spacer->u   = current_id;

                // Update current_id
                current_id += /*spacer */ 1 + option_node_size;
        }
}

void
DLinkTable::SearchSolutions( bool ( *visit_fn )( void   *user_data,
                                                 size_t  solution_size,
                                                 size_t *solution ),
                             void *user_data, size_t max_solution_size,
                             size_t *solution )
{
        std::vector<size_t> X( this->n_items );  // At most n_items.

        ::taocp::SearchSolutions( this->n_items, this->n_options,
                                  this->item_list.data( ), this->table.data( ),
                                  X.data( ), visit_fn, user_data,
                                  max_solution_size, solution );
}

void
DLinkTable::CoverItem( size_t item_id )
{
        assert( item_id > 0 && item_id <= this->n_items );
        ::taocp::CoverItem( this->item_list.data( ), item_id,
                            this->table.data( ) );
}

}  // namespace taocp
