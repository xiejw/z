package policy

import (
	"math/rand/v2"

	"github.com/rs/zerolog/log"

	"g5/go/lib/game"
)

type RandomPolicy struct {
	name   string
	color  game.Color
	states map[[2]int]bool
}

func NewRandomPolicy(name string, color game.Color) Policy {
	return &RandomPolicy{
		name:   name,
		color:  color,
		states: make(map[[2]int]bool),
	}
}

func (p *RandomPolicy) GetName() string      { return p.name }
func (p *RandomPolicy) GetColor() game.Color { return p.color }
func (p *RandomPolicy) GetNextMove(lastMovePos game.Pos, lastMoveColor game.Color) game.Pos {
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
	}
	remainingStates := int64(game.NumRows*game.NumCols - len(p.states))
	for {
		for x := range game.NumRows {
			for y := range game.NumCols {
				if p.states[[2]int{x, y}] {
					continue
				}
				if rand.Int64N(remainingStates) == 0 {
					p.states[[2]int{x, y}] = true
					return game.NewPos(x, y)
				}
			}
		}
	}
}

func (p *RandomPolicy) isMoveLegal(x, y int) bool {
	pos := [2]int{x, y}
	return !p.states[pos]
}
