package main

// A dancing link table with pre-allocated nodes.
//
// All headers should be covered exactly once. For example, to cover all rows
// and columns exactly once in a `3x3` board game with stones, like
//
//	  1 2 3
//	 +-+-+-+
//	1| | | |
//	 +-+-+-+
//	2| | | |
//	 +-+-+-+
//	3| | | |
//	 +-+-+-+,
//
// we could design 6 headers as
//
//	r1 r2 r3 c1 c2 c3.
//
// where `r` is row and `c` is column.
//
// With that, if a stone is placed at row 1 (`r1`) and column 2 (`c2`), then
// visually the 2 options will be inserted into the table like
//
//	            r1 r2 r3 c1 c2 c3             1 2 3
//	            ^           ^                +-+-+-+
//	            |           |               1| |x| |
//	            v           v                +-+-+-+
//	(r1,c2)     + <-------> +               2| | | |
//	                                         +-+-+-+
//	                                        3| | | |
//	                                         +-+-+-+
//
// Note that the 2 options are horizontally linked together so they can find
// each other and each option is vertically linked with its covered header as
// well.
//
// To see how it helps, the conflicting positions `(r1,c1)` and `(r1,c2)` are
// encoded in the table like
//
//	            r1 r2 r3 c1 c2 c3             1 2 3
//	            ^        ^  ^                +-+-+-+
//	            |        |  |               1|x|x| |
//	            |        |  |                +-+-+-+
//	            v        v  |               2| | | |
//	(r1,c1)     + <----> +  |                +-+-+-+
//	            ^           |               3| | | |
//	            |           |                +-+-+-+
//	            v           v
//	(r1,c2)     + <-------> +
//
// When  `(r1,c1)` is selected  by the algorithm, header `r1` will be covered
// and all options it vertically linked, e.g., `(r1,c2)`, are unlinked
// temporarily. This means `(r1,c2)` will not be part of the search anymore,
// until backtracking.
//
// type
//
//	pub struct Table<'a, T> {
//	    // All dancing link nodes (headers and options).
//	    nodes: Vec<Node<'a, T>>,
//	    /// Total count of nodes.
//	    total_node_count: uint,
//	}
//
// A dancing link node for embedded purpose.
type Node[T any] struct {
	Id  uint // Id of the node
	IdL uint // Id of the left node
	IdR uint // Id of the right node.
	IdU uint // Id of the upper node.
	// /// Id of the down node.
	// id_d uint
	// /// Column information.
	// col VerticalCol
	// /// User defined data.
	// udp Option<&'a T>
}

// Records metadata information for a vertical column.
type VerticalCol struct {
	ColHeaderCol uint // Head Id of the vertical col. Used by non-header.
	ColNodeCount uint // Count of nodes in the vertical col. Used by header.
}

//	impl<'a, T> Table<'a, T> {
//	    /// Creates a new dancing link table with exactly `total_header_count` headers
//	    /// and exactly `total_option_count` options. Headers will be initialized but
//	    /// options are not yet filled.
//	    pub fn new(total_header_count: uint, total_option_count: uint) -> Table<'a, T> {
//	        /* The first node is the entry point node. It does not contain any
//	         * information other than the pointers to headers. */
//	        let reserved_node_count = 1 + total_header_count + total_option_count;
//
//	        let mut nodes = Vec::with_capacity(reserved_node_count);
//
//	        nodes.push(Node::new(0));
//
//	        for i in 1..=total_header_count {
//	            nodes.push(Node::new(i));
//	            Self::link_lr(&mut nodes, i - 1, i);
//	        }
//
//	        Table {
//	            nodes: nodes,
//	            total_node_count: reserved_node_count,
//	        }
//	    }
//
//	    /// Link the node `Id` into table after node `end` (horizontal double link).
//	    fn link_lr(nodes: &mut Vec<Node<'a, T>>, end: uint, id: uint) {
//	        let IdR = nodes[end].IdR;
//	        let p = &mut nodes[id];
//	        p.IdL = end;
//	        p.IdR = IdR;
//	        nodes[end].IdR = id;
//	        nodes[IdR].IdL = id;
//	    }
//	}
//
//	impl<'a, T> Node<'a, T> {
//	    /* N.B. Some optimization can be performed in future to write in-place Node.
//	     * Wonder whether this can be done by optimizer or this is too cheap to be
//	     * considered. */
//	    fn new(id: uint) -> Node<'a, T> {
//	        Node {
//	            Id: id,
//	            IdL: id,
//	            IdR: id,
//	            IdU: id,
//	            id_d: id,
//	            col: VerticalCol { ColHeaderCol: 0 },
//	            udp: None,
//	        }
//	    }
//	}
func main() {
}
