package policy

import (
	"testing"

	"atomicgo.dev/keyboard"
	"atomicgo.dev/keyboard/keys"

	"g5/go/lib/game"
)

func TestHumanVisPolicy(t *testing.T) {
	go func() {
		keyboard.SimulateKeyPress(keys.Enter)
	}()

	// Create a policy and swap the reader
	p := NewHumanVisPolicy("h1", game.CLR_WHITE)

	if p.GetName() != "h1" {
		t.Errorf("bad name")
	}
	if p.GetColor() != game.CLR_WHITE {
		t.Errorf("bad color")
	}
	pos := p.GetNextMove(game.NewPos(0, 0), game.CLR_NA)
	if pos.X() != game.NumRows/2 {
		t.Errorf("bad x: %v", pos.X())
	}
	if pos.Y() != game.NumCols/2 {
		t.Errorf("bad y: %v", pos.Y())
	}
}

func TestHumanVisRetryPolicy(t *testing.T) {
	go func() {
		keyboard.SimulateKeyPress(keys.Enter)
		keyboard.SimulateKeyPress(keys.Left)
		keyboard.SimulateKeyPress(keys.Left)
		keyboard.SimulateKeyPress(keys.Up)
		keyboard.SimulateKeyPress(keys.Enter)
	}()

	// Create a policy and swap the reader
	p := NewHumanVisPolicy("h1", game.CLR_WHITE)

	if p.GetName() != "h1" {
		t.Errorf("bad name")
	}
	if p.GetColor() != game.CLR_WHITE {
		t.Errorf("bad color")
	}

	startX := game.NumRows / 2
	startY := game.NumCols / 2
	pos := p.GetNextMove(game.NewPos(startX, startY), game.CLR_BLACK)
	if pos.X() != game.NumRows/2-1 {
		t.Errorf("bad x: %v", pos.X())
	}
	if pos.Y() != game.NumCols/2-2 {
		t.Errorf("bad y: %v", pos.Y())
	}
}
