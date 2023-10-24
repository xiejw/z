package board

import "fmt"

// Board for tic tac toe.
type Board struct {
	w, h       int
	total      int
	numToWin   int
	v          []int
	p          Policy
	aiValue    int
	humanValue int
	emptyValue int
}

// Policy is the AI to place next move
//
// Conversion:
//   - Value 1 is the first move,
//   - Value -1 is for the second move.
//   - Value 0 is empty value.
type Policy interface {
	IsAIFirst() bool
	Query(w, h int, v []int) (row, col int)
}

func NewBoard(p Policy) *Board {
	var aiValue int
	if p.IsAIFirst() {
		aiValue = 1
	} else {
		aiValue = -1
	}
	return &Board{
		w:        3,
		h:        3,
		total:    0,
		numToWin: 3,
		v: []int{
			0, 0, 0, 0, 0, 0, 0, 0, 0,
		},
		p:          p,
		aiValue:    aiValue,
		humanValue: aiValue * -1,
		emptyValue: 0,
	}
}

func (b *Board) W() int {
	return b.w
}

func (b *Board) H() int {
	return b.h
}

func (b *Board) IsHumanFirst() bool {
	return !b.p.IsAIFirst()
}

func (b *Board) QueryAI() bool {
	row, col := b.p.Query(b.w, b.h, b.v)
	if !b.IsValid(row, col) {
		panic(fmt.Sprintf("ai's move (row=%v,col=%v) is invalid", row, col))
	}
	return b.place(row, col, b.aiValue)
}

func (b *Board) Place(row, col int) bool {
	if !b.IsValid(row, col) {
		panic(fmt.Sprintf("human's move (row=%v,col=%v) is invalid", row, col))
	}
	return b.place(row, col, b.humanValue)
}

func (b *Board) Get(row, col int) int {
	return b.v[row*b.w+col]
}

func (b *Board) IsValid(row, col int) bool {
	return b.v[row*b.w+col] == b.emptyValue
}

// place puts the stone into row and col. And determines whether the new move
// results a gaming end situation (including tie). If so, return true.
func (b *Board) place(row, col int, v int) bool {
	b.v[row*b.w+col] = v
	b.total++

	if b.total == b.w*b.h {
		return true
	}

	numToWin := b.numToWin

	{ // cols
		numCols := 1
		i := row - 1
		for i >= 0 {
			if b.v[i*b.w+col] == v {
				i--
				numCols++
			} else {
				break
			}
		}

		i = row + 1
		for i < b.h {
			if b.v[i*b.w+col] == v {
				i++
				numCols++
			} else {
				break
			}
		}

		if numCols >= numToWin {
			return true
		}
	}

	{ // rows
		numRows := 1
		i := col - 1
		for i >= 0 {
			if b.v[row*b.w+i] == v {
				i--
				numRows++
			} else {
				break
			}
		}

		i = col + 1
		for i < b.w {
			if b.v[row*b.w+i] == v {
				i++
				numRows++
			} else {
				break
			}
		}

		if numRows >= numToWin {
			return true
		}
	}

	{ // left-top diag
		num := 1
		i := row - 1
		j := col - 1
		for i >= 0 && j >= 0 {
			if b.v[i*b.w+j] == v {
				i--
				j--
				num++
			} else {
				break
			}
		}

		i = row + 1
		j = col + 1
		for i < b.h && j < b.w {
			if b.v[i*b.w+j] == v {
				i++
				j++
				num++
			} else {
				break
			}
		}

		if num >= numToWin {
			return true
		}
	}

	{ // right-top diag
		num := 1
		i := row - 1
		j := col + 1
		for i >= 0 && j < b.w {
			if b.v[i*b.w+j] == v {
				i--
				j++
				num++
			} else {
				break
			}
		}

		i = row + 1
		j = col - 1
		for i < b.h && j >= 0 {
			if b.v[i*b.w+j] == v {
				i++
				j--
				num++
			} else {
				break
			}
		}

		if num >= numToWin {
			return true
		}
	}

	return false
}
