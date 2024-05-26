package game

// The game is 15x15 (see const.go), so a uint8 is enough to represent the
// position with 16x16.
type Pos uint8

func NewPos(x, y int) Pos {
	return Pos(x*16 + y)
}

func (p Pos) X() int {
	return int(p / 16)
}

func (p Pos) Y() int {
	return int(p % 16)
}
