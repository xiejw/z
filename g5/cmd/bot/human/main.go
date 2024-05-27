package human

import (
	"github.com/rs/zerolog/log"
	"github.com/spf13/cobra"
)

var GameCmd = &cobra.Command{
	Use:   "game",
	Short: "Start a game between human and bot",
	Args:  cobra.NoArgs,
	Run: func(cmd *cobra.Command, args []string) {
		log.Info().Msgf("game cmd: %v", humanFirst)
	},
}

var (
	humanFirst bool
)

func init() {
	GameCmd.Flags().BoolVarP(&humanFirst, "humanfirst", "u", false, "Human starts with black.")
}
