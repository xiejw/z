use rand::Rng;

pub const NUM_WONDERS: usize = 12; // Total wonder cards in the pool
pub const NUM_SELECTED: usize = 8; // Wonder cards drawn per game (split into 2 batches)
pub const BATCH_SIZE: usize = 4; // Wonder cards revealed per batch; players draft from the active batch only
pub const NUM_OPS: usize = 1; // Operations per card (currently 1: select)
pub const OP_SELECT: usize = 0; // The only op: claim this card
pub const NUM_ACTIONS: usize = NUM_WONDERS * NUM_OPS; // Total action-space size = NUM_WONDERS × NUM_OPS

// === --- Wonder Cards ---------------------------------------------------- ===
//

/// Snake draft order: P0, P1, P1, P0, P1, P0, P0, P1
const DRAFT_PLAYER: [i32; NUM_SELECTED] = [0, 1, 1, 0, 1, 0, 0, 1];

// === --- Card and Action ------------------------------------------------- ===
//

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

// === --- Game ------------------------------------------------------------ ===
//

#[derive(Clone)]
pub struct Game {
    /// The 8 wonder card IDs drawn at random from the pool for this game;
    /// indices [0,3] = batch 0, [4,7] = batch 1.
    wonder_cards: [i32; NUM_SELECTED],
    /// Player ID (0 or 1) who claimed each slot, -1 if unclaimed.
    /// Parallel to `wonder_cards`.
    wonder_cards_owner: [i32; NUM_SELECTED],
    /// Number of picks completed (0 = start, NUM_SELECTED = game over);
    /// indexes into DRAFT_PLAYER to find the current player.
    turn: usize,
    /// (player, action) recorded each turn.
    history: Vec<(i32, usize)>,
    /// True once all NUM_SELECTED picks have been made.
    done: bool,
    /// Result: -1 ongoing, 0 or 1 for the winning player, 2 for a tie.
    winner: i32,
}

