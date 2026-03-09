use deep_wonders::game::Game;
use deep_wonders::history::write_game_log;
use deep_wonders::info;
use deep_wonders::mcts::mcts_search;
use deep_wonders::nn::Nn;

const MCTS_ITER_CNT: i32 = 100;
const NUM_GAMES: i32 = 10;

fn self_play_one_game(nn: &Nn, game_id: i32) {
    let mut g = Game::new();

    while !g.is_over() {
        let action = mcts_search(&g, nn, MCTS_ITER_CNT);
        g.apply_action(action);
    }

    let winner = g.winner();
    if winner == 2 {
        println!("Game {}: Tie", game_id);
    } else {
        println!("Game {}: Player {} wins", game_id, winner);
    }

    match write_game_log(&g) {
        Ok(path) => info!("Game {} log written to {}", game_id, path),
        Err(e) => eprintln!("Warning: failed to write game {} log: {}", game_id, e),
    }
}

fn main() {
    let nn = Nn::new();

    info!("Running {} self-play games.", NUM_GAMES);
    for i in 0..NUM_GAMES {
        self_play_one_game(&nn, i);
    }
    info!("Self-play complete.");
}
