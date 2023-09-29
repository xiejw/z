#ifndef C4_POLICY_H_
#define C4_POLICY_H_

#include "base.h"

// this polic only looks ahead one step. So super fast but not that strong.
int policy_lookahead_onestep(const std::vector<int> &b, int next_player_color);

#endif  // C4_POLICY_H_
