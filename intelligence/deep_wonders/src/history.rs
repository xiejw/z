use std::fs;
use std::io::Write;
use std::time::SystemTime;

use crate::game::{action_card, action_op, Game};

/// Convert days since epoch to (year, month, day).
fn epoch_days_to_date(mut days: u64) -> (u64, u64, u64) {
    // Algorithm from http://howardhinnant.github.io/date_algorithms.html
    days += 719_468;
    let era = days / 146_097;
    let doe = days - era * 146_097;
    let yoe = (doe - doe / 1460 + doe / 36524 - doe / 146_096) / 365;
    let y = yoe + era * 400;
    let doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    let mp = (5 * doy + 2) / 153;
    let d = doy - (153 * mp + 2) / 5 + 1;
    let m = if mp < 10 { mp + 3 } else { mp - 9 };
    let y = if m <= 2 { y + 1 } else { y };
    (y, m, d)
}

/// Write a completed game to a JSONL log file in `logs/`.
/// Returns the file path on success.
pub fn write_game_log(game: &Game) -> Result<String, std::io::Error> {
    fs::create_dir_all("logs")?;

    // Generate date string.
    let secs = SystemTime::now()
        .duration_since(SystemTime::UNIX_EPOCH)
        .unwrap_or_default()
        .as_secs();
    let days = secs / 86400;
    let (y, m, d) = epoch_days_to_date(days);
    let date_str = format!("{:04}-{:02}-{:02}", y, m, d);

    // Generate 6 random alphanumeric chars using simple LCG seeded from time nanos.
    let nanos = SystemTime::now()
        .duration_since(SystemTime::UNIX_EPOCH)
        .unwrap_or_default()
        .as_nanos();
    let chars = b"abcdefghijklmnopqrstuvwxyz0123456789";
    let mut seed = nanos as u64;
    let mut suffix = String::with_capacity(6);
    for _ in 0..6 {
        seed = seed.wrapping_mul(6364136223846793005).wrapping_add(1);
        suffix.push(chars[(seed >> 33) as usize % chars.len()] as char);
    }

    let filename = format!("logs/{}_{}_{}.txt", date_str, secs, suffix);
    let mut file = fs::File::create(&filename)?;

    // Header: wonder cards.
    let wc = game.wonder_cards();
    write!(file, "{{\"type\":\"header\",\"wonder_cards\":[")?;
    for (i, &c) in wc.iter().enumerate() {
        if i > 0 {
            write!(file, ",")?;
        }
        write!(file, "{}", c)?;
    }
    writeln!(file, "]}}")?;

    // Moves.
    for (turn, &(player, action)) in game.history().iter().enumerate() {
        let card = action_card(action);
        let op = action_op(action);
        writeln!(
            file,
            "{{\"type\":\"move\",\"turn\":{},\"player\":{},\"action\":{},\"card\":{},\"op\":{}}}",
            turn, player, action, card, op
        )?;
    }

    // Result.
    writeln!(file, "{{\"type\":\"result\",\"winner\":{}}}", game.winner())?;

    Ok(filename)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_epoch_days_to_date() {
        // 2025-01-01 is day 20089
        let (y, m, d) = epoch_days_to_date(20089);
        assert_eq!((y, m, d), (2025, 1, 1));
    }

    #[test]
    fn test_write_game_log() {
        let mut game = Game::new();
        while !game.is_over() {
            let action = game.legal_actions()[0];
            game.apply_action(action);
        }
        let path = write_game_log(&game).expect("write_game_log failed");
        assert!(path.starts_with("logs/"));
        assert!(path.ends_with(".txt"));

        let contents = std::fs::read_to_string(&path).expect("read log file");
        let lines: Vec<&str> = contents.lines().collect();
        // 1 header + 8 moves + 1 result = 10 lines
        assert_eq!(lines.len(), 10);
        assert!(lines[0].contains("\"type\":\"header\""));
        for i in 1..9 {
            assert!(lines[i].contains("\"type\":\"move\""));
        }
        assert!(lines[9].contains("\"type\":\"result\""));

        // Clean up.
        let _ = std::fs::remove_file(&path);
    }
}
