## c4c

A pure c implementation with zero dependency to play connect 4.  The model is
trained in Python world by self-playing the game with MCTS.

To play
```
make RELEASE=1
make                                               # Debug mode
make RELEASE=1 MCTS_ITER_CNT=1600                  # Really strong nn player but super slow
make RELEASE=1 MCTS_ITER_CNT=1600 MCTS_SELF_PLAY=1 # Two nn players play each other
```
Have fun!
