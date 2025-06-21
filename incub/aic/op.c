#include "op.h"

#include "util.h"

#ifndef NDEBUG
#define DEBUG_PRINT 1
#else
#define DEBUG_PRINT 0
#endif

#define OP_LOGGING_PREFIX "[Op ] "

#define DEBUG( ctx, ... ) \
        if ( DEBUG_PRINT ) LOG_DEBUG( ctx, OP_LOGGING_PREFIX __VA_ARGS__ )

static void
tsr_dump_f32_data_for_debugging( struct ctx *ctx, size_t count, void *data )
{
        size_t dump_count = count;
        if ( dump_count >= 20 ) dump_count = 20;

        f32 *ptr = (f32 *)data;

        sds_t sds = sds_new( "debug f32 tensor: [" );

        for ( size_t i = 0; i < dump_count; i++ ) {
                sds_cat_printf( &sds, "%.4e, ", ptr[i] );
        }

        sds_cat_printf( &sds, "...]" );
        DEBUG( ctx, "%s", sds );
        sds_free( sds );
}

/* === --- Implementation of APIs --------------------------------------- === */

error_t
op_load_weight( struct vm_frame *frame )
{
        error_t    err = OK;
        struct vm *vm  = frame->vm;
        vec_t( byte ) ops =
            vm_program_get_bytecode( frame->program ) + *( frame->ppc ) + 9;

        struct tensor *embedding = (struct tensor *)bytecode_load_u64( ops );
        vm_push_tsr( vm, embedding );
        *( frame->ppc ) += 16; /* skip the weight_name and tensor ptr. */
        return err;
}

error_t
op_gatter( struct vm_frame *frame )
{
        error_t     err = OK;
        struct ctx *ctx = frame->ctx;
        struct vm  *vm  = frame->vm;
        if ( vm_stack_size( vm ) <= 1 ) {
                EMIT_ERROR_NOTE( ctx,
                                 "op_gatter expects two operands on stack." );
                return EINVALID;
        }

        struct tensor *tokens    = NULL;
        struct tensor *embedding = NULL;
        vm_pop_tsr( vm, &embedding );
        vm_pop_tsr( vm, &tokens );

        const struct shape *sp_embedding = tsr_get_shape( embedding );
        const struct shape *sp_tokens    = tsr_get_shape( tokens );

        assert( sp_embedding->rank == 2 );
        assert( sp_tokens->rank == 2 );
        assert( sp_tokens->dims[0] == 1 ); /* Batch size 1 assumption. */

        DEBUG( ctx, "op_gatter" );
        DEBUG( ctx, "-> embedding dim [%d, %d]", sp_embedding->dims[0],
               sp_embedding->dims[1] );
        DEBUG( ctx, "-> token     dim [%d, %d]", sp_tokens->dims[0],
               sp_tokens->dims[1] );

        size_t seq_len = (size_t)sp_tokens->dims[1];
        size_t emb_dim = (size_t)sp_embedding->dims[1];

        f32           *ptr    = NULL;
        struct tensor *output = tsr_new_without_data(
            TSR_DTYPE_F32, 3, (u32[]){ 1, (u32)seq_len, (u32)emb_dim } );
        tsr_alloc_data( output, (void **)&ptr );

        f32 *embedding_data = tsr_get_f32_data( embedding );
        i64 *tokens_data    = tsr_get_i64_data( tokens );

        for ( size_t i = 0; i < seq_len; i++ ) {
                i64 token_id = tokens_data[i];
                memcpy( ptr + i * emb_dim,
                        embedding_data + (size_t)token_id * emb_dim,
                        sizeof( f32 ) * emb_dim );
        }

        tsr_dump_f32_data_for_debugging(ctx, seq_len * emb_dim, ptr);

        vm_push_tsr( vm, output );

        tsr_dec_ref( tokens );
        tsr_dec_ref( embedding );
        return err;
}
