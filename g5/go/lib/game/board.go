package game

import (
	"bytes"
	"errors"
	"fmt"
	"io"

	"github.com/rs/zerolog/log"
)

var (
	ERROR_ILLEGAL_MOVE = errors.New("illgal move")
	ERROR_GAME_OVER    = errors.New("game is over")
)

type Board interface {
	Draw(io.Writer)
	NewMove(Pos, Color) (winner bool, err error)
	GetLastMove() (Pos, Color) // CLR_NA means no move yet.
	GetWiner() Color           // CLR_NA means no winner.
	AttachHook(Hook) error

	SetTryMove(pos Pos, color Color)
	ClearTryMove()
}

type Hook interface {
	GetName() string
	ActOnMove(Pos, Color) error
	ActOnLogWinner(Color) error
}

func NewBoard() Board {
	if NumRows <= 0 || NumRows >= 16 {
		panic("the impl assumes NumRows is in [1, 15]")
	}
	if NumCols <= 0 || NumCols >= 16 {
		panic("the impl assumes H is in [1, 15]")
	}
	return &termBoard{}
}

type boardMove struct {
	Pos   Pos
	Color Color
}

type termBoard struct {
	bState        [32]byte
	wState        [32]byte
	winner        Color
	lastMovePos   Pos
	lastMoveColor Color
	tryMove       *boardMove
}

const (
	BASH_CLR_GREY   = "\033[1;30m" // Boader
	BASH_CLR_GREEN  = "\033[1;32m" // For last Move with X
	BASH_CLR_PURPLE = "\033[1;35m" // For last Move with O
	BASH_CLR_RED    = "\033[1;31m" // For invalid try Move
	BASH_CLR_BLUE   = "\033[1;34m" // For valid try Move
	BASH_CLR_NONE   = "\033[0m"
)

// -----------------------------------------------------------------------------
// Conform Board Interface
//

func (b *termBoard) Draw(in io.Writer) {
	// Use a local buffer first to avoid blinking output.
	var w = new(bytes.Buffer)

	// Header
	fmt.Fprintf(w, "x\\y ")
	for y := range NumCols {
		fmt.Fprintf(w, "%2d  ", y)
	}
	fmt.Fprintf(w, "\n")

	// Top Boarder
	bFn := func() { // draw a horizontal boarder line
		fmt.Fprintf(w, BASH_CLR_GREY)
		fmt.Fprintf(w, "   +")
		for _ = range NumCols {
			fmt.Fprintf(w, "---+")
		}
		fmt.Fprintf(w, "\n")
		fmt.Fprintf(w, BASH_CLR_NONE)

	}
	bFn()

	// Board
	vFn := func() { // draw a vertical | char
		fmt.Fprintf(w, BASH_CLR_GREY)
		fmt.Fprintf(w, "|")
		fmt.Fprintf(w, BASH_CLR_NONE)
	}
	for x := range NumRows {
		fmt.Fprintf(w, "%2d ", x)
		vFn()
		for y := range NumCols {
			c := b.getMove(NewPos(x, y))

			lastMove := b.lastMoveColor != CLR_NA && b.lastMovePos.X() == x && b.lastMovePos.Y() == y

			if b.tryMove != nil && b.tryMove.Pos.X() == x && b.tryMove.Pos.Y() == y {
				if c == CLR_NA {
					// A valid try Move
					fmt.Fprintf(w, "%v", BASH_CLR_BLUE)
					switch b.tryMove.Color {
					case CLR_BLACK:
						fmt.Fprintf(w, " %v ", "x")
					case CLR_WHITE:
						fmt.Fprintf(w, " %v ", "o")
					}
					fmt.Fprintf(w, "%v", BASH_CLR_NONE)
				} else {
					// A invalid try Move
					fmt.Fprintf(w, "%v", BASH_CLR_RED)
					switch b.tryMove.Color {
					case CLR_BLACK:
						fmt.Fprintf(w, " %v ", "x")
					case CLR_WHITE:
						fmt.Fprintf(w, " %v ", "o")
					}
					fmt.Fprintf(w, "%v", BASH_CLR_NONE)
				}
			} else {
				// This is not same as try Move. Do special
				// logic for last Move
				if lastMove {
					if b.lastMoveColor == CLR_BLACK {
						fmt.Fprintf(w, "%v", BASH_CLR_GREEN)
					} else {
						fmt.Fprintf(w, "%v", BASH_CLR_PURPLE)
					}
				}
				switch c {
				case CLR_BLACK:
					fmt.Fprintf(w, " %v ", "x")
				case CLR_WHITE:
					fmt.Fprintf(w, " %v ", "o")
				default:
					fmt.Fprintf(w, " %v ", " ")
				}
				if lastMove {
					fmt.Fprintf(w, "%v", BASH_CLR_NONE)
				}
			}

			vFn()
		}
		fmt.Fprintf(w, "\n")
		bFn()
	}
	fmt.Fprintf(in, "%v", w.String())
}

