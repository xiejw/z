package policy

import (
	"bufio"
	"fmt"
	"os"
	"strconv"
	"strings"

	"github.com/rs/zerolog/log"

	"g5/go/lib/game"
)

type HumanPolicy struct {
	name   string
	color  game.Color
	states map[[2]int]bool
	reader *bufio.Reader
}

func NewHumanPolicy(name string, color game.Color) Policy {
	return &HumanPolicy{
		name:   name,
		color:  color,
		states: make(map[[2]int]bool),
		reader: bufio.NewReader(os.Stdin),
	}
}

func (p *HumanPolicy) GetName() string      { return p.name }
func (p *HumanPolicy) GetColor() game.Color { return p.color }
func (p *HumanPolicy) GetNextMove(lastMovePos game.Pos, lastMoveColor game.Color) game.Pos {
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
	var x, y int
	var err error
	for {
		fmt.Printf("X: ")
		x, err = p.readInt()
		if err != nil {
			log.Error().Err(err).Msgf("Invalid int number. Try again.")
			continue
		}
		if !p.isMoveValid(x, game.NumRows) {
			log.Error().Msgf("Invalid input. Try again.")
			continue
		}

		fmt.Printf("Y: ")
		y, err = p.readInt()
		if err != nil {
			log.Error().Err(err).Msgf("Invalid int number. Try again.")
			continue
		}
		if !p.isMoveValid(y, game.NumCols) {
			log.Error().Msgf("Invalid input. Try again.")
			continue
		}

		if !p.isMoveLegal(x, y) {
			log.Error().Msgf("Illegal input. Try again.")
			continue
		}
		break
	}
	p.states[[2]int{x, y}] = true
	return game.NewPos(x, y)
}

func (p *HumanPolicy) isMoveLegal(x, y int) bool {
	pos := [2]int{x, y}
	return !p.states[pos]
}

func (p *HumanPolicy) isMoveValid(x, bound int) bool {
	if x < 0 || x >= bound {
		return false
	}
	return true
}

func (p *HumanPolicy) readInt() (int, error) {
	text, _ := p.reader.ReadString('\n')
	return strconv.Atoi(strings.Trim(text, " \r\n"))
}

// For testing now
func (p *HumanPolicy) attachNewReader(r *bufio.Reader) { p.reader = r }
