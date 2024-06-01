// Human Select is an improved Human policy allowing users to select the move
// with visualization. The HumanPolicy prompts users to type x and y as input
// in stdin. It works but it is very easy to type incorrectly.
// HumanSelectPolicy provides a visualized board to select the move on the
// board.
package policy

import (
	"os"

	"atomicgo.dev/keyboard"
	"atomicgo.dev/keyboard/keys"
	"github.com/rs/zerolog/log"

	"g5/go/lib/game"
)

type HumanSelectPolicy struct {
	name  string
	color game.Color

	// Internal states
	states       map[game.Pos]bool // Record which positions are taken.
	lastRoundPos game.Pos          // Record the pos user typed for this policy last round.

	// Draw the current board with potential selection (but not committed).
	// This is a separated copy so the main board used by the game loop is
	// not populated.
	internalBoard game.Board
}

func NewHumanSelectPolicy(name string, color game.Color) Policy {
	return &HumanSelectPolicy{
		name:          name,
		color:         color,
		states:        make(map[game.Pos]bool),
		internalBoard: game.NewBoard(),
		lastRoundPos:  game.NewPos(game.NumRows/2, game.NumCols/2),
	}
}

func (p *HumanSelectPolicy) GetName() string      { return p.name }
func (p *HumanSelectPolicy) GetColor() game.Color { return p.color }

func (p *HumanSelectPolicy) GetNextMove(lastMovePos game.Pos, lastMoveColor game.Color) game.Pos {
	if lastMoveColor != game.CLR_NA {
		if lastMoveColor == p.color {
			panic("should not be same color")
		}
		if p.states[lastMovePos] {
			x := lastMovePos.X()
			y := lastMovePos.Y()
			log.Panic().Msgf("Pos(%v,%v) is occupied already", x, y)
		}
		p.states[lastMovePos] = true
		p.internalBoard.NewMove(lastMovePos, lastMoveColor)
	}

	var shouldExit bool
	var escapePressedCount int
	var x int = p.lastRoundPos.X()
	var y int = p.lastRoundPos.Y()

	p.internalBoard.SetTryMove(game.NewPos(x, y), p.GetColor())
	p.internalBoard.Draw(os.Stdout)

	keyboard.Listen(func(key keys.Key) (stop bool, err error) {
		if key.Code == keys.CtrlC {
			log.Warn().Msgf("Got \"Ctrl-C\". Quiting")
			shouldExit = true
			return true, nil
		}

		if key.Code == keys.Escape {
			if escapePressedCount >= 1 {
				log.Warn().Msgf("Got \"Escape\" key twice. Quiting")
				shouldExit = true
				return true, nil
			}
			log.Info().Msgf("Got \"Escape\" key once. Press twice to Quit.")
			escapePressedCount++
		} else {
			escapePressedCount = 0
		}

		if key.Code == keys.Up {
			if x > 0 {
				x--
				p.internalBoard.SetTryMove(game.NewPos(x, y), p.GetColor())
				p.internalBoard.Draw(os.Stdout)
			}
		}

		if key.Code == keys.Down {
			if x < game.NumRows-1 {
				x++
				p.internalBoard.SetTryMove(game.NewPos(x, y), p.GetColor())
				p.internalBoard.Draw(os.Stdout)
			}
		}
		if key.Code == keys.Left {
			if y > 0 {
				y--
				p.internalBoard.SetTryMove(game.NewPos(x, y), p.GetColor())
				p.internalBoard.Draw(os.Stdout)
			}
		}
		if key.Code == keys.Right {
			if y < game.NumCols-1 {
				y++
				p.internalBoard.SetTryMove(game.NewPos(x, y), p.GetColor())
				p.internalBoard.Draw(os.Stdout)
			}
		}

		if key.Code == keys.Enter {
			newPos := game.NewPos(x, y)
			if !p.states[newPos] {
				p.states[newPos] = true
				p.internalBoard.NewMove(newPos, p.GetColor())
				return true, nil
			}

			log.Error().Msgf(">>> Invalid Pos to set")
		}
		return false, nil

	})
	if shouldExit {
		os.Exit(1)
	}
	p.lastRoundPos = game.NewPos(x, y)
	return p.lastRoundPos
}
