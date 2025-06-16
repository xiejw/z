#include "llama.h"

#include "vm.h"

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
        return OK;
}

void
model_free( struct llama_model *p )
{
        if ( p == NULL ) return;
        tsr_free_vec( p->tensors );
        free( p );
}
