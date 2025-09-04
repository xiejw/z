// vim: ft=cpp
//
// === NeuralNetwork ------------------------------------------------------- ===

namespace eos::gan {
// Neural Network Paramters.
constexpr int kNNInputSize  = kGameStateCount * 2;
constexpr int kNNHiddenSize = 100;
constexpr int kNNOutputSize = kGameStateCount;
// constexpr f32 kLearningRate = 0.1f;

// Two-layer Neural network.
template <std::size_t IN = kNNInputSize, std::size_t OUT = kNNOutputSize,
          std::size_t HIDDEN = kNNHiddenSize>
class NeuralNetwork {
      public:
        // Weights and biases.
        float weights_ih[IN * HIDDEN];
        float weights_ho[HIDDEN * OUT];
        float biases_h[HIDDEN];
        float biases_o[OUT];

        // Activations are part of the structure itself for simplicity.
        float inputs[IN];
        float hidden[HIDDEN];
        float raw_logits[OUT];  // Outputs before softmax().
        float outputs[OUT];     // Outputs after softmax().
                                //
      public:
        void init( );
        void forward( f32 * );
        void backward( f32 *target_probs, f32 lr, f32 reward_scaling );
};

namespace {
/* Initialize a neural network with random weights, we should
 * use something like He weights since we use RELU, but we don't
 * care as this is a trivial example. */
inline f32
rand_f32( )
{
        return ( ( (float)rand( ) / (float)RAND_MAX ) - 0.5f );
}

/* ReLU activation function */
float
relu( float x )
{
        return x > 0 ? x : 0;
}

/* Derivative of ReLU activation function */
f32
relu_derivative( f32 x )
{
        return x > 0 ? 1.0f : 0.0f;
}

/* Apply softmax activation function to an array input, and
 * set the result into output. */
void
softmax( float *input, float *output, int size )
{
        /* Find maximum value then subtact it to avoid
         * numerical stability issues with exp(). */
        float max_val = input[0];
        for ( int i = 1; i < size; i++ ) {
                if ( input[i] > max_val ) {
                        max_val = input[i];
                }
        }

        // Calculate exp(x_i - max) for each element and sum.
        float sum = 0.0f;
        for ( int i = 0; i < size; i++ ) {
                output[i] = expf( input[i] - max_val );
                sum += output[i];
        }

        // Normalize to get probabilities.
        if ( sum > 0 ) {
                for ( int i = 0; i < size; i++ ) {
                        output[i] /= sum;
                }
        } else {
                /* Fallback in case of numerical issues, just provide
                 * a uniform distribution. */
                for ( int i = 0; i < size; i++ ) {
                        output[i] = 1.0f / (float)size;
                }
        }
}

}  // namespace

template <std::size_t IN, std::size_t OUT, std::size_t HIDDEN>
void
NeuralNetwork<IN, OUT, HIDDEN>::init( )
{
        // Initialize weights with random values between -0.5 and 0.5
        for ( std::size_t i = 0; i < IN * HIDDEN; i++ )
                this->weights_ih[i] = rand_f32( );

        for ( std::size_t i = 0; i < HIDDEN * OUT; i++ )
                this->weights_ho[i] = rand_f32( );

        for ( std::size_t i = 0; i < HIDDEN; i++ )
                this->biases_h[i] = rand_f32( );

        for ( std::size_t i = 0; i < OUT; i++ ) this->biases_o[i] = rand_f32( );
}

/* Neural network foward pass (inference).
 *
 * NOTE: The activations are stored so we can also do backpropagation later.
 */
template <std::size_t IN, std::size_t OUT, std::size_t HIDDEN>
void
NeuralNetwork<IN, OUT, HIDDEN>::forward( f32 *inputs )
{
        // Copy inputs.
        memcpy( this->inputs, inputs, IN * sizeof( float ) );

        // Input to hidden layer.
        //
        // Shapes
        // - matmul (inputs / 1 x IN, weights_ih / IN x HIDDEN)
        // - biases_h / HIDDEN
        // - hidden / HIDDEN
        //
        for ( std::size_t i = 0; i < HIDDEN; i++ ) {
                float sum = this->biases_h[i];
                for ( std::size_t j = 0; j < IN; j++ ) {
                        sum += inputs[j] * this->weights_ih[j * HIDDEN + i];
                }
                this->hidden[i] = relu( sum );
        }

        // Hidden to output (raw logits).
        //
        // Shapes
        // - matmul (hidden / 1 x HIDDEN, weights_ho / HIDDEN x OUT )
        // - biases_o / OUT
        // - raw_logits / OUT
        //
        for ( std::size_t i = 0; i < OUT; i++ ) {
                this->raw_logits[i] = this->biases_o[i];
                for ( std::size_t j = 0; j < HIDDEN; j++ ) {
                        this->raw_logits[i] +=
                            this->hidden[j] * this->weights_ho[j * OUT + i];
                }
        }

        // Apply softmax to get the final probabilities.
        //
        // Shapes
        // - raw_logits / OUT
        // - outputs / OUT
        //
        softmax( /*input=*/this->raw_logits, /*output=*/this->outputs, OUT );
}

/* Backpropagation function.
 *
 * The only difference here from vanilla backprop is that we have
 * a 'reward_scaling' argument that makes the output error more/less
 * dramatic, so that we can adjust the weights proportionally to the
 * reward we want to provide.
 */
template <std::size_t IN, std::size_t OUT, std::size_t HIDDEN>
void
NeuralNetwork<IN, OUT, HIDDEN>::backward( f32 *target_probs, f32 lr,
                                          f32 reward_scaling )
{
        f32 raw_logits_deltas[OUT];
        f32 hidden_deltas[HIDDEN];

        /* === --- STEP 1: Compute Deltas ------------------------------- === */

        /* Calculate output layer deltas:
         *
         * Note what's going on here: we are technically using softmax
         * from raw_logits to output in forward function which uses cross
         * entropy as loss.
         *
         * Still calculating the deltas as:
         *
         *      output[i] - target[i]
         *
         * NOTE: This is gradients w.r.t. to raw_logits not outputs (even
         * though it uses outputs only). Check the gradients for cross entropy
         * for details.
         *
         * Shapes
         * - outputs / OUT
         * - target_probs / OUT
         * - raw_logits_deltas / OUT
         *
         */
        for ( std::size_t i = 0; i < OUT; i++ ) {
                raw_logits_deltas[i] = ( this->outputs[i] - target_probs[i] ) *
                                       fabsf( reward_scaling );
        }

        // Backpropagate error to hidden layer.
        //
        // Shapes
        // - matmul(
        //     raw_logits_deltas /  OUT,
        //     trans( weights_ho / HIDDEN x OUT )
        //   )
        // - hidden / 1 x HIDDEN
        // - hidden_deltas / 1 x HIDDEN
        //
        for ( std::size_t i = 0; i < HIDDEN; i++ ) {
                f32 error = 0;
                for ( std::size_t j = 0; j < OUT; j++ ) {
                        error += raw_logits_deltas[j] *
                                 this->weights_ho[i * OUT + j];
                }
                hidden_deltas[i] = error * relu_derivative( this->hidden[i] );
        }

        /* === --- STEP 2: Weights Updating -----------------------------=== */

        // Output layer weights and biases.
        //
        // Shapes
        // - hidden / 1 x HIDDEN
        // - weights_ho / HIDDEN x OUT
        // - raw_logits_deltas / 1 x OUT
        //
        // Forward
        // - hidden ** weights_ho = raw_logits
        // Backward
        // - d_weights_ho = matmul(trans(hidden), d_raw_logits)
        //
        for ( std::size_t i = 0; i < HIDDEN; i++ ) {
                for ( std::size_t j = 0; j < OUT; j++ ) {
                        this->weights_ho[i * OUT + j] -=
                            lr * raw_logits_deltas[j] * this->hidden[i];
                }
        }
        // Shapes
        // - biases_o / 1 x OUT
        // - raw_logits_deltas / 1 x OUT
        //
        for ( std::size_t j = 0; j < OUT; j++ ) {
                this->biases_o[j] -= lr * raw_logits_deltas[j];
        }

        // Hidden layer weights and biases.
        //
        // Shapes
        // - inputs 1 x IN
        // - weights_ih IN x HIDDEN
        // - hidden_deltas 1 x HIDDEN
        //
        // Forward
        // - inputs ** weights_ih = hidden
        // Backward
        // - d_weights_ih = trans(d_inputs) ** hidden
        //
        for ( std::size_t i = 0; i < IN; i++ ) {
                for ( std::size_t j = 0; j < HIDDEN; j++ ) {
                        this->weights_ih[i * HIDDEN + j] -=
                            lr * hidden_deltas[j] * this->inputs[i];
                }
        }
        // Shapes
        // - biases_h / 1 x HIDDEN
        // - hidden_deltas / 1 x HIDDEN
        //
        for ( std::size_t j = 0; j < HIDDEN; j++ ) {
                this->biases_h[j] -= lr * hidden_deltas[j];
        }
}

}  // namespace eos::gan
