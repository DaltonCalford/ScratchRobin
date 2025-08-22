package main

import (
    "os"

    "github.com/spf13/cobra"
)

var rootCmd = &cobra.Command{
    Use:   "scratchrobin-cli",
    Short: "ScratchRobin Command Line Interface",
    Long: `A command line interface for managing ScratchBird databases.
Complete documentation is available at https://scratchrobin.org/docs/cli`,
    Version: "0.1.0",
}

var (
    // Global flags
    verbose bool
    config  string
    host    string
    port    int
    database string
    user    string
    password string
)

func init() {
    rootCmd.PersistentFlags().BoolVarP(&verbose, "verbose", "v", false, "verbose output")
    rootCmd.PersistentFlags().StringVarP(&config, "config", "c", "", "config file (default is $HOME/.scratchrobin/config.yaml)")
    rootCmd.PersistentFlags().StringVarP(&host, "host", "H", "localhost", "database server host")
    rootCmd.PersistentFlags().IntVarP(&port, "port", "p", 5432, "database server port")
    rootCmd.PersistentFlags().StringVarP(&database, "database", "d", "", "database name")
    rootCmd.PersistentFlags().StringVarP(&user, "user", "U", "", "database user")
    rootCmd.PersistentFlags().StringVarP(&password, "password", "W", "", "database password (prompt if not specified)")

    // Add subcommands
    rootCmd.AddCommand(connectCmd)
    rootCmd.AddCommand(queryCmd)
    rootCmd.AddCommand(schemaCmd)
    rootCmd.AddCommand(backupCmd)
    rootCmd.AddCommand(restoreCmd)
    rootCmd.AddCommand(statusCmd)
    rootCmd.AddCommand(configCmd)
}

func main() {
    if err := rootCmd.Execute(); err != nil {
        os.Exit(1)
    }
}
