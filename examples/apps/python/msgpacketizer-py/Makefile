.PHONY: help
help: ## Display this help screen
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-20s\033[0m %s\n", $$1, $$2}'

.PHONY: run_serialport
run_serialport: ## Run src/serialport.py
	python src/serialport.py

.PHONY: run_network
run_network: ## Run src/network.py
	python src/network.py

.PHONY: export
export: requirements.lock ## Generate requirements.txt from requirements.lock for pip
	cat requirements.lock | sed '/-e file/d' > requirements.txt

requirements.lock: ## Generate requirements.lock if not exists
	rye sync

.PHONY: generate_type_stubs
generate_type_stubs: ## Generate type stubs for cobs and crc8
	stubgen -p cobs -o typings
	stubgen -p crc8 -o typings
