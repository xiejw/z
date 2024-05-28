package game

import (
	"errors"
	"fmt"
	"io"
)

var (
	ERROR_ILLEGAL_MOVE = errors.New("illgal move")
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
	bState [32]byte
	wState [32]byte
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
		for y := range H {
			c := b.getMove(NewPos(x, y))
			switch c {
			case CLR_BLACK:
				fmt.Fprintf(w, " %v ", "x")
			case CLR_WHITE:
				fmt.Fprintf(w, " %v ", "o")
			default:
				fmt.Fprintf(w, " %v ", " ")
			}
			vFn()
		}
		fmt.Fprintf(w, "\n")
		bFn()
	}
}
func (b *termBoard) NewMove(pos Pos, color Color) (winner bool, err error) {
	if color == CLR_NA {
		panic("CLR_NA is not supported by NewMove")
	}

	// TODO winner is not set
	c := b.getMove(pos)
	if c != CLR_NA {
		return false, ERROR_ILLEGAL_MOVE
	}

	b.setMove(pos, color)
	return false, nil
}
func (b *termBoard) GetLastMove() (Pos, Color) { return NewPos(0, 0), CLR_NA }
func (b *termBoard) GetWiner() Color           { return CLR_NA }
func (b *termBoard) AttachHook(Hook) error     { return nil }

func (b *termBoard) getMove(pos Pos) Color {
	idx := int(pos)
	bytePos := idx / 8
	byteBit := byte(1 << (idx % 8))

	isBOnBoard := b.bState[bytePos] & byteBit
	if isBOnBoard != 0 {
		return CLR_BLACK
	}

	isWOnBoard := b.wState[bytePos] & byteBit
	if isWOnBoard != 0 {
		return CLR_WHITE
	}

	return CLR_NA
}

func (b *termBoard) setMove(pos Pos, c Color) {
	if c == CLR_NA {
		panic("CLR_NA is not supported by setMove")
	}

	idx := int(pos)
	bytePos := idx / 8
	byteBit := byte(1 << (idx % 8))

	if c == CLR_BLACK {
		byt := b.bState[bytePos] | byteBit
		b.bState[bytePos] = byt
	} else {
		byt := b.wState[bytePos] | byteBit
		b.wState[bytePos] = byt
	}
}
