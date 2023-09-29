This folder contains all scripts to run the components. In particular,

    # Plays the MCT policy, based on current best model, against human player.
    play_with_human.py

    # Runs the random policy to generate data so the training routing can
    # boostrap.
    #
    # Alternative solution is: Initializes the model for MCTS and dumps it
    # directly (without any weights udpate).
    bootstrap.py

    # Plays two MCTS policies, based on current best model, against each other.
    # This is used by RL routing to generate data.
    self_plays.py

    # Use scheduler to launch lots of self plays, by calling self_plays.py
    # above, to run self plays in parallel. This script sets a quene and
    # keeps the numbrer of active jobs below threshold.
    launch_self_plays.py

    # Loads the data from SQL and trains the model.
    # After training, a ckpt will be saved.
    train.py

    # A simple bash loop to call launch_self_plays and train in turns.
    loop.sh

    # Evaluates the models used by MCTS.
    # Caller should provide two ckpt paths. Currently it is hard coded in script
    # now :(
    evaluate.py

    # Evaluates elo rating for each ckeckpoint dumped in each iteration.
    # The results are collected via the output from `evaluate.py`
    elo_rating.py
