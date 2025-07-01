#pragma once

#include "tensor.h"
#include "vm.h"

#include <adt/types.h>

ADT_NO_DISCARD error_t op_load_weight( struct vm_frame *frame );
ADT_NO_DISCARD error_t op_gatter( struct vm_frame *frame );
ADT_NO_DISCARD error_t op_matmul( struct vm_frame *frame );
ADT_NO_DISCARD error_t op_assert_eq( struct vm_frame *frame );
