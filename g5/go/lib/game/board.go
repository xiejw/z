package game

import (
	"io"
)

type Board interface {
	NewMove(Pos, Color) (winner bool, err error)
	Draw(io.Writer)
	GetLastMove() (Pos, Color) // CLR_NA means no move yet.
	GetWiner() Color           // CLR_NA means no winner.
}

type Tracer interface {
	LogMove(Pos, Color)
	LogWinner(Color)
}

func NewBoard() Board {
	return nil
}
