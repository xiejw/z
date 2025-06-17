#include "op.h"

error_t
op_gatter( struct ctx *ctx, struct vm *vm, struct vm_program *p, size_t pc )
{
        if ( vm_stack_size( vm ) <= 1 ) {
                EMIT_ERROR_NOTE( ctx,
                                 "op_gatter expects two operands on stack." );
                return EINVALID;
        }
        ADT_MAYBE_UNUSED( p );
        ADT_MAYBE_UNUSED( pc );

        error_t err = OK;

        struct tensor *tokens    = NULL;
        struct tensor *embedding = NULL;
        vm_pop_tsr( vm, &tokens );
        vm_pop_tsr( vm, &embedding );

        tsr_dec_ref( tokens );
        tsr_dec_ref( embedding );
        return err;
}
