package game

import (
	"fmt"
	"io"
)

type Board interface {
	Draw(io.Writer)
	NewMove(Pos, Color) (winner bool, err error)
	GetLastMove() (Pos, Color) // CLR_NA means no move yet.
	GetWiner() Color           // CLR_NA means no winner.
	AttachHook(Hook) error
}

type Hook interface {
	ActOnMove(Pos, Color) error
	ActOnLogWinner(Color) error
}

func NewBoard() Board {
	return &termBoard{}
}

type termBoard struct {
	bState [16]byte
	wState [16]byte
	winner Color
}

const (
	GREY = "\033[1;30m"
	NC   = "\033[0m"
)

func (b *termBoard) Draw(w io.Writer) {
	// Header
	fmt.Fprintf(w, "x\\y ")
	for y := range H {
		fmt.Fprintf(w, "%2d  ", y)
	}
	fmt.Fprintf(w, "\n")

	// Top Boarder
	bFn := func() { // draw a horizontal boarder line
		fmt.Fprintf(w, GREY)
		fmt.Fprintf(w, "   +")
		for _ = range H {
			fmt.Fprintf(w, "---+")
		}
		fmt.Fprintf(w, "\n")
		fmt.Fprintf(w, NC)

	}
	bFn()

	// Board
	vFn := func() { // draw a vertical | char
		fmt.Fprintf(w, GREY)
		fmt.Fprintf(w, "|")
		fmt.Fprintf(w, NC)
	}
	for x := range W {
		fmt.Fprintf(w, "%2d ", x)
		vFn()
		for _ = range H {
			fmt.Fprintf(w, " %v ", "x")
			vFn()
		}
		fmt.Fprintf(w, "\n")
		bFn()
	}
}
func (b *termBoard) NewMove(Pos, Color) (winner bool, err error) { return false, nil }
func (b *termBoard) GetLastMove() (Pos, Color)                   { return NewPos(0, 0), CLR_NA }
func (b *termBoard) GetWiner() Color                             { return CLR_NA }
func (b *termBoard) AttachHook(Hook) error                       { return nil }
