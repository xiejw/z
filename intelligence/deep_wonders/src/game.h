// vim: ft=cpp
// deep_wonders:v1
#pragma once

#define NUM_ACTIONS 3 /* Stub: 3 placeholder actions */
#define MAX_TURNS  10 /* Stub: game ends after 10 turns */

namespace deep_wonders {

class Game {
public:
        /// Initialize game to starting state.
        Game();

        /// Deep-copy this game. Caller owns the returned pointer.
        Game *Dup() const;

        /// Number of legal actions available (may vary by game state in real games).
        int  NumActions() const;

        /// Apply action, advancing the game state. Asserts action is valid.
        void ApplyAction( int action );

        /// Whose turn it is: 0 or 1.
        int  CurrentPlayer() const;

        /// Returns -1 (ongoing), 0 or 1 (winner), 2 (tie).
        int  Winner() const;

        /// True if the game has ended.
        bool IsOver() const;

        /// Print current state to stdout.
        void Show() const;

private:
        int  current_player_;
        int  turn_;
        bool done_;
        int  winner_;
};

}  // namespace deep_wonders
