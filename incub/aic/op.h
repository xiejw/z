#pragma once

#include "tensor.h"
#include "vm.h"

#include <adt/types.h>

error_t op_load_weight( struct vm_frame *frame );
error_t op_gatter( struct vm_frame *frame );
