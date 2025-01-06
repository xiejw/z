#include "model.h"

#include "ctx.h"
#include "model.h"
#include "policy.h"

int
main( )
{
        int boards[BOARD_SIZE] = { };
        int predict_col = c4::policy_call( std::span{ boards }, BLACK_INT );
        DEBUG( ) << "predict col is " << predict_col << "\n";
        return 0;
}
