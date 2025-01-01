package ai

import (
	"github.com/rs/zerolog/log"
	"github.com/spf13/cobra"

	"g5/go/lib/control"
	"g5/go/lib/game"
	"g5/go/lib/policy"
)

var AiCmd = &cobra.Command{
	Use:   "ai",
	Short: "Start a game between ai bots",
	Args:  cobra.NoArgs,
	Run:   startAiGame,
}

func startAiGame(cmd *cobra.Command, args []string) {
	log.Info().Msgf("Start game between ai bots")

	p1 := policy.NewRandomPolicy("r1", game.CLR_BLACK)
	p2 := policy.NewRandomPolicy("r2", game.CLR_WHITE)

	err := control.StartGame(p1, p2, nil)
	if err != nil {
		log.Panic().Err(err).Msgf("unexpected error")
	}
}
