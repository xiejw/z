package game

type Color int

const (
	CLR_NA Color = iota
	CLR_BLACK
	CLR_WHITE
)

func (c Color) String() string {
	switch c {
	case CLR_NA:
		return "_"
	case CLR_BLACK:
		return "b"
	case CLR_WHITE:
		return "w"
	default:
		panic("unknown color")
	}
}
