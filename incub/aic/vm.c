#include "vm.h"

#include <stdarg.h>

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

/* === --- Help Methods ------------------------------------------------- === */

static void
push_u64( vec_t( byte ) * pops, u64 v )
{
        size_t size = vec_size( *pops );
        vec_reserve( pops, size + 8 );
        byte *ptr = ( *pops ) + size;
        ptr[0]    = (byte)( v );
        ptr[1]    = (byte)( v >> 8 );
        ptr[2]    = (byte)( v >> 16 );
        ptr[3]    = (byte)( v >> 24 );
        ptr[4]    = (byte)( v >> 32 );
        ptr[5]    = (byte)( v >> 40 );
        ptr[6]    = (byte)( v >> 48 );
        ptr[7]    = (byte)( v >> 56 );
        vec_set_size( *pops, size + 8 );
}

static u64
load_u64( byte *ptr )
{
        return (u64)( ptr[0] ) | ( (u64)( ptr[1] ) << 8 ) |
               ( (u64)( ptr[2] ) << 16 ) | ( (u64)( ptr[3] ) << 24 ) |
               ( (u64)( ptr[4] ) << 32 ) | ( (u64)( ptr[5] ) << 40 ) |
               ( (u64)( ptr[6] ) << 48 ) | ( (u64)( ptr[7] ) << 56 );
}

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
vm_program_push_op( struct vm_program *p, enum vm_op op, ... )
{
        assert( (int)( op ) <= 256 );
        vec_push( &p->ops, (byte)op );

        va_list args;
        switch ( op ) {
        case OP_LOAD_WEIGHT:
                va_start( args, op );
                const char *name = va_arg( args, const char * );
                push_u64( &p->ops, (u64)name );
                struct tensor *w = va_arg( args, struct tensor * );
                push_u64( &p->ops, (u64)w );
                va_end( args );
        default:
                break;
        }
        return OK;
}

sds_t
vm_program_dump( const struct vm_program *p )
{
        sds_t       s        = sds_empty( );
        size_t      op_count = vec_size( p->ops );
        const char *weight_name;
        if ( op_count == 0 ) {
                sds_cat_printf( &s, "(empty program)" );
                return s;
        }

        for ( size_t i = 0; i < op_count; i++ ) {
                enum vm_op op = (byte)p->ops[i];
                switch ( op ) {
                case OP_LOAD_WEIGHT:
                        weight_name = (const char *)load_u64( p->ops + i + 1 );
                        sds_cat_printf( &s, "%4zu: OP_LOAD_WEIGHT `%s`\n", i,
                                        weight_name );
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

static vm_op_fn_t op_fn_tbl[] = {
    [OP_LOAD_WEIGHT] = op_load_weight,
    [OP_GATTER]      = op_gatter,
};

error_t
vm_run( struct vm *vm, const struct vm_program *p )
{
        error_t err = OK;
        size_t  pc  = 0;

        struct vm_frame frame = {
            .ctx     = vm->ctx,
            .vm      = vm,
            .program = p,
            .ppc     = &pc,
        };

        assert( vm == p->vm );

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
