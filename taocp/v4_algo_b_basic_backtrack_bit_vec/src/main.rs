/// === --- Configuration and Auxiliary Data Structures --------------------- ===
///
/// Hard code the number of Queues.
///
/// NUM_QUEUE =  8   Counter = 92        MEM Acc Counter = 4112
/// NUM_QUEUE = 16   Counter = 14772512  MEM Acc Counter = 2'282'380'604
///
const NUM_QUEUE: usize = 8;
//const NUM_QUEUE: usize = 16;

/// === --- Memory Access Macros -------------------------------------------- ===

static mut MEM_ACCESS_COUNTER: u64 = 0;

#[inline]
fn mem_r(x: &[usize], i: usize) -> usize {
    unsafe { MEM_ACCESS_COUNTER += 1 };
    unsafe { *x.get_unchecked(i) }
    //x[i]
}

#[inline]
fn mem_w(x: &mut [usize], i: usize, v: usize) {
    unsafe { MEM_ACCESS_COUNTER += 1 };
    unsafe { *x.get_unchecked_mut(i) = v };
    //x[i] = v;
}

/// === --- Count number of solutions --------------------------------------- ===
static mut COUNTER: u64 = 0;

fn visit_solution() {
    unsafe { COUNTER += 1 };
}

/// === --- Algorithm B - Basic Backtrack - Vol 4B Page 33 ------------------
///
/// This code is actually the Algorithm B with bit vectors (in registers) as
/// auxiliary data structures.

/// Assume 1 based access. Read is straightforward. Write is using a
/// clear-then-set logic, like x = (x & ~(1u << t)) | ((v & 1u) << t);
#[inline]
fn reg_r(x: u64, t: usize) -> u64 {
    (x >> t) & 1
}

#[inline]
fn reg_w(x: &mut u64, t: usize, v: u64) {
    *x = (*x & !(1u64 << t)) | ((v & 1u64) << t)
}

fn main() {
    println!(
        "Basic Backtrack + Bit Vectors (Vol 4B, Page 32) - N Queue: N = {}",
        NUM_QUEUE
    );
    search();
    println!("Done: {}", unsafe { COUNTER });
    println!("Memory Access: {}", unsafe { MEM_ACCESS_COUNTER });
}

enum Step {
    B1,
    B2,
    B3,
    B4,
    B5,
}

fn search() {
    // Use array to hold X state.
    let mut x = [0usize; NUM_QUEUE + 1];

    // Use variable to hold A, b, C and hope compiler can put them into
    // registers.
    let mut a: u64 = 0;
    let mut b: u64 = 0;
    let mut c: u64 = 0;

    let mut t: usize = 0;
    let mut l: usize = 0;

    let mut step = Step::B1;

    loop {
        match step {
            Step::B1 => {
                // Initialize
                l = 1;
                step = Step::B2;
            }

            Step::B2 => {
                // Enter level l
                if l > NUM_QUEUE {
                    visit_solution();
                    step = Step::B5;
                } else {
                    // Scan domain now.
                    t = 1;
                    step = Step::B3;
                }
            }

            Step::B3 => {
                // Try t
                if reg_r(a, t) != 0 || reg_r(b, t + l - 1) != 0 || reg_r(c, t + NUM_QUEUE - l) != 0
                {
                    step = Step::B4;
                } else {
                    reg_w(&mut a, t, 1);
                    reg_w(&mut b, t + l - 1, 1);
                    reg_w(&mut c, t + NUM_QUEUE - l, 1);
                    mem_w(&mut x, l, t);
                    l += 1;
                    step = Step::B2;
                }
            }

            Step::B4 => {
                // Try next t
                if t < NUM_QUEUE {
                    t += 1;
                    step = Step::B3;
                } else {
                    step = Step::B5;
                }
            }

            Step::B5 => {
                // Backtrack
                l -= 1;
                if l > 0 {
                    t = mem_r(&x, l);
                    reg_w(&mut c, t + NUM_QUEUE - l, 0);
                    reg_w(&mut b, t + l - 1, 0);
                    reg_w(&mut a, t, 0);
                    step = Step::B4;
                } else {
                    break;
                }
            }
        };
    }
}
