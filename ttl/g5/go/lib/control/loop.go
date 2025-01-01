package control

import (
	"os"

	"github.com/rs/zerolog/log"

	"github.com/xiejw/y/ann/luna/errors"

	"g5/go/lib/game"
	"g5/go/lib/policy"
)

func StartGame(blackPolicy, whitePolicy policy.Policy, hooks []game.Hook) error {
	if blackPolicy.GetColor() != game.CLR_BLACK {
		log.Panic().Msgf("bad black policy color")
	}
	if whitePolicy.GetColor() != game.CLR_WHITE {
		log.Panic().Msgf("bad white policy color")
	}
	if blackPolicy.GetName() == whitePolicy.GetName() {
		log.Panic().Msgf("dup policy names")
	}

	var p policy.Policy = blackPolicy
	var lastMoveColor game.Color
	var lastPos game.Pos
	var err error
	var foundWinner bool

	b := game.NewBoard()
	for _, h := range hooks {
		err = b.AttachHook(h)
		if err != nil {
			return errors.From(err).EmitNote("failed to attach the hook %v to board", h.GetName())
		}
	}

	b.Draw(os.Stdout)

	for {
		log.Info().Msgf(">>> Next Player (%v): %v", p.GetColor(), p.GetName())
		lastPos = p.GetNextMove(lastPos, lastMoveColor)
		foundWinner, err = b.NewMove(lastPos, p.GetColor())
		if err != nil {
			log.Panic().Err(err).Msgf("unexpected error")
		}

		b.Draw(os.Stdout)

		if foundWinner {
			log.Info().Msgf(">>> Found Winner (%v): %v", p.GetColor(), p.GetName())
			return nil
		}

		switch lastMoveColor {
		case game.CLR_NA:
			p = whitePolicy
			lastMoveColor = game.CLR_BLACK
		case game.CLR_BLACK:
			p = blackPolicy
			lastMoveColor = game.CLR_WHITE
		case game.CLR_WHITE:
			p = whitePolicy
			lastMoveColor = game.CLR_BLACK
		default:
			panic("color not expected")
		}
	}
}
