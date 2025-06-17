#pragma once

#include "tensor.h"
#include "vm.h"

#include <adt/types.h>

error_t op_gatter( struct ctx *ctx, struct vm *vm, struct vm_program *p,
                   size_t pc );
