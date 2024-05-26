// bot tries to play the games by itself or play with human.
package main

import (
	"fmt"
	"os"

	"github.com/rs/zerolog"
	"github.com/rs/zerolog/log"
	"github.com/spf13/cobra"

	_ "g5/go/lib/game"
	_ "github.com/xiejw/y/ann/luna/errors"
)

var rootCmd = &cobra.Command{
	Use:   "bot",
	Short: "bot is the cli to play the g5",
}

var gameCmd = &cobra.Command{
	Use:   "game",
	Short: "Start a game between human and bot",
	Args:  cobra.NoArgs,
	Run: func(cmd *cobra.Command, args []string) {
		log.Info().Msgf("game cmd: %v", humanFirst)
	},
}

func Execute() {
	if err := rootCmd.Execute(); err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
}

var (
	humanFirst bool
)

func init() {
	gameCmd.Flags().BoolVarP(&humanFirst, "humanfirst", "u", false, "Human starts with black.")
	rootCmd.CompletionOptions.DisableDefaultCmd = true
	rootCmd.AddCommand(gameCmd)
}

func main() {
	log.Logger = log.Output(zerolog.ConsoleWriter{Out: os.Stderr})
	Execute()
}
