package main

import "fmt"

const (
	// === --- Configuration and Auxiliary Data Structures --------------------- ===
	//
	// Hard code the number of Queues.
	//
	// kNum =  8   Counter = 92        MEM Acc Counter = 4112
	// kNum = 16   Counter = 14772512  MEM Acc Counter = 2'282'380'604
	//
	// constexpr int kNum = 8;
	// kNum = 8
	kNum = 16
)

var (
	// Static allocating the data structures. All arrays start from base 1.
	X = [kNum + 1]int{0}
)

//// === --- Memory Access Macros -------------------------------------------- ===

var mem_access_counter uint64 = 0

// // #define MEM_R( x, i )    ( ( x )[( i )] )
// // #define MEM_W( x, i, v ) ( ( x )[( i )] = ( v ) )
func MemR(x []int, i int) int {
	mem_access_counter++
	return x[i]
}

func MemW(x []int, i int, v int) {
	mem_access_counter++
	x[i] = v
}

// === --- Count number of solutions --------------------------------------- ===
var counter uint64

func VisitSolution() {
	counter++
}

// === --- Algorithm B - Basic Backtrack - Vol 4B Page 33 ------------------
//
// This code is actually the Algorithm B with bit vectors (in registers) as
// auxiliary data structures.

// Assume 1 based access. Read is straightforward. Write is using a
// clear-then-set logic, like x = (x & ~(1u << t)) | ((v & 1u) << t);
func RegR(x uint64, t int) uint64 {
	return (x >> t) & 1
}

func RegW(x *uint64, t int, v uint64) {
	*x = (*x & ^(uint64(1) << t)) | ((v & uint64(1)) << t)
}

func main() {
	fmt.Printf("Basic Backtrack + Bit Vectors (Vol 4B, Page 32) - N Queue: N = %v\n", kNum)
	Search()
	fmt.Printf("Done: %v\n", counter)
	fmt.Printf("Memory Access: %v\n", mem_access_counter)
}

func Search() {
	// Use variable to hold A, B, C and hope compiler can put them into
	// registers.
	var A uint64 = 0
	var B uint64 = 0
	var C uint64 = 0

	var t int
	var l int

	goto B1

B1: // Initialize
	l = 1

	// Fallthrough

B2: // Enter level l
	if l > kNum {
		VisitSolution()
		goto B5
	}

	// Scan domain now.
	t = 1

	// Fallthrough

B3: // Try t
	if RegR(A, t) != 0 || RegR(B, t+l-1) != 0 ||
		RegR(C, t-l+kNum) != 0 {
		goto B4
	}

	RegW(&A, t, 1)
	RegW(&B, t+l-1, 1)
	RegW(&C, t-l+kNum, 1)
	MemW(X[:], l, t)
	l++
	goto B2

B4: // Try next t
	if t < kNum {
		t++
		goto B3
	}

	// Fallthrough

B5: // Backtrack
	l--
	if l > 0 {
		t = MemR(X[:], l)
		RegW(&C, t-l+kNum, 0)
		RegW(&B, t+l-1, 0)
		RegW(&A, t, 0)
		goto B4
	}

	// Fallthrough

	// Exit
	return
}

//
//}  // namespace
//
//int
//main( )
//{
//        INFO( "Basic Backtrack + Bit Vectors (Vol 4B, Page 32) - N Queue: N = %d", kNum );
//        Search( );
//        INFO( "Done: %" PRIu64, counter );
//        INFO( "Memory Access: %" PRIu64, mem_access_counter );
//}
