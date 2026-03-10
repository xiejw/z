use rand::Rng;

pub const NUM_WONDERS: usize = 12; // Total wonder cards in the pool
pub const NUM_SELECTED: usize = 8; // Wonder cards drawn per game (split into 2 batches)
pub const BATCH_SIZE: usize = 4; // Wonder cards revealed per batch; players draft from the active batch only
pub const NUM_OPS: usize = 1; // Operations per card (currently 1: select)
pub const OP_SELECT: usize = 0; // The only op: claim this card
pub const NUM_ACTIONS: usize = NUM_WONDERS * NUM_OPS; // Total action-space size = NUM_WONDERS × NUM_OPS
/// Number of floats per card in the observation vector (in_game, visible, unclaimed, owner_self, owner_opp).
/// Acts as the stride: card ID `c` occupies indices `[c*OBS_CARD_FEATURES .. c*OBS_CARD_FEATURES+4]`.
pub const OBS_CARD_FEATURES: usize = 5;
/// Number of game-level floats appended after the card section (game_stage, next_player).
pub const OBS_GLOBAL_FEATURES: usize = 2;
pub const OBS_SIZE: usize = NUM_WONDERS * OBS_CARD_FEATURES + OBS_GLOBAL_FEATURES;

// === --- Wonder Cards ---------------------------------------------------- ===
//

/// Snake draft order: P0, P1, P1, P0, P1, P0, P0, P1
const DRAFT_PLAYER: [i32; NUM_SELECTED] = [0, 1, 1, 0, 1, 0, 0, 1];

