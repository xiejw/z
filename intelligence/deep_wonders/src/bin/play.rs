use std::io::{self, BufRead, Write};

use deep_wonders::game::*;
use deep_wonders::mcts::mcts_search;
use deep_wonders::nn::Nn;
use deep_wonders::{info, panic_log};

const MCTS_ITER_CNT: i32 = 100;

fn policy_human_move(g: &Game) -> usize {
    let legal = g.legal_actions();

    // Show available cards.
    print!("  Available:");
    for &a in &legal {
        let card = action_card(a);
        print!(" {}", card_char(card));
    }
    println!();

    let stdin = io::stdin();
    loop {
        print!("[Player {}] Pick a card: ", g.current_player());
        io::stdout().flush().unwrap();

        let mut line = String::new();
        if stdin.lock().read_line(&mut line).unwrap() == 0 {
            panic_log!("eof, unexpected.");
        }

        let movec = match line.trim().chars().next() {
            Some(c) => c,
            None => continue,
        };

        let card = if movec >= '0' && movec <= '9' {
            (movec as usize) - ('0' as usize)
        } else if movec >= 'a' && movec <= 'b' {
            10 + (movec as usize) - ('a' as usize)
        } else {
            println!("Invalid input. Use 0-9 or a-b.");
            continue;
        };

        let action = action_encode(card, OP_SELECT);
        if !g.is_legal_action(action) {
            println!("Card not available. Try again.");
            continue;
        }
        return action;
    }
}

fn policy_ai_move(g: &Game, nn: &Nn) -> usize {
    mcts_search(g, nn, MCTS_ITER_CNT)
}

fn play_game(nn: &Nn) {
    let mut g = Game::new();
    let ai_player = rand::random::<u32>() % 2;

    info!(
        "You are Player {}. AI is Player {}.",
        1 - ai_player,
        ai_player
    );

    g.show();
    while !g.is_over() {
        let action;
        if g.current_player() == ai_player as i32 {
            action = policy_ai_move(&g, nn);
            let card = action_card(action);
            println!("AI picks card {}", card_char(card));
        } else {
            action = policy_human_move(&g);
        }

        g.apply_action(action);
        g.show();
    }

    let winner = g.winner();
    if winner == 2 {
        println!("Game over: Tie!");
    } else if winner == ai_player as i32 {
        println!("Game over: AI wins!");
    } else {
        println!("Game over: You win!");
    }
}

fn main() {
    let nn = Nn::new();
    play_game(&nn);
}
