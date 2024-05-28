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
		b.Draw(os.Stdout)

	},
}

var (
	humanFirst bool
)

func init() {
	GameCmd.Flags().BoolVarP(&humanFirst, "humanfirst", "u", false, "Human starts with black.")
}
