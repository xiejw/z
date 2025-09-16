pub mod learner;
pub mod nn;

use std::ops::DerefMut;

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

    learner::eval::run_against_random(model.borrow_mut().deref_mut(), /*play_count=*/ 1000);
    learner::rl::train_against_random(model.borrow_mut().deref_mut(), /*iter_count=*/ 15000);

    let generator = nn::NeuralNet::new(
        nn::Config {
            in_size: 32,
            hidden_size: 128,
            out_size: 16,
        },
        &mut opt,
    );
    learner::gan::train(
        generator.borrow_mut().deref_mut(),
        model.borrow_mut().deref_mut(),
    );
}
