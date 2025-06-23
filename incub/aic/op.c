#include "op.h"

#include <math.h>
#include <stdio.h>

#include "util.h"

#ifndef NDEBUG
#define DEBUG_PRINT 1
#else
#define DEBUG_PRINT 0
#endif

#define OP_LOGGING_PREFIX "[Op ] "

#define DEBUG( ctx, ... ) \
        if ( DEBUG_PRINT ) LOG_DEBUG( ctx, OP_LOGGING_PREFIX __VA_ARGS__ )

#define PANIC( msg )                 \
        do {                         \
                printf( "panic\n" ); \
                printf( msg );       \
                exit( 1 );           \
        } while ( 0 )

#define REL_ERROR 1e-2f /* Max relative error allowed during assertion. */

static void
tsr_dump_f32_data_for_debugging( struct ctx *ctx, struct tensor *output,
                                 void *data )
{
        const struct shape *sp         = tsr_get_shape( output );
        size_t              dump_count = sp->ele_count;
        if ( dump_count >= 20 ) dump_count = 20;

        f32 *ptr = (f32 *)data;

        sds_t sds = sds_new( "debug f32 tensor: shape/" );

        for ( u32 dim = 0; dim < sp->rank; dim++ ) {
                u32 e = sp->dims[dim];
                sds_cat_printf( &sds, "%d, ", (int)e );
        }
        sds_cat_printf( &sds, "/ [" );

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

        tsr_dump_f32_data_for_debugging( ctx, output, ptr );

        vm_push_tsr( vm, output );

        tsr_dec_ref( tokens );
        tsr_dec_ref( embedding );
        return err;
}

error_t
op_assert_eq( struct vm_frame *frame )
{
        error_t     err = OK;
        struct ctx *ctx = frame->ctx;
        struct vm  *vm  = frame->vm;
        if ( vm_stack_size( vm ) <= 1 ) {
                EMIT_ERROR_NOTE(
                    ctx, "op_assert_eq expects two operands on stack." );
                return EINVALID;
        }

        struct tensor *op1 = NULL;
        struct tensor *op2 = NULL;
        vm_pop_tsr( vm, &op1 );
        vm_pop_tsr( vm, &op2 );

        /* Check dtype. */
        if ( tsr_get_dtype( op1 ) != TSR_DTYPE_F32 ||
             tsr_get_dtype( op1 ) != tsr_get_dtype( op2 ) ) {
                EMIT_ERROR_NOTE( ctx,
                                 "op_assert_eq expects f32 operands only." );
                return EINVALID;
        }

        /* Check shape and ele_count. */
        const struct shape *sp_op1 = tsr_get_shape( op1 );
        const struct shape *sp_op2 = tsr_get_shape( op2 );

        if ( sp_op1->ele_count != sp_op2->ele_count ) {
                EMIT_ERROR_NOTE(
                    ctx,
                    "op_assert_eq expects equal-ele_count operands only." );
                return EINVALID;
        }

        f32   *data_op1  = tsr_get_f32_data( op1 );
        f32   *data_op2  = tsr_get_f32_data( op2 );
        size_t ele_count = (size_t)sp_op1->ele_count;

        for ( u32 i = 0; i < ele_count; i++ ) {
                f32 ferr = data_op1[i] - data_op2[i];
                if ( fabsf( ferr / data_op1[i] ) >= REL_ERROR ) {
                        printf( "the %u-th ele is not same. %g vs %g\n", i,
                                data_op1[i], data_op2[i] );
                        fflush( stdout );
                        PANIC( "op_assert_eq" );
                }
        }

        tsr_dec_ref( op1 );
        tsr_dec_ref( op2 );
        return err;
}
