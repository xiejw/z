// Human Select is an improved Human policy allowing users to select the move
// with visualization.
package policy

import (
	"os"

	"atomicgo.dev/keyboard"
	"atomicgo.dev/keyboard/keys"
	"github.com/rs/zerolog/log"

	"g5/go/lib/game"
)

type HumanSelectPolicy struct {
	name   string
	color  game.Color
	states map[[2]int]bool
	board  game.Board
}

func NewHumanSelectPolicy(name string, color game.Color) Policy {
	return &HumanSelectPolicy{
		name:   name,
		color:  color,
		states: make(map[[2]int]bool),
		board:  game.NewBoard(),
	}
}

func (p *HumanSelectPolicy) GetName() string      { return p.name }
func (p *HumanSelectPolicy) GetColor() game.Color { return p.color }
func (p *HumanSelectPolicy) GetNextMove(lastMovePos game.Pos, lastMoveColor game.Color) game.Pos {
	if lastMoveColor != game.CLR_NA {
		if lastMoveColor == p.color {
			panic("should not be same color")
		}

		x := lastMovePos.X()
		y := lastMovePos.Y()

		if !p.isMoveLegal(x, y) {
			log.Panic().Msgf("Pos(%v,%v) is occupied already", x, y)
		}
		p.states[[2]int{x, y}] = true
		p.board.NewMove(lastMovePos, lastMoveColor)
	}

	var x int = game.NumRows / 2
	var y int = game.NumCols / 2
	var shouldExist bool

	p.board.SetTryMove(game.NewPos(x, y), p.GetColor())
	p.board.Draw(os.Stdout)

	keyboard.Listen(func(key keys.Key) (stop bool, err error) {
		if key.Code == keys.CtrlC {
			shouldExist = true
			return true, nil // Stop listener by returning true on Ctrl+C
		}

		if key.Code == keys.Up {
			if x > 0 {
				x--
				p.board.SetTryMove(game.NewPos(x, y), p.GetColor())
				p.board.Draw(os.Stdout)
			}
		}

		if key.Code == keys.Down {
			if x < game.NumRows-1 {
				x++
				p.board.SetTryMove(game.NewPos(x, y), p.GetColor())
				p.board.Draw(os.Stdout)
			}
		}
		if key.Code == keys.Left {
			if y > 0 {
				y--
				p.board.SetTryMove(game.NewPos(x, y), p.GetColor())
				p.board.Draw(os.Stdout)
			}
		}
		if key.Code == keys.Right {
			if y < game.NumCols-1 {
				y++
				p.board.SetTryMove(game.NewPos(x, y), p.GetColor())
				p.board.Draw(os.Stdout)
			}
		}

		if key.Code == keys.Enter {
			if p.isMoveLegal(x, y) {
				p.states[[2]int{x, y}] = true
				p.board.NewMove(game.NewPos(x, y), p.GetColor())
				return true, nil
			}

			log.Error().Msgf(">>> Invalid Pos to set")
		}
		return false, nil

	})
	if shouldExist {
		os.Exit(1)
	}
	return game.NewPos(x, y)
}

func (p *HumanSelectPolicy) isMoveLegal(x, y int) bool {
	pos := [2]int{x, y}
	return !p.states[pos]
}

func (p *HumanSelectPolicy) isMoveValid(x, bound int) bool {
	if x < 0 || x >= bound {
		return false
	}
	return true
}
