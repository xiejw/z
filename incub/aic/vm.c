#include "vm.h"

#include <stdarg.h>

#include "op.h"
#include "util.h"

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
        vec_t( byte ) ops;
};

/* === --- Implementation of APIs --------------------------------------- === */

/* ------- Program ---------------------------------------------------------- */
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
vm_program_push_op( struct vm_program *p, enum vm_op op, ... )
{
        assert( (int)( op ) <= 256 );
        vec_push( &p->ops, (byte)op );

        va_list args;
        switch ( op ) {
        case OP_LOAD_WEIGHT:
                va_start( args, op );
                const char *name = va_arg( args, const char * );
                bytecode_store_u64( &p->ops, (u64)name );
                struct tensor *w = va_arg( args, struct tensor * );
                bytecode_store_u64( &p->ops, (u64)w );
                va_end( args );
        default:
                break;
        }
        return OK;
}

vec_t( byte ) vm_program_get_bytecode( const struct vm_program *p )
{
        return p->ops;
}

sds_t
vm_program_dump( const struct vm_program *p )
{
        sds_t          s        = sds_empty( );
        size_t         op_count = vec_size( p->ops );
        const char    *weight_name;
        struct tensor *tsr;

        if ( op_count == 0 ) {
                sds_cat_printf( &s, "(empty program)" );
                return s;
        }

        for ( size_t i = 0; i < op_count; i++ ) {
                enum vm_op op = (byte)p->ops[i];
                switch ( op ) {
                case OP_LOAD_WEIGHT:
                        weight_name =
                            (const char *)bytecode_load_u64( p->ops + i + 1 );
                        tsr = (struct tensor *)bytecode_load_u64( p->ops + i +
                                                                  9 );
                        sds_cat_printf( &s, "%4zu: OP_LOAD_WEIGHT \"%s\": %p\n",
                                        i, weight_name, tsr );
                        i += 16; /* skip the weight_name and tensor ptr. */
                        break;
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

/* ------- VM --------------------------------------------------------------- */

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

static vm_op_fn_t op_fn_tbl[] = {
    [OP_LOAD_WEIGHT] = op_load_weight,
    [OP_GATTER]      = op_gatter,
    [OP_ASSERT_EQ]   = op_assert_eq,
};

error_t
vm_run( struct vm *vm, const struct vm_program *p )
{
        error_t err = OK;
        size_t  pc  = 0;

        assert( vm == p->vm );

        struct vm_frame frame = {
            .ctx     = vm->ctx,
            .vm      = vm,
            .program = p,
            .ppc     = &pc,
        };

        const size_t op_count = vec_size( p->ops );
        for ( ; pc < op_count; pc++ ) {
                enum vm_op op = p->ops[pc];
                assert( op_fn_tbl[op] != NULL );

                err = op_fn_tbl[op]( &frame );
                if ( err != OK ) {
                        EMIT_ERROR_NOTE( vm->ctx, "unexpected error at op %d",
                                         (int)op );
                        goto cleanup;
                }
        }

cleanup:
        return err;
}
