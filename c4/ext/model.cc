#include "model.h"

#include <torch/script.h>
#include <torch/torch.h>

namespace c4 {

/* === --- Tensor device and dtype ----------------------------------------- ===
 */
static const auto deviceCPU = torch::kCPU;

static const auto tensor_opt_for_placeholder =
    torch::TensorOptions( ).dtype( torch::kFloat32 ).device( deviceCPU );

#ifdef MODEL_ON_CPU
static const auto  deviceForInference = torch::kCPU;
static const char *deviceForInfStr    = "CPU";
#else
static const auto  deviceForInference = torch::kMPS;
static const char *deviceForInfStr    = "MPS";
#endif

/* === --- Static allocated module and lazy loading info ------------------- ===
 */
static bool                       module_loaded = false;
static torch::jit::script::Module module;

/* === --- Helper methods prototypes --------------------------------------- ===
 */
namespace {
error_t
call_model( torch::Tensor &input, f32_t **_C4_Out p_prob,
            f32_t *_C4_Out p_value )
{
        /* Inference mode guard. RAII style. */
        c10::InferenceMode guard;

        /* Lazy loading model. */
        if ( !module_loaded ) {
                error_t err = model_init( );
                if ( OK != err ) return err;
        };

        /* Model Inference. */
        try {
                std::vector<torch::jit::IValue> inputs;
                inputs.push_back( input.to( deviceForInference ) );
                auto output = module.forward( inputs );

                /* Obtain the outputs to fill the results. */
                torch::Tensor t0 =
                    output.toTuple( )->elements( )[0].toTensor( ).cpu( );
                torch::Tensor t1 =
                    output.toTuple( )->elements( )[1].toTensor( ).cpu( );

                DEBUG2( ) << t0.numel( ) << ": " << t0 << "\n";
                DEBUG2( ) << t1.numel( ) << ": " << t1 << "\n";

                memcpy( *p_prob, t0.data_ptr<f32_t>( ),
                        size_t( t0.numel( ) ) * sizeof( f32_t ) );
                *p_value = t1.data_ptr<float>( )[0];
                return OK;
        } catch ( const c10::Error &e ) {
                ERROR( ) << "error invoking the model\n";
                ERROR( ) << e.msg( ) << "\n";
                return ERR;
        }
}
}  // namespace

/* === --- Helper methods prototypes --------------------------------------- ===
 */

error_t
model_init( )
{
        if ( module_loaded ) return OK;
        try {
                INFO( ) << "load resent model from " C4_FILE_PATH "\n";
                module = torch::jit::load( C4_FILE_PATH );
                INFO( ) << "move resent model to " << deviceForInfStr << "\n";
                module.to( deviceForInference );
                module.eval( );
                module_loaded = true;
                return OK;
        } catch ( const c10::Error &e ) {
                ERROR( ) << "error loading the model\n";
                ERROR( ) << e.msg( ) << "\n";
                return ERR;
        }
}

void
model_deinit( )
{
        DEBUG2( ) << "deinit model module\n";
        module.~Module( );
        module_loaded = false;
        DEBUG2( ) << "deinit model module done\n";
}

error_t
model_predict( const color_t next_player_color, const color_t *board,
               const int board_size, f32_t **_C4_Out prob,
               f32_t *_C4_Out value )
{
        error_t err = OK;

        if ( DEBUG2_ENABLED ) {
                DEBUG2( ) << "[debug] calling model\n";
                for ( int i = 0; i < board_size; i++ ) {
                        DEBUG2( ) << board[i] << ",";
                }
                DEBUG2( ) << "\n";
        }

        torch::Tensor feature_input =
            torch::empty( { 1, CHANNEL_COUNT, ROW_COUNT, COL_COUNT },
                          tensor_opt_for_placeholder );

        /* Fill the inputs
         *
         * Note: this should match the function
         * convert_inference_state_to_model_feature in file
         * `lib/model/features.py`
         */

        /* For position with black color, fill the first channel (offset 0),
         * otherwise, fill the second channel (offset is board_size) */
        const int white_channel_offset = board_size;
        /* Fills this channel with all 1 if the next_player_color is black. */
        const int color_channel_offset = 2 * board_size;

        f32_t *ptr = feature_input.data_ptr<f32_t>( );

        for ( int i = 0; i < board_size; i++ ) {
                color_t c = board[i];
                if ( next_player_color == BLACK_INT )
                        ptr[i + color_channel_offset] = 1.0f;

                if ( c == NA_INT ) {
                        continue;
                } else if ( c == BLACK_INT ) {
                        ptr[i] = 1.0f;
                } else {
                        assert( c == WHITE_INT );
                        ptr[i + white_channel_offset] = 1.0f;
                }
        }

        DEBUG2( ) << "tensor input: " << feature_input << "\n";
        err = call_model( feature_input, prob, value );
        if ( OK != err ) {
                ERROR( ) << "failed to get pred\n ";
                return err;
        }

        return OK;
}

}  // namespace c4
