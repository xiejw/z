package policy

import (
	"bufio"
	"bytes"
	"testing"

	"g5/go/lib/game"
)

func TestHumanPolicy(t *testing.T) {
	var buf bytes.Buffer

	buf.WriteString("5\n")
	buf.WriteString("6\n")

	// Create a policy and swap the reader
	p := NewHumanPolicy("h1", game.CLR_WHITE)
	p.(*HumanPolicy).reader = bufio.NewReader(&buf)

	if p.GetName() != "h1" {
		t.Errorf("bad name")
	}
	if p.GetColor() != game.CLR_WHITE {
		t.Errorf("bad color")
	}
	pos := p.GetNextMove(game.NewPos(0, 0), game.CLR_NA)
	if pos.X() != 5 {
		t.Errorf("bad x: %v", pos.X())
	}
	if pos.Y() != 6 {
		t.Errorf("bad y: %v", pos.Y())
	}
}
