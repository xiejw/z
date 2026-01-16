/// === --- Configuration and Auxiliary Data Structures --------------------- ===
///
/// Hard code the number of Queues.
///
/// NUM_QUEUE =  8   Counter = 92        MEM Acc Counter = 4112
/// NUM_QUEUE = 16   Counter = 14772512  MEM Acc Counter = 2'282'380'604
///
const NUM_QUEUE: isize = 8;
//const NUM_QUEUE: isize = 16;

/// === --- Memory Access Macros -------------------------------------------- ===

static mut MEM_ACCESS_COUNTER: u64 = 0;

#[inline]
fn mem_r(x: &[usize], i: usize) -> usize {
    unsafe { MEM_ACCESS_COUNTER += 1 };
    x[i]
}

#[inline]
fn mem_w(x: &mut [usize], i: usize, v: usize) {
    unsafe { MEM_ACCESS_COUNTER += 1 };
    x[i] = v;
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

fn search() {}
