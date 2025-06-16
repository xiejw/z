#ifndef AIC_VM_H_
#define AIC_VM_H_

#include <adt/sds.h>
#include <adt/types.h>

#include "ctx.h"
#include "tensor.h"

// Forward declaration.
struct vm;
struct vm_value;
struct vm_program;

/* Before closing vm, all tensors on stack must be popped. */
struct vm *vm_new( struct ctx *ctx );
void       vm_free( struct vm *p );

/* Once push, vm owns the tensor by increasing the ref count. Once pop, vm will
 * release (by moving) the tensor to caller.*/
error_t vm_push_tsr( struct vm *, struct tensor * );
error_t vm_pop_tsr( struct vm *, _OUT_ struct tensor ** );
size_t  vm_stack_size( struct vm * );

error_t vm_run( struct vm *, struct vm_program *p );

enum vm_op {
        OP_GATTER,
};

struct vm_program *vm_program_new( struct vm * );
void               vm_program_free( struct vm_program * );
error_t            vm_program_push_op( struct vm_program *p, enum vm_op op );

/* Dump the program text form to a new sds. Caller owns it. */
sds_t vm_program_dump( struct vm_program *p );

#endif  // AIC_VM_H_
