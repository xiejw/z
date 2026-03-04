// vim: ft=cpp
// deep_wonders:v1
#pragma once

#define NUM_WONDERS   12
#define NUM_SELECTED   8
#define BATCH_SIZE     4
#define NUM_OPS        1
#define OP_SELECT      0
#define NUM_CARDS      NUM_WONDERS
#define NUM_ACTIONS   (NUM_CARDS * NUM_OPS)

static inline int ActionEncode( int card, int op ) { return card * NUM_OPS + op; }
static inline int ActionCard( int action )          { return action / NUM_OPS; }
static inline int ActionOp( int action )            { return action % NUM_OPS; }

namespace deep_wonders {

class Game {
public:
        Game();
        Game *Dup() const;
        int  NumActions() const;
        int  LegalActions( int *out ) const;
        bool IsLegalAction( int action ) const;
        void ApplyAction( int action );
        int  CurrentPlayer() const;
        int  Winner() const;
        bool IsOver() const;
        void Show() const;

private:
        int  selected_[NUM_SELECTED];   /* the 8 wonder IDs chosen */
        bool picked_[NUM_SELECTED];     /* true if slot has been drafted */
        int  owner_[NUM_SELECTED];      /* -1=available, 0/1=player who picked */
        int  turn_;                     /* 0-7 then done */
        bool done_;
        int  winner_;                   /* -1 ongoing, 0/1 winner, 2 tie */
};

}  // namespace deep_wonders
