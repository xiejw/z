package game

import (
	"testing"
)

func TestPos(t *testing.T) {
	for x := range 15 {
		for y := range 15 {
			pos := NewPos(x, y)
			if x != pos.X() {
				t.Errorf("x mismatch")
			}
			if y != pos.Y() {
				t.Errorf("y mismatch")
			}
		}
	}
}
