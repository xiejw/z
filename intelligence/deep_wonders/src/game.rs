use rand::Rng;

pub const NUM_WONDERS: usize = 12;
pub const NUM_SELECTED: usize = 8;
pub const BATCH_SIZE: usize = 4;
pub const NUM_OPS: usize = 1;
pub const OP_SELECT: usize = 0;
pub const NUM_ACTIONS: usize = NUM_WONDERS * NUM_OPS;

/// Snake draft order: P0, P1, P1, P0, P1, P0, P0, P1
const DRAFT_PLAYER: [i32; NUM_SELECTED] = [0, 1, 1, 0, 1, 0, 0, 1];

pub fn action_encode(card: usize, op: usize) -> usize {
    card * NUM_OPS + op
}

pub fn action_card(action: usize) -> usize {
    action / NUM_OPS
}

pub fn action_op(action: usize) -> usize {
    action % NUM_OPS
}

/// Display character for a card ID: 0-9 as '0'-'9', 10 as 'a', 11 as 'b'.
pub fn card_char(card: usize) -> char {
    assert!(card < NUM_WONDERS);
    if card < 10 {
        (b'0' + card as u8) as char
    } else {
        (b'a' + (card - 10) as u8) as char
    }
}

#[derive(Clone)]
pub struct Game {
    selected: [i32; NUM_SELECTED],
    picked: [bool; NUM_SELECTED],
    owner: [i32; NUM_SELECTED],
    turn: usize,
    done: bool,
    winner: i32, // -1 ongoing, 0/1 winner, 2 tie
}

impl Game {
    pub fn new() -> Game {
        let mut rng = rand::thread_rng();

        // Fisher-Yates shuffle of 0..NUM_WONDERS, take first 8.
        let mut deck = [0i32; NUM_WONDERS];
        for i in 0..NUM_WONDERS {
            deck[i] = i as i32;
        }
        for i in (1..NUM_WONDERS).rev() {
            let j = rng.gen_range(0..=i);
            deck.swap(i, j);
        }

        let mut selected = [0i32; NUM_SELECTED];
        let picked = [false; NUM_SELECTED];
        let owner = [-1i32; NUM_SELECTED];
        for i in 0..NUM_SELECTED {
            selected[i] = deck[i];
        }

        Game {
            selected,
            picked,
            owner,
            turn: 0,
            done: false,
            winner: -1,
        }
    }

    pub fn num_actions(&self) -> usize {
        NUM_ACTIONS
    }

    pub fn legal_actions(&self) -> Vec<usize> {
        assert!(!self.done);
        let base = if self.turn < BATCH_SIZE { 0 } else { BATCH_SIZE };
        let mut actions = Vec::new();
        for i in base..base + BATCH_SIZE {
            if !self.picked[i] {
                actions.push(action_encode(self.selected[i] as usize, OP_SELECT));
            }
        }
        actions
    }

    pub fn is_legal_action(&self, action: usize) -> bool {
        if self.done {
            return false;
        }
        if action >= NUM_ACTIONS {
            return false;
        }
        let card = action_card(action);
        let op = action_op(action);
        if op != OP_SELECT {
            return false;
        }

        let base = if self.turn < BATCH_SIZE { 0 } else { BATCH_SIZE };
        for i in base..base + BATCH_SIZE {
            if !self.picked[i] && self.selected[i] == card as i32 {
                return true;
            }
        }
        false
    }

    pub fn apply_action(&mut self, action: usize) {
        assert!(!self.done);
        assert!(self.is_legal_action(action));

        let card = action_card(action) as i32;
        let player = DRAFT_PLAYER[self.turn];

        // Find slot and mark picked.
        let base = if self.turn < BATCH_SIZE { 0 } else { BATCH_SIZE };
        for i in base..base + BATCH_SIZE {
            if !self.picked[i] && self.selected[i] == card {
                self.picked[i] = true;
                self.owner[i] = player;
                break;
            }
        }

        self.turn += 1;

        if self.turn >= NUM_SELECTED {
            self.done = true;
            // Count cards in [0, 7] per player.
            let mut score = [0i32; 2];
            for i in 0..NUM_SELECTED {
                if self.selected[i] >= 0 && self.selected[i] <= 7 {
                    score[self.owner[i] as usize] += 1;
                }
            }
            if score[0] > score[1] {
                self.winner = 0;
            } else if score[1] > score[0] {
                self.winner = 1;
            } else {
                self.winner = 2;
            }
        }
    }

    pub fn current_player(&self) -> i32 {
        if self.done {
            return -1;
        }
        DRAFT_PLAYER[self.turn]
    }

    pub fn winner(&self) -> i32 {
        self.winner
    }

    pub fn is_over(&self) -> bool {
        self.done
    }

    pub fn show(&self) {
        print!("Turn {}/{}", self.turn, NUM_SELECTED);
        if !self.done {
            print!(" | Player {} to pick", self.current_player());
        }
        println!();

        for batch in 0..2 {
            let base = batch * BATCH_SIZE;
            print!("  Batch {}:", batch);
            for i in base..base + BATCH_SIZE {
                if self.picked[i] {
                    print!(" [{}:P{}]", card_char(self.selected[i] as usize), self.owner[i]);
                } else {
                    print!(" {}", card_char(self.selected[i] as usize));
                }
            }
            println!();
        }

        if self.done {
            if self.winner == 2 {
                println!("Result: Tie");
            } else {
                println!("Result: Player {} wins", self.winner);
            }
        }
    }
}