// === --- Card and Action ------------------------------------------------- ===
//

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum GameStage {
    WonderBatch0,
    WonderBatch1,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Action {
    SelectWonder(usize), // card id
}

impl Action {
    pub fn encode(self) -> usize {
        match self {
            Action::SelectWonder(card) => card * NUM_OPS + OP_SELECT,
        }
    }

    pub fn decode(encoded: usize) -> Self {
        let card = encoded / NUM_OPS;
        let op = encoded % NUM_OPS;
        match op {
            OP_SELECT => Action::SelectWonder(card),
            _ => panic!("invalid action encoding: {}", encoded),
        }
    }

    pub fn card(self) -> usize {
        match self {
            Action::SelectWonder(card) => card,
        }
    }

    pub fn op(self) -> usize {
        match self {
            Action::SelectWonder(_) => OP_SELECT,
        }
    }
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
    history: Vec<(i32, Action)>,
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

    pub fn legal_actions(&self) -> Vec<Action> {
        assert!(!self.done);
        let base = if self.turn < BATCH_SIZE {
            0
        } else {
            BATCH_SIZE
        };
        let mut actions = Vec::new();
        for i in base..base + BATCH_SIZE {
            if self.wonder_cards_owner[i] == -1 {
                actions.push(Action::SelectWonder(self.wonder_cards[i] as usize));
            }
        }
        actions
    }

    pub fn is_legal_action(&self, action: Action) -> bool {
        if self.done {
            return false;
        }
        let card = action.card();

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

    pub fn apply_action(&mut self, action: Action) {
        assert!(!self.done);
        assert!(self.is_legal_action(action));

        let card = action.card() as i32;
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

    pub fn history(&self) -> &[(i32, Action)] {
        &self.history
    }

    pub fn game_stage(&self) -> GameStage {
        if self.turn < BATCH_SIZE {
            GameStage::WonderBatch0
        } else {
            GameStage::WonderBatch1
        }
    }

    /// Build a fixed-size observation vector of length `OBS_SIZE` (= 62) for
    /// the current player.  The layout has two sections:
    ///
    /// **Card section** — `NUM_WONDERS * OBS_CARD_FEATURES` = 12 × 5 = 60 floats.
    ///
    /// The vector is *card-centric*: card ID `c` always occupies indices
    /// `[c*5 .. c*5+4]`, regardless of which draft slot it was placed in.
    /// `OBS_CARD_FEATURES = 5` is the stride between consecutive card blocks.
    /// Within each block the five features are:
    ///
    /// ```text
    ///   base+0  in_game    — 1 if card c was drawn into this game
    ///   base+1  visible    — 1 if in_game AND the card has been revealed
    ///                        (batch 0 is always revealed; batch 1 only after
    ///                        all four batch-0 picks are done)
    ///   base+2  unclaimed  — 1 if visible AND no player has taken it yet
    ///   base+3  owner_self — 1 if visible AND taken by the current player
    ///   base+4  owner_opp  — 1 if visible AND taken by the opponent
    /// ```
    ///
    /// Cards not drawn this game, and batch-1 cards before they are revealed,
    /// have all five features = 0.  Exactly one of {unclaimed, owner_self,
    /// owner_opp} is 1 for every visible card.
    ///
    /// **Global section** — `OBS_GLOBAL_FEATURES` = 2 floats, starting at
    /// index `NUM_WONDERS * OBS_CARD_FEATURES` = 60.
    ///
    /// ```text
    ///   60  game_stage   — 0.0 = WonderBatch0, 1.0 = WonderBatch1
    ///   61  next_player  — 0.0 or 1.0, absolute ID of the player to move
    /// ```
    pub fn observe(&self) -> [f32; OBS_SIZE] {
        let mut obs = [0.0f32; OBS_SIZE];
        let player = self.current_player();
        let batch1_revealed = self.turn >= BATCH_SIZE;

        for slot in 0..NUM_SELECTED {
            let card = self.wonder_cards[slot] as usize;
            let in_batch1 = slot >= BATCH_SIZE;
            let visible = !in_batch1 || batch1_revealed;
            // Card-centric stride: each card ID maps to a fixed block of
            // OBS_CARD_FEATURES (= 5) consecutive floats.
            let base = card * OBS_CARD_FEATURES;

            if !visible {
                continue; // batch 1 not yet revealed: all features stay 0
            }
            obs[base + 0] = 1.0; // in_game
            obs[base + 1] = 1.0; // visible
            let owner = self.wonder_cards_owner[slot];
            if owner == -1 {
                obs[base + 2] = 1.0; // unclaimed
            } else if owner == player {
                obs[base + 3] = 1.0; // owner_self
            } else {
                obs[base + 4] = 1.0; // owner_opp
            }
        }

        // Global features begin immediately after the card section.
        // OBS_GLOBAL_FEATURES = 2, so this block occupies indices [60, 61].
        let global_base = NUM_WONDERS * OBS_CARD_FEATURES;
        obs[global_base + 0] = match self.game_stage() {
            GameStage::WonderBatch0 => 0.0,
            GameStage::WonderBatch1 => 1.0,
        };
        obs[global_base + 1] = player as f32; // next_player: 0.0 or 1.0
        obs
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
            let action = Action::SelectWonder(card);
            let encoded = action.encode();
            assert_eq!(Action::decode(encoded).card(), card);
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

        // Actions from the second batch (not yet active) are illegal.
        // Build the set of first-batch actions.
        let first_batch: std::collections::HashSet<usize> =
            actions.iter().map(|a| a.encode()).collect();
        // Enumerate all valid encoded actions and check non-first-batch ones are illegal.
        for a in 0..NUM_ACTIONS {
            if !first_batch.contains(&a) {
                assert!(!game.is_legal_action(Action::decode(a)));
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
    fn test_observe_shape_and_basic_invariants() {
        let game = Game::new();
        let obs = game.observe();
        assert_eq!(obs.len(), OBS_SIZE);
        // All values in [0, 1].
        for &v in &obs {
            assert!(v >= 0.0 && v <= 1.0);
        }
        // Exactly BATCH_SIZE cards visible and unclaimed at start.
        let unclaimed: usize = (0..NUM_WONDERS)
            .filter(|&c| obs[c * OBS_CARD_FEATURES + 2] == 1.0)
            .count();
        assert_eq!(unclaimed, BATCH_SIZE);
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
