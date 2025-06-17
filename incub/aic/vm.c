#include "vm.h"

#include "op.h"

#include <adt/vec.h>

#define VM_STACK_MAX_SIZE 256

/* === --- Data Structures ---------------------------------------------- === */
struct vm_value {
        // TODO add value tag.
        struct tensor *tsr;
};

struct vm {
        struct ctx     *ctx; /* Not owned */
        struct vm_value stack[VM_STACK_MAX_SIZE];
        size_t          sp; /* Point to next position. */
};

struct vm_program {
        struct vm *vm; /* Not owned */
        vec_t( enum vm_op ) ops;
};

/* === --- Implementation of APIs --------------------------------------- === */
struct vm_program *
vm_program_new( struct vm *vm )
{
        struct vm_program *p = malloc( sizeof( *p ) );
        p->vm                = vm;
        p->ops               = vec_new( );
        return p;
}

void
vm_program_free( struct vm_program *p )
{
        if ( p == NULL ) return;
        vec_free( p->ops );
        free( p );
}

error_t
vm_program_push_op( struct vm_program *p, enum vm_op op )
{
        vec_push( &p->ops, op );
        return OK;
}

sds_t
vm_program_dump( struct vm_program *p )
{
        sds_t  s        = sds_empty( );
        size_t op_count = vec_size( p->ops );
        if ( op_count == 0 ) {
                sds_cat_printf( &s, "(empty program)" );
                return s;
        }

        for ( size_t i = 0; i < op_count; i++ ) {
                enum vm_op op = p->ops[i];
                switch ( op ) {
                case OP_GATTER:
                        sds_cat_printf( &s, "%4zu: OP_GATTER\n", i );
                        break;
                default:
                        sds_cat_printf( &s, "%4zu: (unknown op: %d)\n", i,
                                        (int)op );
                        break;
                }
        }
        return s;
}

struct vm *
vm_new( struct ctx *ctx )
{
        struct vm *vm = malloc( sizeof( *vm ) );
        vm->ctx       = ctx;
        vm->sp        = 0;
        return vm;
}

void
vm_free( struct vm *p )
{
        if ( p == NULL ) return;
        assert( p->sp == 0 );
        // for ( size_t i = 0; i < vm->sp; i++ ) {
        //         tsr_dec_ref( vm->stack[i] );
        // }
        free( p );
}

error_t
vm_push_tsr( struct vm *vm, struct tensor *tsr )
{
        if ( vm->sp >= VM_STACK_MAX_SIZE ) {
                EMIT_ERROR_NOTE( vm->ctx, "vm stack overflow" );
                return ERROR;
        }

        tsr_inc_ref( tsr );
        vm->stack[vm->sp++].tsr = tsr;

        return OK;
}

error_t
vm_pop_tsr( struct vm *vm, _OUT_ struct tensor **ptsr )
{
        if ( vm->sp == 0 ) {
                EMIT_ERROR_NOTE( vm->ctx, "vm stack underflow" );
                return ERROR;
        }
        *ptsr = vm->stack[--vm->sp].tsr;
        return OK;
}

size_t
vm_stack_size( struct vm *vm )
{
        return vm->sp;
}

typedef error_t ( *vm_op_fn_t )( struct ctx *ctx, struct vm *vm,
                                 struct vm_program *p, size_t pc );

static vm_op_fn_t op_fn_tbl[] = {
    [OP_GATTER] = op_gatter,
};

error_t
vm_run( struct vm *vm, struct vm_program *p )
{
        error_t err = OK;

        assert( vm == p->vm );

        size_t op_count = vec_size( p->ops );
        for ( size_t i = 0; i < op_count; i++ ) {
                enum vm_op op = p->ops[i];
                err           = op_fn_tbl[op]( vm->ctx, vm, p, i );
                if ( err != OK ) {
                        EMIT_ERROR_NOTE( vm->ctx, "unexpected error at op %d",
                                         (int)op );
                        goto cleanup;
                }
        }

cleanup:
        return err;
}
