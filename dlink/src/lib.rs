#![allow(dead_code)]

/// A dancing link table with pre-allocated nodes.
///
/// All headers should be covered exactly once. For example, to cover all rows
/// and columns exactly once in a `3x3` board game with stones, like
/// ```
///   1 2 3
///  +-+-+-+
/// 1| | | |
///  +-+-+-+
/// 2| | | |
///  +-+-+-+
/// 3| | | |
///  +-+-+-+,
/// ```
/// we could design 6 headers as
/// ```
/// r1 r2 r3 c1 c2 c3.
/// ```
/// where `r` is row and `c` is column.
///
/// With that, if a stone is placed at row 1 (`r1`) and column 2 (`c2`), then
/// visually the 2 options will be inserted into the table like
/// ```
///             r1 r2 r3 c1 c2 c3             1 2 3
///             ^           ^                +-+-+-+
///             |           |               1| |x| |
///             v           v                +-+-+-+
/// (r1,c2)     + <-------> +               2| | | |
///                                          +-+-+-+
///                                         3| | | |
///                                          +-+-+-+
/// ```
/// Note that the 2 options are horizontally linked together so they can find
/// each other and each option is vertically linked with its covered header as
/// well.
///
/// To see how it helps, the conflicting positions `(r1,c1)` and `(r1,c2)` are
/// encoded in the table like
/// ```
///             r1 r2 r3 c1 c2 c3             1 2 3
///             ^        ^  ^                +-+-+-+
///             |        |  |               1|x|x| |
///             |        |  |                +-+-+-+
///             v        v  |               2| | | |
/// (r1,c1)     + <----> +  |                +-+-+-+
///             ^           |               3| | | |
///             |           |                +-+-+-+
///             v           v
/// (r1,c2)     + <-------> +
/// ```
/// When  `(r1,c1)` is selected  by the algorithm, header `r1` will be covered
/// and all options it vertically linked, e.g., `(r1,c2)`, are unlinked
/// temporarily. This means `(r1,c2)` will not be part of the search anymore,
/// until backtracking.
///
pub struct Table<'a, T> {
    /// All dancing link nodes (headers and options).
    nodes: Vec<Node<'a, T>>,
    /// Total count of nodes.
    total_node_count: usize,
}

/// A dancing link node for embedded purpose.
struct Node<'a, T> {
    /// Id of the node
    id: usize,
    /// Id of the left node
    id_l: usize,
    /// Id of the right node.
    id_r: usize,
    /// Id of the upper node.
    id_u: usize,
    /// Id of the down node.
    id_d: usize,
    /// Column information.
    col: VerticalCol,
    /// User defined data.
    udp: Option<&'a T>,
}

/// Records metadata information for a vertical column.
union VerticalCol {
    /// Head Id of the vertical col. Used by non-header.
    col_header_id: usize,
    /// Count of nodes in the vertical col. Used by header.
    col_count: usize,
}

impl<'a, T> Table<'a, T> {
    /// Creates a new dancing link table with exactly `total_header_count` headers
    /// and exactly `total_option_count` options. Headers will be initialized but
    /// options are not yet filled.
    pub fn new(total_header_count: usize, total_option_count: usize) -> Table<'a, T> {
        /* The first node is the entry point node. It does not contain any
         * information other than the pointers to headers. */
        let reserved_node_count = 1 + total_header_count + total_option_count;

        let mut nodes = Vec::with_capacity(reserved_node_count);

        nodes.push(Node::new(0));

        for i in 1..=total_header_count {
            nodes.push(Node::new(i));
            Self::link_lr(&mut nodes, i - 1, i);
        }

        Table {
            nodes: nodes,
            total_node_count: reserved_node_count,
        }
    }

    /// Link the node `id` into table after node `end` (horizontal double link).
    fn link_lr(nodes: &mut Vec<Node<'a, T>>, end: usize, id: usize) {
        let id_r = nodes[end].id_r;
        let p = &mut nodes[id];
        p.id_l = end;
        p.id_r = id_r;
        nodes[end].id_r = id;
        nodes[id_r].id_l = id;
    }
}

impl<'a, T> Node<'a, T> {
    /* N.B. Some optimization can be performed in future to write in-place Node.
     * Wonder whether this can be done by optimizer or this is too cheap to be
     * considered. */
    fn new(id: usize) -> Node<'a, T> {
        Node {
            id: id,
            id_l: id,
            id_r: id,
            id_u: id,
            id_d: id,
            col: VerticalCol { col_header_id: 0 },
            udp: None,
        }
    }
}

pub fn hello() {
    println!("123");
}
