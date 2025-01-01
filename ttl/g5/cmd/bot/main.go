// bot tries to play the games by itself or play with human.
package main

import (
	"fmt"
	"os"

	"github.com/rs/zerolog"
	"github.com/rs/zerolog/log"
	"github.com/spf13/cobra"

	"g5/cmd/bot/ai"
	"g5/cmd/bot/human"
)

var rootCmd = &cobra.Command{
	Use:   "bot",
	Short: "bot is the cli to play the g5",
}

func Execute() {
	if err := rootCmd.Execute(); err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
}

func init() {
	rootCmd.CompletionOptions.DisableDefaultCmd = true
	rootCmd.AddCommand(human.HumanCmd)
	rootCmd.AddCommand(ai.AiCmd)
}

func main() {
	log.Logger = log.Output(zerolog.ConsoleWriter{Out: os.Stderr})
	Execute()
}
