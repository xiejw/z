package human

import (
	"os"

	"github.com/rs/zerolog/log"
	"github.com/spf13/cobra"

	"g5/go/lib/game"
	"g5/go/lib/policy"
)

var GameCmd = &cobra.Command{
	Use:   "game",
	Short: "Start a game between human and bot",
	Args:  cobra.NoArgs,
	Run: func(cmd *cobra.Command, args []string) {
		log.Info().Msgf("Start game with human. Cfg: Human first = %v", humanFirst)

		b := game.NewBoard()
		p1 := policy.NewHumanPolicy("black", game.CLR_BLACK)
		p2 := policy.NewHumanPolicy("white", game.CLR_WHITE)

		var p policy.Policy = p1
		var lastMoveColor game.Color
		var lastPos game.Pos
		var err error

		b.Draw(os.Stdout)

		for {
			log.Info().Msgf("->> Next player (%v): %v", p.GetColor(), p.GetName())
			lastPos = p.GetNextMove(lastPos, lastMoveColor)
			_, err = b.NewMove(lastPos, p.GetColor())
			if err != nil {
				log.Panic().Err(err).Msgf("unexpected error")
			}

			b.Draw(os.Stdout)

			switch lastMoveColor {
			case game.CLR_NA:
				p = p2
				lastMoveColor = game.CLR_BLACK
			case game.CLR_BLACK:
				p = p1
				lastMoveColor = game.CLR_WHITE
			case game.CLR_WHITE:
				p = p2
				lastMoveColor = game.CLR_BLACK
			default:
				panic("color not expected")
			}
		}
	},
}

var (
	humanFirst bool
)

func init() {
	GameCmd.Flags().BoolVarP(&humanFirst, "humanfirst", "u", false, "Human starts with black.")
}
