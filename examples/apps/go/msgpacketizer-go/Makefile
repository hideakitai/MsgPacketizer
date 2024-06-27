.PHONY: help
help: ## Display this help screen
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-20s\033[0m %s\n", $$1, $$2}'

.PHONY: help-serial
help-serial: ## Display help for cmd/serial/main.go
	go run cmd/serial/main.go -h

.PHONY: help-network
help-network: ## Display help for cmd/network/main.go
	go run cmd/network/main.go -h

.PHONY: go-mod-tidy
go-mod-tidy: ## run "go mod tidy" for all go.mod files in the project
	find . -name 'go.mod' -execdir go mod tidy \;

.PHONY: build
build: ## build all binaries (cmd/serial/main.go and cmd/network/main.go)
	go build -o bin/serial cmd/serial/main.go
	go build -o bin/network cmd/network/main.go

.PHONY: run-serialport
run-serialport: ## run "go run cmd/serial/main.go"
	go run cmd/serial/main.go

.PHONY:
run-network: ## run "go run cmd/network/main.go"
	go run cmd/network/main.go
