#include "llama.h"

#include "vm.h"

#define DEBUG_PRINT 1

#define MODEL_LOGGING_PREFIX "[Llama] "

#define DEBUG( ctx, ... ) \
        if ( DEBUG_PRINT ) LOG_DEBUG( ctx, MODEL_LOGGING_PREFIX __VA_ARGS__ )

/* === --- Implementation of APIs --------------------------------------- === */

error_t
model_new( struct ctx *ctx, const char *fname,
           _OUT_ struct llama_model **model )
{
        error_t             err          = OK;
        struct llama_model *p            = calloc( 1, sizeof( *p ) );
        vec_t( struct tensor * ) tensors = vec_new( );

        err = tsr_load_from_file( ctx, fname, &tensors );
        if ( err != OK ) {
                EMIT_ERROR_NOTE( ctx, "failed to load tensors" );
                goto cleanup;
        }

        p->embedding = tensors[0];

        p->tensors = _MOVED_IN_ tensors;
        tensors    = NULL;

        *model = p;

        struct vm *vm = vm_new( ctx );
        p->vm         = vm;
cleanup:
        if ( err != OK ) {
                if ( p != NULL ) model_free( p );
                if ( tensors != NULL ) tsr_free_vec( tensors );
        }
        return err;
}

void
model_free( struct llama_model *p )
{
        if ( p == NULL ) return;
        tsr_free_vec( p->tensors );
        vm_free( p->vm );
        free( p );
}

error_t
model_run( struct llama_model *model )
{
        error_t err = OK;
        // TODO the program should be program'ed once.
        struct vm_program *program = vm_program_new( model->vm );

#define CHECK_AND_JUMP( err )                                          \
        if ( ( err ) != OK ) {                                         \
                EMIT_ERROR_NOTE( model->ctx, "failed to program op" ); \
                goto cleanup;                                          \
        }

        CHECK_AND_JUMP( vm_program_push_op( program, OP_GATTER ) );

#undef CHECK_AND_JUMP

        if ( DEBUG_PRINT ) {
                sds_t s = vm_program_dump( program );
                DEBUG( model->ctx, "Program:\n%s", s );
                sds_free( s );
        }

cleanup:
        vm_program_free( program );
        return err;
}
