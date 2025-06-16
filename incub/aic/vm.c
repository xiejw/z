#include "vm.h"

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
