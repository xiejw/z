/*
[Tictactoe] is a program to play tic-tac-toe with RL algorithrm.

[Tictactoe]: https://en.wikipedia.org/wiki/Tic-tac-toe
*/
package main

import (
	"log"
	"math/rand"
	"os"

	"ttt/cmd/internal/board"
	"ttt/cmd/internal/drawing"
	"ttt/cmd/internal/policy"
)

func main() {
	// Prepare logging
	fo, err := os.Create("/tmp/tictactoe_log.txt")
	if err != nil {
		panic(err)
	}

	log.SetOutput(fo)
	defer fo.Close()

	// policy1 := &policy.DummyPolicy{
	// 	SleepSeconds: 1,
	// 	AIFirst:      true,
	// }

	aiFirst := rand.Int()&1 == 0
	log.Printf("aiFirst %v", aiFirst)

	policy2 := &policy.MCPolicy{
		SimulationCount: 1000,
		Seed:            123,
		AIFirst:         aiFirst,
	}
	policy2.Init()

	p := policy2

	drawing.BoardLoop("Tic Tac Toe", board.NewBoard(p))
}