func (b *termBoard) NewMove(pos Pos, color Color) (winner bool, err error) {
	if b.winner != CLR_NA {
		log.Panic().Msgf("NewMove cannot be called if there is winner already")
	}
	if color == CLR_NA {
		log.Panic().Msgf("CLR_NA is not supported by NewMove")
	}

	c := b.getMove(pos)
	if c != CLR_NA {
		return false, ERROR_ILLEGAL_MOVE
	}

	b.lastMovePos = pos
	b.lastMoveColor = color
	b.setMove(pos, color)
	winnerFound := b.findWinner()
	if winnerFound {
		b.winner = color
	}
	return winnerFound, nil
}
func (b *termBoard) GetLastMove() (Pos, Color) { return b.lastMovePos, b.lastMoveColor }
func (b *termBoard) GetWiner() Color           { return b.winner }
func (b *termBoard) AttachHook(Hook) error {
	if b.winner != CLR_NA {
		return ERROR_GAME_OVER
	}
	log.Panic().Msgf("AttachHook unimpl")
	return nil
}

// -----------------------------------------------------------------------------
// Public methods
func (b *termBoard) SetTryMove(pos Pos, color Color) {
	b.tryMove = &boardMove{
		Pos:   pos,
		Color: color,
	}
}
func (b *termBoard) ClearTryMove() { b.tryMove = nil }

// -----------------------------------------------------------------------------
// Helper methods
//

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

func (b *termBoard) findWinner() (winner bool) {
	if b.lastMoveColor == CLR_NA {
		log.Panic().Msgf("findWinner cannot be call if there is no move at all")
	}
	if b.winner != CLR_NA {
		log.Panic().Msgf("findWinner cannot be call if there is winner already")
	}

	x := b.lastMovePos.X()
	y := b.lastMovePos.Y()
	colorToMatch := b.lastMoveColor

	countMoveWithSameColor := func(currX, currY int, incFn func(x, y int) (int, int)) int {
		c := 0
		for {
			currX, currY = incFn(currX, currY)
			if currX < 0 || currX >= NumRows {
				return c
			}
			if currY < 0 || currY >= NumCols {
				return c
			}
			if b.getMove(NewPos(currX, currY)) != colorToMatch {
				return c
			}
			c++

		}
	}

	{
		xRightCount := countMoveWithSameColor(x, y, func(x, y int) (int, int) { return x, y + 1 })
		xLeftCount := countMoveWithSameColor(x, y, func(x, y int) (int, int) { return x, y - 1 })
		if xRightCount+1+xLeftCount >= NumMovesToWin {
			return true
		}
	}

	{
		yUpCount := countMoveWithSameColor(x, y, func(x, y int) (int, int) { return x - 1, y })
		yDownCount := countMoveWithSameColor(x, y, func(x, y int) (int, int) { return x + 1, y })
		if yUpCount+1+yDownCount >= NumMovesToWin {
			return true
		}
	}

	{
		xyUpRightCount := countMoveWithSameColor(x, y, func(x, y int) (int, int) { return x - 1, y + 1 })
		xyDownLeftCount := countMoveWithSameColor(x, y, func(x, y int) (int, int) { return x + 1, y - 1 })
		if xyUpRightCount+1+xyDownLeftCount >= NumMovesToWin {
			return true
		}
	}

	{
		xyUpLeftCount := countMoveWithSameColor(x, y, func(x, y int) (int, int) { return x - 1, y - 1 })
		xyDownRightCount := countMoveWithSameColor(x, y, func(x, y int) (int, int) { return x + 1, y + 1 })
		if xyUpLeftCount+1+xyDownRightCount >= NumMovesToWin {
			return true
		}
	}
	return false
}
