#ifndef AIC_VM_H_
#define AIC_VM_H_

#include <adt/sds.h>
#include <adt/types.h>

#include "ctx.h"
#include "tensor.h"

/* ------- Forward Declaratons ---------------------------------------------- */

struct vm;
struct vm_value;
struct vm_program;

/* ------- VM --------------------------------------------------------------- */

/* Before closing vm, all tensors on stack must be popped up. */
struct vm *vm_new( struct ctx *ctx );
void       vm_free( struct vm *p );

/* Once push, vm owns the tensor by increasing the ref count. Once pop, vm will
 * release (by moving) the tensor to caller.*/
error_t vm_push_tsr( struct vm *, struct tensor * );
error_t vm_pop_tsr( struct vm *, _OUT_ struct tensor ** );

size_t                 vm_stack_size( struct vm * );
ADT_NO_DISCARD error_t vm_run( struct vm *, const struct vm_program *p );

/* ------- Program ---------------------------------------------------------- */

struct vm_program *vm_program_new( struct vm * );
void               vm_program_free( struct vm_program * );

/* Return the byte code. */
vec_t( byte ) vm_program_get_bytecode( const struct vm_program *p );

/* Dump the program text form to a new sds. Caller owns it. */
sds_t vm_program_dump( const struct vm_program *p );

enum vm_op {
        /* Load the weight whose name address is next 8 bytes and tensor
         * address is 8 bytes after in the program.
         *
         * Push it to stack.
         *
         * To push op, use
         *
         *   vm_program_push_op(OP_LOAD_WEIGHT, "embedding", tsr_embedding_ptr)
         */
        OP_LOAD_WEIGHT,

        /* Pop up the weight first and then the index tensor.
         * Push the result tensor
         */
        OP_GATTER,

        /* Pop up the column major weight first and then the input tensor.
         * Push the result tensor
         */
        OP_MATMUL,

        /* This is for debugging only.
         * - Pop up two tensors and check the values are identical.
         * - Push nothing
         */
        OP_ASSERT_EQ,
};

ADT_NO_DISCARD error_t vm_program_push_op( struct vm_program *p, enum vm_op op,
                                           ... );

/* ------- Op Handler ------------------------------------------------------- */

struct vm_frame {
        struct ctx              *ctx;
        struct vm               *vm;
        const struct vm_program *program;
        size_t                  *ppc; /* point to pc. ok to change .*/
};

typedef ADT_NO_DISCARD error_t ( *vm_op_fn_t )( struct vm_frame *frame );

#endif  // AIC_VM_H_
