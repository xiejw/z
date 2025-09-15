pub mod nn;

fn main() {
    let mut opt = nn::SGDOpt {
        learning_rate: 0.1f32,
    };
    let model = nn::NeuralNet::new(
        nn::Config {
            in_size: 32,
            hidden_size: 128,
            out_size: 16,
        },
        &mut opt,
    );

    model.borrow_mut().fill_random();
    model.borrow().debug_dump();
}
