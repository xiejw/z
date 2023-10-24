// Package drawing provides common func to play board game in turns.
//
// It has termbox like text-based user interfaces so human can just use arrows
// and space to place stones.
package drawing

import (
	"log"
	"os"

	"github.com/gdamore/tcell/v2"
)

// Board defines a board game to be interactive with AI.
type Board interface {
	W() int             // width of board
	H() int             // height of board
	IsHumanFirst() bool // if true, human goes first.

	Get(row, col int) int      // get current value >0 for 'x' <0 for '0' =0 for '.'
	IsValid(row, col int) bool // true if valid position

	QueryAI() bool           // ask ai to move.  return true if game ends
	Place(row, col int) bool // place for human. return true if game ends
}

// Plays until game ended. One turn for human and one turn for AI.
func BoardLoop(title string, b Board) {
	w, h := b.W(), b.H()

	// Initialize screen
	scr, err := tcell.NewScreen()
	if err != nil {
		log.Fatalf("%+v", err)
	}
	if err := scr.Init(); err != nil {
		log.Fatalf("%+v", err)
	}
	scr.SetStyle(defStyle)
	scr.Clear()

	// Draw initial box
	widthToPlot := 2*w - 1 + 3 + 3 // 3 is margins
	if widthToPlot < len(title)+2 {
		widthToPlot = len(title) + 2
	}
	heightToPlot := h + 1 + 2 + 1
	drawBox(scr, 1, 1, 1+widthToPlot+1, 1+heightToPlot, defStyle, boxStyle, title)

	boardXPos := 1 + 4
	boardYPos := 1 + 3

	msgYPos := 2 + heightToPlot + 1 // 1 is gap

	curCol, curRow := 0, 0

	msg := &errors{}
	msg.Reset("Press C to reset")

	refreshBoard := func() {
		msgToDisplay, changedSinceLastTime := msg.GetMsgAndClear()
		if changedSinceLastTime {
			drawBox(scr, 1, msgYPos, 1+1+errMsgLen, msgYPos+2, defStyle, boxStyle, msgToDisplay)
		}
		drawBoard(scr, boardXPos, boardYPos, curCol, curRow, defStyle, curStyle, w, h, b)
	}

	// Event loop
	quit := func() {
		scr.Fini()
		os.Exit(0)
	}

	endGameFn := func() {
		// Switch the message
		msg.Reset(msgGameEnded)
	}

	// In case panic, scr must be reset.
	defer func() {
		if r := recover(); r != nil {
			scr.Fini()
			panic(r)
		}
	}()

	gameEnded := false            // if true, no new stone (either human or ai)
	humanTurn := b.IsHumanFirst() // if false, will ask ai to move

	if !humanTurn {
		msg.SetMsg(msgAITurn)
	}

	refreshBoard()

	for {

		// Update screen
		scr.Show()

		// If AI turn, then go with AI and skip human turn for this iteration.
		if !gameEnded && !humanTurn {
			gameEnded = b.QueryAI()
			humanTurn = true
			if gameEnded {
				endGameFn()
			}
			refreshBoard()
			continue
		}

		// Now human turn.

		// Poll event
		ev := scr.PollEvent()

		// Process event
		switch ev := ev.(type) {
		case *tcell.EventResize:
			scr.Sync()
		case *tcell.EventKey:
			if ev.Key() == tcell.KeyEscape || ev.Key() == tcell.KeyCtrlC {
				quit()
			} else if ev.Key() == tcell.KeyCtrlL {
				scr.Sync()
			} else if ev.Key() == tcell.KeyLeft || ev.Rune() == 'h' {
				if curCol > 0 {
					curCol--
				}
				refreshBoard()
			} else if ev.Key() == tcell.KeyRight || ev.Rune() == 'l' {
				if curCol < w-1 {
					curCol++
				}
				refreshBoard()
			} else if ev.Key() == tcell.KeyUp || ev.Rune() == 'k' {
				if curRow > 0 {
					curRow--
				}
				refreshBoard()
			} else if ev.Key() == tcell.KeyDown || ev.Rune() == 'j' {
				if curRow < h-1 {
					curRow++
				}
				refreshBoard()
			} else if ev.Rune() == ' ' {
				// Only respond whhen game has not yet ended.
				if !gameEnded {
					if b.IsValid(curRow, curCol) {
						gameEnded = b.Place(curRow, curCol)
						if gameEnded {
							endGameFn()
						} else {
							msg.SetMsg(msgAITurn)
							humanTurn = false
						}
						refreshBoard()
					} else {
						msg.SetMsg(errInvalidPos)
						refreshBoard()
					}
				}
			}
		}
	}
}

