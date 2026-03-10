use crate::game::{Game, NUM_ACTIONS};

pub struct Nn {
    _placeholder: i32,
}

impl Nn {
    pub fn new() -> Nn {
        Nn { _placeholder: 0 }
    }

    /// Evaluate the game state. Returns uniform policy over legal actions and value=0.
    pub fn evaluate(&self, g: &Game) -> ([f32; NUM_ACTIONS], f32) {
        let _obs = g.observe(); // will feed into network; unused in stub
        let actions = g.legal_actions();
        let mut policy = [0.0f32; NUM_ACTIONS];
        let n = actions.len() as f32;
        for &a in &actions {
            policy[a.encode()] = 1.0 / n;
        }
        (policy, 0.0)
    }
}
