module github.com/scratchbird/scratchrobin-cli

go 1.19

require (
    github.com/spf13/cobra v1.7.0
    github.com/spf13/viper v1.15.0
    github.com/stretchr/testify v1.8.4
    gopkg.in/yaml.v3 v3.0.1
    github.com/olekukonko/tablewriter v0.0.5
    github.com/fatih/color v1.15.0
    github.com/mattn/go-sqlite3 v1.14.17
)

// Development dependencies
require (
    github.com/cucumber/godog v0.13.0
    github.com/onsi/ginkgo/v2 v2.11.0
    github.com/onsi/gomega v1.27.8
)
