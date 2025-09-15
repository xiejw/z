use rand;
use rand::Rng;
use std::any::Any;
use std::cell::RefCell;
use std::rc::Rc;

pub struct Config {
    pub in_size: usize,
    pub hidden_size: usize,
    pub out_size: usize,
}

pub struct NeuralNet {
    cfg: Config,

    // Weights
    pub weights_ih: Vec<f32>,

    // Activations
    pub inputs: Vec<f32>,

    // Grads and States
    pub grad_weights_ih: Vec<f32>,
    pub state_weights_ih: Option<Box<dyn Any>>,
}

pub trait Optimizer {
    fn init(&self, _params: &[f32]) -> Option<Box<dyn Any>>;
    fn apply(&self, _params: &[f32], _grads: &[f32], _states: &mut Option<Box<dyn Any>>);
}

impl NeuralNet {
    pub fn new(cfg: Config, opt: &mut dyn Optimizer) -> Rc<RefCell<Self>> {
        let weights_ih = vec![0f32; cfg.in_size * cfg.hidden_size];
        let grad_weights_ih = vec![0f32; cfg.in_size * cfg.hidden_size];
        let state_weights_ih = opt.init(&weights_ih);

        let inputs = vec![0f32; cfg.in_size];
        Rc::new(RefCell::new(NeuralNet {
            cfg,
            weights_ih,
            inputs,
            grad_weights_ih,
            state_weights_ih,
        }))
    }

    pub fn fill_random(&mut self) {
        let mut rng = rand::rng();
        for v in &mut self.inputs {
            *v = rng.random::<f32>() - 0.5f32;
        }
    }

    pub fn forward(&mut self, _inputs: &[f32]) {}

    pub fn zero_grad(&mut self) {
        self.grad_weights_ih.fill(0f32);
    }

    pub fn backward(&mut self, _grad_logits: &[f32]) {}
    pub fn apply_grad(&mut self, _opt: &mut dyn Optimizer) {}

    pub fn debug_dump(&self) {
        print!("values: <{}f32> ", self.cfg.in_size);
        for v in self.inputs.iter() {
            print!("{:.3} ", v);
        }
        print!("\n");
    }
}

pub struct SGDOpt {
    pub learning_rate: f32,
}

impl Optimizer for SGDOpt {
    fn init(&self, _params: &[f32]) -> Option<Box<dyn Any>> {
        None
    }
    fn apply(&self, _params: &[f32], _grads: &[f32], _states: &mut Option<Box<dyn Any>>) {}
}
