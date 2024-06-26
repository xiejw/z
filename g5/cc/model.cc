#include "model.h"

#include <torch/script.h>
#include <torch/torch.h>

const static auto deviceCPU = torch::kCPU;

#ifdef G5_MODEL_ON_CPU
const static auto deviceForInference = torch::kCPU;
#else
const static auto deviceForInference = torch::kMPS;
#endif

const static auto tensor_opt_for_placeholder =
    torch::TensorOptions( ).dtype( torch::kFloat32 ).device( deviceCPU );

// error_t
// c4_model_predict( const color_t next_player_color, const color_t *board,
//                   const int pos_count, f32_t **prob, f32_t *value )
// {
//     error_t err = OK;
//
//     // debug only
//     if ( DEBUG2_ENABLED ) {
//         DEBUG2( ) << "[debug] calling model\n";
//         for ( int i = 0; i < pos_count; i++ ) {
//             DEBUG2( ) << board[i] << ",";
//         }
//         DEBUG2( ) << "\n";
//     }
//
//     torch::Tensor input =
//         torch::empty( { 1, 3, 15, 15 }, tensor_opt_for_placeholder );
//
//     // fill the inputs
//     //
//     // Note: this should match `lib/model/features.py`
//     // convert_inference_state_to_model_feature
//     f32_t *ptr = input.data_ptr<f32_t>( );
//
//     // channel spec
//     //
//     //   - black            0
//     //   - white            pos_count
//     //   - next play color  2 * pos_count
//     //
//     const int white_channel_offset = pos_count;
//     const int color_channel_offset = 2 * pos_count;
//
//     for ( int i = 0; i < pos_count; i++ ) {
//         color_t c = board[i];
//         if ( next_player_color == BLACK_INT )
//             ptr[i + color_channel_offset] = 1.0f;
//
//         if ( c == NA_INT ) {
//             continue;
//         } else if ( c == BLACK_INT ) {
//             ptr[i] = 1.0f;
//         } else {
//             assert( c == WHITE_INT );
//             ptr[i + white_channel_offset] = 1.0f;
//         }
//     }
//
//     DEBUG2( ) << "[debug] tensor input: " << input << "\n";
//     err = call_model( input, prob, value );
//     if ( OK != err ) {
//         std::cout << "[error] failed to get pred\n";
//         return err;
//     }
//
//     return OK;
// }

error_t
call_model( torch::jit::script::Module &module, torch::Tensor &input,
            f32_t **prob, f32_t *value )
{
    c10::InferenceMode guard;

    // Invoke model for inference.
    try {
        std::vector<torch::jit::IValue> inputs;
        inputs.push_back( input.to( deviceForInference ) );
        auto output = module.forward( inputs );

        torch::Tensor t0 = output.toTuple( )->elements( )[0].toTensor( ).cpu( );
        torch::Tensor t1 = output.toTuple( )->elements( )[1].toTensor( ).cpu( );

        DEBUG2( ) << "[debug] " << t0.numel( ) << ": " << t0 << "\n";
        DEBUG2( ) << "[debug] " << t1.numel( ) << ": " << t1 << "\n";

        memcpy( *prob, t0.data_ptr<f32_t>( ), t0.numel( ) * sizeof( f32_t ) );
        *value = t1.data_ptr<float>( )[0];
        return OK;
    } catch ( const c10::Error &e ) {
        std::cerr << "[sys] error invoking the model\n";
        std::cerr << e.msg( ) << "\n";
        return -1;
    }
}
