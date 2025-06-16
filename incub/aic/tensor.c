#include "tensor.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef NDEBUG
#define DEBUG_PRINT 1
#else
#define DEBUG_PRINT 0
#endif

#define DEBUG( ctx, ... ) \
        if ( DEBUG_PRINT ) LOG_DEBUG( ctx, __VA_ARGS__ )

static error_t
mmap_file( struct ctx *ctx, const char *fname, _OUT_ void **paddr )
{
        int fd = open( fname, O_RDONLY );
        if ( fd == -1 ) {
                EMIT_ERROR_NOTE( ctx, "failed to open tensor data file %s: %s",
                                 fname, strerror( errno ) );
                return EIO;
        }

        error_t err = OK;

        struct stat s;
        if ( fstat( fd, &s ) == -1 ) {
                EMIT_ERROR_NOTE( ctx, "failed to stat tensor data file %s: %s",
                                 fname, strerror( errno ) );
                err = EIO;
                goto cleanup;
        }

        size_t fsize = (size_t)s.st_size;
        DEBUG( ctx, "mmap size for file %s is %zu", fname, fsize );

        void *addr = mmap( NULL, fsize, PROT_READ, MAP_PRIVATE, fd, 0 );
        if ( addr == MAP_FAILED ) {
                EMIT_ERROR_NOTE( ctx, "failed to mmap tensor data file %s: %s",
                                 fname, strerror( errno ) );
                err = EIO;
                goto cleanup;
        }

        *paddr = addr;

cleanup:
        close( fd );
        return err;
}

static struct tensor *
read_tensor_without_data( struct ctx *ctx, char **paddr )
{
        struct tensor *tsr = calloc( 1, sizeof( *tsr ) );
        assert( tsr != NULL );

        u32 rank = *(u32 *)*paddr;
        *paddr += 4;
        DEBUG( ctx, "tensor dim %d", (int)rank );
        assert( rank <= AIC_TENSOR_MAX_RANK );
        tsr->sp.rank = rank;

        u64 ele_count = 1;
        for ( u32 dim = 0; dim < rank; dim++ ) {
                u32 e = *(u32 *)*paddr;
                *paddr += 4;
                tsr->sp.dims[dim] = e;
                ele_count *= (u64)e;
        }
        tsr->sp.ele_count = ele_count;

        DEBUG( ctx, "tensor rank %d with ele_count %zu", (int)tsr->sp.rank,
               (size_t)ele_count );

        tsr->ref_cnt = 1;
        tsr->alias   = 1;
        return tsr;
}

static void
alias_tensor_data( struct ctx *ctx, char *base_addr,
                   vec_t( struct tensor * ) tensors )
{
        (void)ctx;
        size_t offset    = 0;
        size_t tsr_count = vec_size( tensors );
        for ( size_t i = 0; i < tsr_count; i++ ) {
                struct tensor *tsr = tensors[i];
                assert( tsr->dtype == 0 );
                tsr->f = (f32 *)( base_addr + offset );
                offset += tsr->sp.ele_count * sizeof( f32 );
        }
}

static error_t
load_tensors( struct ctx *ctx, char *addr,
              _OUT_ vec_t( struct tensor * ) * ptensors )
{
        /* Pass 1. Read total tensor count. */
        u32 count = *(u32 *)addr;
        addr += 4;
        DEBUG( ctx, "tensor count %d", (int)count );
        assert( count == 1 );

        /* Pass 2. Read all tensor rank and shapes. */
        for ( u32 i = 0; i < count; i++ ) {
                vec_push( ptensors, read_tensor_without_data( ctx, &addr ) );
        }

        /* Pass 3. Adjust the tensor data pointers. */
        alias_tensor_data( ctx, addr, *ptensors );

        return OK;
}

/* === --- Implementation of APIs --------------------------------------- === */

error_t
tsr_load_from_file( struct ctx *ctx, const char *fname,
                    _OUT_ vec_t( struct tensor * ) * ptensors )
{
        void   *addr = NULL;
        error_t err  = mmap_file( ctx, fname, &addr );
        if ( err != OK ) {
                goto cleanup;
        }

        err = load_tensors( ctx, addr, ptensors );
        if ( err != OK ) {
                goto cleanup;
        }

cleanup:
        // TODO if error, we probably should clear the ptensors to original
        // state, clear the tensor allocations, and munmap the file.
        return err;
}

void
tsr_inc_ref( struct tensor *p )
{
        assert( p != NULL );
        assert( p->ref_cnt >= 1 );
        p->ref_cnt++;
}

void
tsr_dec_ref( struct tensor *tsr )
{
        if ( tsr == NULL ) return;
        assert( tsr->ref_cnt > 0 );
        if ( tsr->ref_cnt-- != 1 ) return;
        assert( tsr->dtype == 0 );
        if ( !tsr->alias ) free( tsr->f );
        free( tsr );
}

void
tsr_free_vec( vec_t( struct tensor * ) tensors )
{
        size_t count = vec_size( tensors );
        for ( size_t i = 0; i < count; i++ ) {
                struct tensor *tsr = tensors[i];
                tsr_dec_ref( tsr );
        }
        vec_free( tensors );
}
