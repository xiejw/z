#include <algos/dal.h>

#include <cassert>

#include <eve/base/error.h>

namespace eve::algos::dal {

auto
Table::FillNode( Node &node, std::size_t Id ) -> void
{
        node.Id = Id;
        node.L  = Id;
        node.R  = Id;
        node.U  = Id;
        node.D  = Id;
        node.C  = 0;
}

// Link the `Id` into table after node `end` (horizantal double link)
auto
Table::LinkLR( Node *h, size_t end, size_t Id ) -> void
{
        auto p    = &h[Id];
        p->L      = end;
        p->R      = h[end].R;
        h[end].R  = Id;
        h[p->R].L = Id;
}

// Link the `Id` into table with column head `Id_c` (vertical double link)
auto
Table::LinkUD( Node *h, size_t Id_c, size_t Id ) -> void
{
        auto *c = &h[Id_c];
        auto *p = &h[Id];
        p->C    = Id_c;

        c->S += 1;

        size_t Id_end = c->U;
        c->U          = Id;
        h[Id_end].D   = Id;
        p->D          = Id_c;
        p->U          = Id_end;
}

auto
Table::CoverColumn( Node *h, size_t c ) -> void
{
        h[h[c].R].L = h[c].L;
        h[h[c].L].R = h[c].R;
        for ( size_t i = h[c].D; i != c; i = h[i].D ) {
                for ( size_t j = h[i].R; j != i; j = h[j].R ) {
                        h[h[j].D].U = h[j].U;
                        h[h[j].U].D = h[j].D;
                        ( h[h[j].C].S )--;
                }
        }
}

auto
Table::UncoverColumn( Node *h, size_t c ) -> void
{
        for ( size_t i = h[c].U; i != c; i = h[i].U ) {
                for ( size_t j = h[i].L; j != i; j = h[j].L ) {
                        ( h[h[j].C].S )++;
                        h[h[j].D].U = j;
                        h[h[j].U].D = j;
                }
        }
        h[h[c].R].L = c;
        h[h[c].L].R = c;
}

Table::Table( std::size_t n_col_heads, std::size_t n_options_total )
    : mNodes{ nullptr, std::free }
{
        auto total_reserved_nodes_count = 1 + n_col_heads + n_options_total;
        this->mNumNodesTotal            = total_reserved_nodes_count;
        this->mNumNodesAdded            = 1 + n_col_heads;
        this->mNodes.reset(
            (Node *)malloc( sizeof( Node ) * total_reserved_nodes_count ) );

        auto nodes = this->mNodes.get( );
        FillNode( nodes[0], /*Id=*/0 );

        for ( std::size_t i = 1; i <= n_col_heads; i++ ) {
                FillNode( nodes[i], i );
                LinkLR( nodes, i - 1, i );
        }
}

auto
Table::CoverCol( size_t c ) -> void
{
        CoverColumn( this->mNodes.get( ), c );
}

auto
Table::AppendOption( std::span<std::size_t> col_Ids, void *Data ) -> void
{
        auto  *nodes     = this->mNodes.get( );
        size_t offset_Id = this->mNumNodesAdded;
        auto   num_Ids   = col_Ids.size( );

        if ( offset_Id + num_Ids > this->mNumNodesTotal ) {
                panic(
                    "Reserved space is not enough for dancing link table: "
                    "reserved "
                    "with %d, used %d, needed %d more.",
                    this->mNumNodesTotal, offset_Id, num_Ids );
        }

        for ( size_t i = 0; i < num_Ids; i++ ) {
                size_t Id = offset_Id + i;
                FillNode( nodes[Id], Id );
                LinkUD( nodes, col_Ids[i], Id );
                nodes[Id].Data = Data;
                if ( i != 0 ) {
                        LinkLR( nodes, Id - 1, Id );
                }
        }

        this->mNumNodesAdded += num_Ids;
}

auto
Table::SearchSolution( std::vector<std::size_t> &sols ) -> bool
{
        assert( sols.size( ) == 0 );
        return Search( sols, 0 );
}

auto
Table::Search( std::vector<std::size_t> &sols, std::size_t k ) -> bool
{
        auto h = this->mNodes.get( );

        if ( h[0].R == 0 ) {
                return true;
        }

        size_t c = h[0].R;
        if ( h[c].S == 0 ) {
                return false;
        }

        this->CoverColumn( h, c );
        for ( size_t r = h[c].D; r != c; r = h[r].D ) {
                if ( sols.size( ) == k ) {
                        sols.push_back( r );
                } else {
                        sols[k] = r;
                }
                assert( sols.size( ) >= k );
                for ( size_t j = h[r].R; j != r; j = h[j].R ) {
                        this->CoverColumn( h, h[j].C );
                }
                if ( Search( sols, k + 1 ) ) return true;
                for ( size_t j = h[r].L; j != r; j = h[j].L ) {
                        this->UncoverColumn( h, h[j].C );
                }
        }
        this->UncoverColumn( h, c );
        return false;
}

}  // namespace eve::algos::dal
