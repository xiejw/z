pub mod eval {
    use crate::nn::NeuralNet;

    pub fn run_against_random(_model: &mut NeuralNet, _play_count: usize) {}
}

pub mod rl {
    use crate::nn;

    pub fn train_against_random(model: &mut nn::NeuralNet, _iter_count: usize) {
        let mut opt = nn::SGDOpt {
            learning_rate: 0.1f32,
        };
        model.init_opt(&mut opt);
    }
}

pub mod gan {
    use crate::nn::NeuralNet;
    pub fn train(_generator: &mut NeuralNet, _base_model: &mut NeuralNet) {}
}
