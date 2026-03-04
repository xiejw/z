use rand::Rng;

use crate::game::{Game, NUM_ACTIONS};
use crate::nn::Nn;

pub struct MctsNode {
    pub total_count: i32,
    pub visit_count: [i32; NUM_ACTIONS],
    pub w: [f32; NUM_ACTIONS],
    pub p: [f32; NUM_ACTIONS],
}

impl MctsNode {
    pub fn new(g: &Game, nn: &Nn) -> MctsNode {
        let (policy, _value) = nn.evaluate(g);
        MctsNode {
            total_count: 0,
            visit_count: [0; NUM_ACTIONS],
            w: [0.0; NUM_ACTIONS],
            p: policy,
        }
    }
}

/// Run MCTS search and return the best action.
/// Currently returns a random legal action (stub).
pub fn mcts_search(g: &Game, _nn: &Nn, _iterations: i32) -> usize {
    let actions = g.legal_actions();
    let mut rng = rand::thread_rng();
    actions[rng.gen_range(0..actions.len())]
}
