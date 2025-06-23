#include "llama.h"

#include "vm.h"

#define DEBUG_PRINT 1

#define MODEL_LOGGING_PREFIX "[Llama] "

#define DEBUG( ctx, ... ) \
        if ( DEBUG_PRINT ) LOG_DEBUG( ctx, MODEL_LOGGING_PREFIX __VA_ARGS__ )

/* === --- Data Structures ---------------------------------------------- === */

struct weight_info {
        const char    *name;   /* Alias */
        struct tensor *weight; /* Alias */
};

struct llama_model {
        struct ctx *ctx;                  /* Now owned. */
        struct vm  *vm;                   /* Owned. */
        vec_t( struct tensor * ) tensors; /* Owned */

        // DEBUG only
        vec_t( struct tensor * ) outputs; /* Owned */

        struct weight_info embedding;
};

/* === --- Implementation of APIs --------------------------------------- === */

error_t
llama_model_new( struct ctx *ctx, const char *fname,
                 _OUT_ struct llama_model **model )
{
        error_t err                      = OK;
        vec_t( struct tensor * ) tensors = vec_new( );
        vec_t( struct tensor * ) outputs = vec_new( );

        // DEBUG only
        err = tsr_load_from_file( ctx, "/tmp/output_data.bin", &outputs );
        if ( err != OK ) {
                vec_free( tensors );
                vec_free( outputs );
                EMIT_ERROR_NOTE( ctx, "failed to load outputs" );
                goto exit;
        }

        err = tsr_load_from_file( ctx, fname, &tensors );
        if ( err != OK ) {
                vec_free( tensors );
                vec_free( outputs );
                EMIT_ERROR_NOTE( ctx, "failed to load tensors" );
                goto exit;
        }

        struct llama_model *p  = calloc( 1, sizeof( *p ) );
        struct vm          *vm = vm_new( ctx );

        p->ctx     = ctx;
        p->vm      = _MOVED_IN_      vm;
        p->tensors = _MOVED_IN_ tensors;

        assert( vec_size( outputs ) == 1 );
        p->outputs = _MOVED_IN_ outputs;

        /* Fill model weights */
        size_t tensor_idx = 0;
        p->embedding      = (struct weight_info){ .name   = "embedding",
                                                  .weight = tensors[tensor_idx++] };

        *model = p;

exit:
        return err;
}

void
llama_model_free( struct llama_model *p )
{
        if ( p == NULL ) return;
        tsr_free_vec( p->tensors );
        tsr_free_vec( p->outputs );
        vm_free( p->vm );
        free( p );
}

#define CHECK_AND_JUMP( err )                                          \
        if ( ( err ) != OK ) {                                         \
                EMIT_ERROR_NOTE( model->ctx, "failed to program op" ); \
                goto cleanup;                                          \
        }

error_t
llama_model_run( struct llama_model *model, const vec_t( i64 ) tokens )
{
        error_t            err        = OK;
        struct vm_program *program    = NULL;
        struct tensor     *tsr_tokens = NULL;
        struct vm         *vm         = model->vm;

        /* --- Prepare the program. ----------------------------------------- */

        // TODO the program should be program'ed once.
        program = vm_program_new( vm );

        CHECK_AND_JUMP( vm_program_push_op( program, OP_LOAD_WEIGHT,
                                            model->embedding.name,
                                            model->embedding.weight ) );
        CHECK_AND_JUMP( vm_program_push_op( program, OP_GATTER ) );
        CHECK_AND_JUMP( vm_program_push_op( program, OP_ASSERT_EQ ) );

        if ( DEBUG_PRINT ) {
                sds_t s = vm_program_dump( program );
                DEBUG( model->ctx, "Program:\n%s", s );
                sds_free( s );
        }

        /* --- Prepare the token as tensor ---------------------------------- */
        u32 seq_len = (u32)vec_size( tokens );
        tsr_tokens  = tsr_new_without_data( TSR_DTYPE_I64, 2,
                                            (u32 *)(u32[]){ 1, seq_len } );

        tsr_alias_data( tsr_tokens, (void *)tokens );

        vm_push_tsr( vm, model->outputs[0] );
        vm_push_tsr( vm, tsr_tokens );

        /* --- Run ---------------------------------------------------------- */
        err = vm_run( vm, program );
        if ( err != OK ) {
                EMIT_ERROR_NOTE( model->ctx, "fail to run the model" );
                goto cleanup;
        }
        // TODO this is 1 but during debug it is 0 due to OP_ASSERT_EQ
        assert( vm_stack_size( vm ) == 0 );

cleanup:
        while ( vm_stack_size( vm ) > 0 ) {
                struct tensor *tsr;
                vm_pop_tsr( vm, &tsr );
                tsr_dec_ref( tsr );
        }
        tsr_dec_ref( tsr_tokens );
        vm_program_free( program );
        return err;
}

#undef CHECK_AND_JUMP
