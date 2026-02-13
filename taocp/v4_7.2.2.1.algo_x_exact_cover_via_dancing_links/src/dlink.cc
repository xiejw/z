// forge:v1
#include "dlink.h"

#include <assert.h>

namespace taocp {

// === --- Data Structures ------------------------------------------------- ===
struct DLinkNode {
        union {
                size_t l;  // LLINK. Used by horizontal item list.
                size_t u;  // ULINK. Used by table.
        };
        union {
                size_t r;  // RLINK. Used by horizontal item list.
                size_t d;  // DLINK. Used by table.
        };
        union {
                // Unowned user data. Used by horizontal item list.
                void *data;
                // Count of the vertical col. Used by vertical header.
                size_t len;
                // Item Id of the vertical col. Used by option node.
                size_t top;
        };
};

namespace {
// // Fill a DLinkNode to initialize it for horizontal
// void
// DLinkNodeFill( DLinkNode *node, size_t id )
// {
//      node->id = id;
//      node->l  = id;
//      node->r  = id;
//      node->u  = id;
//      node->d  = id;
//      node->c  = 0;
// }
//
// // Link the `id` into table after node `end` (horizontal double link)
// static void
// dlink_link_lr( struct dlink_node *h, size_t end, size_t id )
// {
//      struct dlink_node *p = &h[id];
//      p->l                 = end;
//      p->r                 = h[end].r;
//      h[end].r             = id;
//      h[p->r].l            = id;
// }
}

DLinkTable::DLinkTable( size_t n_items, size_t n_options,
                        size_t n_option_nodes )
    : n_items( n_items ),
      n_options( n_options ),
      n_option_nodes( n_option_nodes ),
      mem(  // horizontal list
          1 + n_items +
          // vertical headers
          1 + n_items +
          // spacer nodes
          1 + n_options +
          // option nodes
          n_option_nodes )
{
        // Set up all memory pointers.
        this->item_list      = this->mem.data( );
        this->item_list_size = 1 + n_items;
        this->table          = this->mem.data( ) + this->item_list_size;
        this->table_size     = 1 + n_items + n_options + n_option_nodes;
        assert( this->item_list_size + this->table_size == this->mem.size( ) );

        // Link the horizontal item list.
        auto *item_list      = this->item_list;
        item_list[0].l       = n_items;  // Same as this->item_list_size - 1;
        item_list[0].r       = 1;
        item_list[n_items].l = n_items - 1;
        item_list[n_items].r = 0;
        for ( size_t i = 1; i < n_items; i++ ) {
                item_list[i].l = i - 1;
                item_list[i].r = i + 1;
        }

        // Link the vertical headers.
        for ( size_t i = 1; i <= n_items; i++ ) {
                auto *n = this->table + i;
                n->len  = 0;
                n->u    = i;
                n->d    = i;
        }
}

DLinkNode *
DLinkTable::GetHorizontalItem( size_t i )
{
        assert( i <= this->item_list_size );
        return &this->item_list[i];
}

DLinkNode *
DLinkTable::GetTableItem( size_t i )
{
        assert( i <= this->table_size );
        return &this->table[i];
}
}  // namespace taocp

/*


// === --- Private helper utils ---------------------------------------------



// Link the `id` with item column `item_id` (vertical double link).
static void
dlink_link_ud( struct dlink_node *h, size_t item_id, size_t id )
{
     struct dlink_node *c = &h[item_id];
     struct dlink_node *p = &h[id];

     p->c = item_id;
     c->s += 1;

     size_t id_end = c->u;
     c->u          = id;
     h[id_end].d   = id;
     p->d          = item_id;
     p->u          = id_end;
}

// Covers a item in the table and unlink all options linked with this item.
static void
dlink_cover_item_impl( struct dlink_node *h, size_t item_id )
{
     h[h[item_id].r].l = h[item_id].l;
     h[h[item_id].l].r = h[item_id].r;
     for ( size_t i = h[item_id].d; i != item_id; i = h[i].d ) {
             for ( size_t j = h[i].r; j != i; j = h[j].r ) {
                     h[h[j].d].u = h[j].u;
                     h[h[j].u].d = h[j].d;
                     ( h[h[j].c].s )--;
             }
     }
}

// Undo dlink_cover_item_impl
static void
dlink_uncover_item_impl( struct dlink_node *h, size_t item_id )
{
     for ( size_t i = h[item_id].u; i != item_id; i = h[i].u ) {
             for ( size_t j = h[i].l; j != i; j = h[j].l ) {
                     ( h[h[j].c].s )++;
                     h[h[j].d].u = j;
                     h[h[j].u].d = j;
             }
     }
     h[h[item_id].r].l = item_id;
     h[h[item_id].l].r = item_id;
}

static error_t
dlink_search_impl( struct dlink_tbl *p, size_t *sols, size_t *num_sol,
                size_t k )
{
     struct dlink_node *h = p->nodes;

     if ( h[0].r == 0 ) {
             *num_sol = k;
             return OK;
     }

     size_t c = h[0].r;
     if ( h[c].s == 0 ) {
             return ENOTEXIST;
     }

     dlink_cover_item_impl( h, c );
     for ( size_t r = h[c].d; r != c; r = h[r].d ) {
             sols[k] = r;
             for ( size_t j = h[r].r; j != r; j = h[j].r ) {
                     dlink_cover_item_impl( h, h[j].c );
             }
             if ( dlink_search_impl( p, sols, num_sol, k + 1 ) == OK )
                     return OK;

             for ( size_t j = h[r].l; j != r; j = h[j].l ) {
                     dlink_uncover_item_impl( h, h[j].c );
             }
     }
     dlink_uncover_item_impl( h, c );
     return ENOTEXIST;
}

// === --- Implementation ---------------------------------------------------


void
dlink_free( struct dlink_tbl *p )
{
     if ( p == NULL ) return;
     free( p->nodes );
     free( p );
}

void
dlink_cover_item( struct dlink_tbl *p, size_t c )
{
     dlink_cover_item_impl( p->nodes, c );
}

void
dlink_append_opt( struct dlink_tbl *p, size_t num_ids, size_t *item_ids,
               void *priv_data )
{
     struct dlink_node *nodes     = p->nodes;
     size_t             offset_id = p->num_nodes_added;

     if ( offset_id + num_ids > p->num_nodes_total ) {
             printf(
                 "Reserved space is not enough for dancing link table: "
                 "reserved "
                 "with %zu, used %zu, needed %zu more.\n",
                 p->num_nodes_total, offset_id, num_ids );
             PANIC( );
     }

     for ( size_t i = 0; i < num_ids; i++ ) {
#ifndef NDEBUG
             assert( item_ids[i] > 0 && item_ids[i] <= p->num_items );
#endif
             size_t id = offset_id + i;
             dlink_fill_node( &nodes[id], id );
             dlink_link_ud( nodes, item_ids[i], id );
             nodes[id].data = priv_data;
             if ( i != 0 ) {
                     dlink_link_lr( nodes, id - 1, id );
             }
     }

     p->num_nodes_added += num_ids;
}

void *
dlink_get_node_data( struct dlink_tbl *p, size_t id )
{
#ifndef NDEBUG
     assert( id > p->num_items );
#endif
     return p->nodes[id].data;
}

error_t
dlink_search( struct dlink_tbl *p, size_t *sols, size_t *num_sol )
{
     if ( OK == dlink_search_impl( p, sols, num_sol, 0 ) ) return OK;
     return ENOTEXIST;
}
*/
