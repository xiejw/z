package policy

import (
	"fmt"
	"log"
	"math/rand"
)

const (
	MCBoardSize = 9
	MCNumToWin  = 3
	MCBoardW    = 3
	MCBoardH    = 3
)

// MCPolicy is designed based on a monte carlo algorithrm.
//
// It is hard code as 3x3 board. Also given there are 3^9 ~= 20k states, 1000 simulations can hit
// about half of the spaces. So, the algorithrm is quite strong.
type MCPolicy struct {
	SimulationCount int
	Seed            int64
	AIFirst         bool

	// Call Init() to fill these values.
	startValue  int
	movesValues map[[MCBoardSize]int]float32
	legalMoves  map[[MCBoardSize]int][]int
	r           *rand.Rand
	alpha       float32
}

func (p *MCPolicy) Init() {
	src := rand.NewSource(p.Seed)
	p.r = rand.New(src)
	p.alpha = 0.01

	if p.AIFirst {
		p.startValue = 1
	} else {
		p.startValue = -1
	}

	p.movesValues = make(map[[MCBoardSize]int]float32)
	p.legalMoves = make(map[[MCBoardSize]int][]int)

}

func (p *MCPolicy) IsAIFirst() bool {
	return p.AIFirst
}

func (p *MCPolicy) Query(w, h int, v []int) (row, col int) {
	if w != MCBoardW || h != MCBoardH {
		panic(fmt.Sprintf("w_x_h must be %v_x_%v", MCBoardW, MCBoardH))
	}

	totalCount := p.SimulationCount

	// Make a copy from the original board
	var v_copy [MCBoardSize]int
	copy(v_copy[:MCBoardSize], v)

	// Simulation multiple times
	for count := 0; count < totalCount; count++ {
		p.simulate(p.startValue, v_copy)
	}

	// Evaluate value for each legal move.
	moves, exists := p.legalMoves[v_copy]
	if !exists {
		panic("should exist")
	}

	log.Printf("==> simulation ended for board %v", v_copy)
	bestMove := -1
	var bestValue float32 = -100.0
	for _, nextMoveIndex := range moves {
		var v_arr_new [MCBoardSize]int = v_copy
		v_arr_new[nextMoveIndex] = p.startValue

		value := p.movesValues[v_arr_new]
		row = nextMoveIndex / MCBoardW
		col = nextMoveIndex % MCBoardW
		log.Printf("for move (%v,%v) value is: %v", row, col, value)

		if bestMove == -1 {
			bestMove = nextMoveIndex
			bestValue = value
		} else if value > bestValue {
			bestMove = nextMoveIndex
			bestValue = value
		}
	}

	row = bestMove / MCBoardW
	col = bestMove % MCBoardW
	return row, col
}

func (p *MCPolicy) simulate(next_v int, v [MCBoardSize]int) int {
	moves := p.queryLegalMoves(&v)
	movesCount := len(moves)

	selectedNextMoveIndex := p.r.Intn(movesCount)
	nextMoveIndex := moves[selectedNextMoveIndex]

	row := nextMoveIndex / MCBoardW
	col := nextMoveIndex % MCBoardW

	v[nextMoveIndex] = next_v

	winner_v, ended := p.winner(MCBoardSize-movesCount+1, row, col, next_v, &v)
	if !ended {
		winner_v = p.simulate(next_v*-1, v)
	}

	var award float32 = 0.0
	if winner_v == 0 {
		award = 0
	} else {
		if winner_v == next_v {
			award = 1.0
		} else {
			award = -1.0
		}
	}
	value := p.movesValues[v]
	value = value + p.alpha*(award-value)
	p.movesValues[v] = value
	return winner_v
}

// return indexes
func (p *MCPolicy) queryLegalMoves(v *[MCBoardSize]int) []int {
	moves, exists := p.legalMoves[*v]
	if exists {
		return moves
	}

	w, h := MCBoardW, MCBoardH
	moves = make([]int, 0, w*h)
	for row := 0; row < h; row++ {
		offset := row * w
		for col := 0; col < w; col++ {
			index := offset + col
			if (*v)[index] == 0 {
				moves = append(moves, index)
			}
		}
	}

	p.legalMoves[*v] = moves
	return moves
}

func (p *MCPolicy) winner(total int, row, col int, new_v int, v *[MCBoardSize]int) (int, bool) {
	numToWin := MCNumToWin
	w, h := MCBoardW, MCBoardH

	{ // cols
		numCols := 1
		i := row - 1
		for i >= 0 {
			if (*v)[i*w+col] == new_v {
				i--
				numCols++
			} else {
				break
			}
		}

		i = row + 1
		for i < h {
			if (*v)[i*w+col] == new_v {
				i++
				numCols++
			} else {
				break
			}
		}

		if numCols >= numToWin {
			return new_v, true
		}
	}

	{ // rows
		numRows := 1
		i := col - 1
		for i >= 0 {
			if (*v)[row*w+i] == new_v {
				i--
				numRows++
			} else {
				break
			}
		}

		i = col + 1
		for i < w {
			if (*v)[row*w+i] == new_v {
				i++
				numRows++
			} else {
				break
			}
		}

		if numRows >= numToWin {
			return new_v, true
		}
	}

	{ // left-top diag
		num := 1
		i := row - 1
		j := col - 1
		for i >= 0 && j >= 0 {
			if (*v)[i*w+j] == new_v {
				i--
				j--
				num++
			} else {
				break
			}
		}

		i = row + 1
		j = col + 1
		for i < h && j < w {
			if (*v)[i*w+j] == new_v {
				i++
				j++
				num++
			} else {
				break
			}
		}

		if num >= numToWin {
			return new_v, true
		}
	}

	{ // right-top diag
		num := 1
		i := row - 1
		j := col + 1
		for i >= 0 && j < w {
			if (*v)[i*w+j] == new_v {
				i--
				j++
				num++
			} else {
				break
			}
		}

		i = row + 1
		j = col - 1
		for i < h && j >= 0 {
			if (*v)[i*w+j] == new_v {
				i++
				j--
				num++
			} else {
				break
			}
		}

		if num >= numToWin {
			return new_v, true
		}
	}

	// We generate tie in later stage so we can have a result first.
	if total == w*h {
		return 0, true
	}

	return new_v, false
}
