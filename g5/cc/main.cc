#include "model.h"

#include <torch/script.h>
#include <torch/torch.h>

const static auto deviceCPU = torch::kCPU;
const static auto tensor_opt_for_placeholder =
    torch::TensorOptions( ).dtype( torch::kFloat32 ).device( deviceCPU );

#ifdef G5_MODEL_ON_CPU
const static auto deviceForInference = torch::kCPU;
#else
const static auto deviceForInference = torch::kMPS;
#endif

int
main( )
{
    torch::jit::script::Module module;
    try {
        std::cout << "[sys] load model (once) from " << G5_FILE_PATH << "\n";
        module = torch::jit::load( G5_FILE_PATH );
        module.to( deviceForInference );
        module.eval( );
    } catch ( const c10::Error &e ) {
        std::cerr << "[sys] error loading the model\n";
        std::cerr << e.msg( ) << "\n";
        return -1;
    }

    torch::Tensor input =
        torch::empty( { 1, 3, 15, 15 }, tensor_opt_for_placeholder );

    // fill the inputs
    //
    // Note: this should match `lib/model/features.py`
    // convert_inference_state_to_model_feature
    f32_t *ptr = input.data_ptr<f32_t>( );

    for ( size_t i = 0; i < 3 * 15 * 15; i++ ) {
        ptr[i] = 1.0f;
    }

    f32_t  probs[BOARD_SIZE];
    f32_t  value;
    f32_t *probs_ptr = probs;

    error_t err = call_model( module, input, &probs_ptr, &value );

    DEBUG( ) << "[debug] error code: err " << err << "\n";

    DEBUG( ) << "[debug] probs: ";
    size_t c = 0;
    for ( size_t i = 0; i < BOARD_SIZE; i++ ) {
        DEBUG( ) << probs[i] << ", ";
        c++;
        if ( c >= 9 ) {
            c = 0;
            DEBUG( ) << "\n";
        }
    }

    DEBUG( ) << "[debug] winning probabilty from model: " << value << "\n";
    return 0;
}
