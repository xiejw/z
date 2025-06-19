#include "op.h"

#include "util.h"

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

        tsr_dec_ref( tokens );
        tsr_dec_ref( embedding );
        return err;
}
