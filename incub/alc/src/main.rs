pub mod alc {
    pub struct Shape {
        pub rank: usize,
        pub dims: [usize; 4],
    }

    pub enum Value {
        Array(Vec<f32>),
        Reference,
    }

    pub struct Tensor {
        pub shape: Shape,
        pub value: Value,
    }

    pub enum Op<'a> {
        Add(&'a Op<'a>, &'a Op<'a>),
        Mul(&'a Op<'a>, &'a Op<'a>),
        Static(&'a Tensor),
        Placeholder(String, Shape),
    }

    pub struct Program<'a> {
        pub tensors: Vec<Tensor>,
        pub ops: Vec<Op<'a>>,
    }

    impl Shape {
        pub fn r1(dim: usize) -> Shape {
            Shape {
                rank: 1,
                dims: [dim, 0, 0, 0],
            }
        }
    }

    impl Tensor {
        pub fn new_scalar(v: f32) -> Tensor {
            Tensor {
                shape: Shape::r1(1),
                value: Value::Array(vec![v]),
            }
        }
    }
}

use crate::alc::*;

fn main() {
    let x = Op::Placeholder("x".to_owned(), Shape::r1(1));
    let y = Op::Placeholder("y".to_owned(), Shape::r1(1));
    let arr = Tensor::new_scalar(1.0f32);
    let z = Op::Static(&arr);
    let o = Op::Add(&x, &y);
    let _o = Op::Mul(&o, &z);
}
