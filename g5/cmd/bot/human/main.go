package human

import (
	"github.com/rs/zerolog/log"
	"github.com/spf13/cobra"

	"g5/go/lib/control"
	"g5/go/lib/game"
	"g5/go/lib/policy"
)

var GameCmd = &cobra.Command{
	Use:   "game",
	Short: "Start a game between human and bot",
	Args:  cobra.NoArgs,
	Run:   startHumanGame,
}

var (
	humanFirst bool
)

func init() {
	GameCmd.Flags().BoolVarP(&humanFirst, "humanfirst", "u", false, "Human starts with black.")
}

func startHumanGame(cmd *cobra.Command, args []string) {
	log.Info().Msgf("Start game with human. Cfg: Human first = %v", humanFirst)

	p1 := policy.NewHumanPolicy("black", game.CLR_BLACK)
	p2 := policy.NewHumanPolicy("white", game.CLR_WHITE)

	err := control.StartGame(p1, p2, nil)
	if err != nil {
		log.Panic().Err(err).Msgf("unexpected error")
	}
}
