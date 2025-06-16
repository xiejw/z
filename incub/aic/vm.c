#include "vm.h"

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

/* === --- Implementation of APIs --------------------------------------- === */

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
