package control

import (
	"testing"

	"g5/go/lib/game"
	"g5/go/lib/policy"
)

func TestLoop(t *testing.T) {
	p1 := policy.NewRandomPolicy("r1", game.CLR_BLACK)
	p2 := policy.NewRandomPolicy("r2", game.CLR_WHITE)
	err := StartGame(p1, p2, nil)
	if err != nil {
		t.Errorf("unexpected err: %v", err)
	}
}
