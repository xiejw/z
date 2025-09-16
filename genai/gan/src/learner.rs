pub mod eval {
    use crate::nn::NeuralNet;

    pub fn run_against_random(_model: &mut NeuralNet, _play_count: usize) {}
}

pub mod rl {
    use crate::nn::NeuralNet;

    pub fn train_against_random(_model: &mut NeuralNet, _iter_count: usize) {}
}

pub mod gan {
    use crate::nn::NeuralNet;
    pub fn train(_generator: &mut NeuralNet, _base_model: &mut NeuralNet) {}
}