impl Game {
    pub fn new() -> Game {
        let mut rng = rand::thread_rng();

        // Fisher-Yates shuffle of 0..NUM_WONDERS, take first NUM_SELECTED.
        let mut deck = [0i32; NUM_WONDERS];
        for i in 0..NUM_WONDERS {
            deck[i] = i as i32;
        }
        for i in (1..NUM_WONDERS).rev() {
            let j = rng.gen_range(0..=i);
            deck.swap(i, j);
        }

        let mut wonder_cards = [0i32; NUM_SELECTED];
        let wonder_cards_owner = [-1i32; NUM_SELECTED];
        for i in 0..NUM_SELECTED {
            wonder_cards[i] = deck[i];
        }

        Game {
            wonder_cards,
            wonder_cards_owner,
            history: Vec::new(),
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
        let base = if self.turn < BATCH_SIZE {
            0
        } else {
            BATCH_SIZE
        };
        let mut actions = Vec::new();
        for i in base..base + BATCH_SIZE {
            if self.wonder_cards_owner[i] == -1 {
                actions.push(action_encode(self.wonder_cards[i] as usize, OP_SELECT));
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

        let base = if self.turn < BATCH_SIZE {
            0
        } else {
            BATCH_SIZE
        };
        for i in base..base + BATCH_SIZE {
            if self.wonder_cards_owner[i] == -1 && self.wonder_cards[i] == card as i32 {
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
        let base = if self.turn < BATCH_SIZE {
            0
        } else {
            BATCH_SIZE
        };
        for i in base..base + BATCH_SIZE {
            if self.wonder_cards_owner[i] == -1 && self.wonder_cards[i] == card {
                self.wonder_cards_owner[i] = player;
                break;
            }
        }

        self.history.push((player, action));

        self.turn += 1;

        if self.turn >= NUM_SELECTED {
            self.done = true;
            // Count "scoring" wonders (card IDs 0–7) per player; IDs 8–11 are non-scoring.
            let mut score = [0i32; 2];
            for i in 0..NUM_SELECTED {
                if self.wonder_cards[i] >= 0 && self.wonder_cards[i] <= 7 {
                    score[self.wonder_cards_owner[i] as usize] += 1;
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

    pub fn history(&self) -> &[(i32, usize)] {
        &self.history
    }

    pub fn wonder_cards(&self) -> &[i32; NUM_SELECTED] {
        &self.wonder_cards
    }

    pub fn show(&self) {
        print!("Turn {}/{}", self.turn, NUM_SELECTED);
        if !self.done {
            print!(" | Player {} to pick", self.current_player());
        }
        println!();

        let num_batches = if self.turn < BATCH_SIZE { 1 } else { 2 };
        for batch in 0..num_batches {
            let base = batch * BATCH_SIZE;
            print!("  Batch {}:", batch);
            for i in base..base + BATCH_SIZE {
                if self.wonder_cards_owner[i] >= 0 {
                    print!(
                        " [{}:P{}]",
                        card_char(self.wonder_cards[i] as usize),
                        self.wonder_cards_owner[i]
                    );
                } else {
                    print!(" {}", card_char(self.wonder_cards[i] as usize));
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

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_action_encode_decode_roundtrip() {
        for card in 0..NUM_WONDERS {
            for op in 0..NUM_OPS {
                let action = action_encode(card, op);
                assert_eq!(action_card(action), card);
                assert_eq!(action_op(action), op);
            }
        }
    }

    #[test]
    fn test_card_char() {
        assert_eq!(card_char(0), '0');
        assert_eq!(card_char(9), '9');
        assert_eq!(card_char(10), 'a');
        assert_eq!(card_char(11), 'b');
    }

    #[test]
    fn test_legal_actions_count_at_start() {
        let game = Game::new();
        assert_eq!(game.legal_actions().len(), BATCH_SIZE);
    }

    #[test]
    fn test_is_legal_action() {
        let game = Game::new();
        let actions = game.legal_actions();

        // All returned actions are legal.
        for &a in &actions {
            assert!(game.is_legal_action(a));
        }

        // Out-of-range action is illegal.
        assert!(!game.is_legal_action(NUM_ACTIONS));

        // Actions from the second batch (not yet active) are illegal.
        // Build the set of first-batch actions.
        let first_batch: std::collections::HashSet<usize> = actions.iter().cloned().collect();
        // Enumerate all valid actions and check non-first-batch ones are illegal.
        for a in 0..NUM_ACTIONS {
            if !first_batch.contains(&a) {
                assert!(!game.is_legal_action(a));
            }
        }
    }

    #[test]
    fn test_apply_action_advances_turn_and_removes_card() {
        let mut game = Game::new();
        let actions = game.legal_actions();
        let first_action = actions[0];

        game.apply_action(first_action);

        // One slot picked — 3 remain in this batch.
        assert_eq!(game.legal_actions().len(), BATCH_SIZE - 1);

        // Applied action is no longer legal.
        assert!(!game.is_legal_action(first_action));

        // Turn advanced to index 1, so current player is DRAFT_PLAYER[1].
        assert_eq!(game.current_player(), DRAFT_PLAYER[1]);
    }

    #[test]
    fn test_full_game_plays_to_completion() {
        let mut game = Game::new();
        for _ in 0..NUM_SELECTED {
            let action = game.legal_actions()[0];
            game.apply_action(action);
        }
        assert!(game.is_over());
        let w = game.winner();
        assert!(w == 0 || w == 1 || w == 2);
    }

    #[test]
    fn test_history_records_all_moves() {
        let mut game = Game::new();
        let mut expected = Vec::new();
        for turn in 0..NUM_SELECTED {
            let player = game.current_player();
            let action = game.legal_actions()[0];
            expected.push((player, action));
            game.apply_action(action);
            assert_eq!(game.history().len(), turn + 1);
        }
        assert!(game.is_over());
        let history = game.history();
        assert_eq!(history.len(), NUM_SELECTED);
        for i in 0..NUM_SELECTED {
            assert_eq!(history[i], expected[i]);
        }
    }

    #[test]
    fn test_current_player_returns_minus_one_when_done() {
        let mut game = Game::new();
        for _ in 0..NUM_SELECTED {
            let action = game.legal_actions()[0];
            game.apply_action(action);
        }
        assert_eq!(game.current_player(), -1);
    }
}
