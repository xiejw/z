package human

import (
	"os"

	"github.com/rs/zerolog/log"
	"github.com/spf13/cobra"

	"g5/go/lib/game"
)

var GameCmd = &cobra.Command{
	Use:   "game",
	Short: "Start a game between human and bot",
	Args:  cobra.NoArgs,
	Run: func(cmd *cobra.Command, args []string) {
		log.Info().Msgf("Start game with human. Cfg: Human first = %v", humanFirst)

		b := game.NewBoard()
		b.NewMove(game.NewPos(1, 2), game.CLR_WHITE)
		b.NewMove(game.NewPos(1, 3), game.CLR_BLACK)
		b.Draw(os.Stdout)

	},
}

var (
	humanFirst bool
)

func init() {
	GameCmd.Flags().BoolVarP(&humanFirst, "humanfirst", "u", false, "Human starts with black.")
}
