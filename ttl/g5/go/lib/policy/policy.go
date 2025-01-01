package policy

import (
	"g5/go/lib/game"
)

type Policy interface {
	GetName() string
	GetColor() game.Color
	GetNextMove(lastMovePos game.Pos, lastMoveColor game.Color) game.Pos // CLR_NA if no move
}