// Messages displayed by the message box. All messages should be exactly 16
// characters line (including trailing spaces).
const (
	errMsgLen = 16

	//// ruler ///// 0123456789abcdef
	errInvalidPos = "invalid pos     "
	msgGameEnded  = "game end  ctrl-c"
	msgAITurn     = "ai turn ... wait"
)

// Styles displayed by the boxes and text
var (
	defStyle = tcell.StyleDefault.Background(tcell.ColorReset).Foreground(tcell.ColorReset)
	curStyle = tcell.StyleDefault.Background(tcell.ColorGreen).Foreground(tcell.ColorBlack)
	boxStyle = tcell.StyleDefault.Foreground(tcell.ColorWhite).Background(tcell.ColorNavy)
)

// drawText prints text in the box between x1 and x2 (inclusive) and between y1
// and y2 (inclusive).
func drawText(scr tcell.Screen, x1, y1, x2, y2 int, style tcell.Style, text string) {
	row := y1
	col := x1
	for _, r := range []rune(text) {
		scr.SetContent(col, row, r, nil, style)
		col++
		if col > x2 {
			row++
			col = x1
		}
		if row > y2 {
			break
		}
	}
}

// drawBox prints a box with boarder defines by x1, x2, y1, y2 (inclusive) and a
// title bar provided as text.
func drawBox(scr tcell.Screen, x1, y1, x2, y2 int, textStyle, boxStyle tcell.Style, text string) {
	if y2 < y1 {
		y1, y2 = y2, y1
	}
	if x2 < x1 {
		x1, x2 = x2, x1
	}

	// Draw borders
	for col := x1; col <= x2; col++ {
		scr.SetContent(col, y1, tcell.RuneHLine, nil, boxStyle)
		scr.SetContent(col, y2, tcell.RuneHLine, nil, boxStyle)
	}
	for row := y1 + 1; row < y2; row++ {
		scr.SetContent(x1, row, tcell.RuneVLine, nil, boxStyle)
		scr.SetContent(x2, row, tcell.RuneVLine, nil, boxStyle)
	}

	// Only draw corners if necessary
	if y1 != y2 && x1 != x2 {
		scr.SetContent(x1, y1, tcell.RuneULCorner, nil, boxStyle)
		scr.SetContent(x2, y1, tcell.RuneURCorner, nil, boxStyle)
		scr.SetContent(x1, y2, tcell.RuneLLCorner, nil, boxStyle)
		scr.SetContent(x2, y2, tcell.RuneLRCorner, nil, boxStyle)
	}

	drawText(scr, x1+1, y1+1, x2-1, y2-1, textStyle, text)
}

// drawBoard prints the board b from starting position (x,y).
//
// In addition, the cursor is specified by (curx, cury) and printed with
// curStyle.
func drawBoard(scr tcell.Screen, x, y, curx, cury int, style, curStyle tcell.Style, w, h int, b Board) {
	for i := 0; i < h; i++ {
		for j := 0; j < w; j++ {
			v := b.Get(i, j)

			col := x + 2*j
			row := y + i

			sty := style
			if j == curx && i == cury {
				sty = curStyle
			}

			if v > 0 {
				scr.SetContent(col, row, 'x', nil, sty)
			} else if v < 0 {
				scr.SetContent(col, row, 'o', nil, sty)
			} else {
				scr.SetContent(col, row, '.', nil, sty)
			}
		}
	}
}

// The errors provides an abstraction to track message to display. It will be
// reset defaultMsg afterwards.
type errors struct {
	msg        string
	lastMsg    string
	defaultMsg string
}

// Reset resets the all messages and defaultMsg.
func (e *errors) Reset(defaultMsg string) {
	e.msg = ""
	e.lastMsg = "not same"
	e.defaultMsg = defaultMsg
}

// GetMsgAndClear gets the current message and clears it to defaultMsg.
func (e *errors) GetMsgAndClear() (msg string, changed bool) {
	changed = e.msg != e.lastMsg
	e.lastMsg = e.msg
	msg = e.msg
	e.msg = "" // reset

	if msg == "" {
		msg = e.defaultMsg
	}
	return msg, changed
}

// SetMsg sets the next message to be displayed.
//
// msg must have exactly errMsgLen length.
func (e *errors) SetMsg(msg string) {
	if len(msg) != errMsgLen {
		panic("msg should be exactly errMsgLen long " + msg)
	}
	e.msg = msg
}
