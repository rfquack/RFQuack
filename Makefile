#
# vim: noexpandtab
#
# RFQuack is a versatile RF-hacking tool that allows you to sniff, analyze, and
# transmit data over the air. Consider it as the modular version of the great
# 
# Copyright (C) 2019 Trend Micro Incorporated
# 
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later
# version.
# 
# This program is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.  See the GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc., 51 Franklin
# Street, Fifth Floor, Boston, MA  02110-1301, USA.
#

# Note: make sure that client/ points to the root of https://github.com/rfquack/RFQuack-cli

# Thanks to: https://gist.github.com/mpneuried/0594963ad38e68917ef189b4e6a269db

# HELP
# This will output the help for each task
# thanks to https://marmelab.com/blog/2016/02/29/auto-documented-makefile.html
.PHONY: help

.DEFAULT_GOAL := help

APP_PREFIX := rfquack
APP_NAME := rfquack
APP := $(APP_PREFIX)/$(APP_NAME)
SHELL := /bin/bash
EXAMPLES := $(wildcard examples/*)

help: ## This help.
	@awk 'BEGIN {FS = ":.*?## "} /^[a-zA-Z_-]+:.*?## / {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}' $(MAKEFILE_LIST)

docker-build: ## Build the container
	docker build --progress plain -t $(APP) .

docker-build-nc: ## Build the container without caching
	docker build --progress plain --no-cache -t $(APP) .

docker-stop: ## Stop and remove a running container
	docker stop $(APP_NAME); docker rm $(APP_NAME)

build: ## Build firmware image according to variables set in .env file
	docker run --rm -it --env-file build.env $(APP)

flash: ## Flash firmware image to $PORT
	docker run --rm -it --device=${PORT}:/board --env-file build.env $(APP)

proto: ## Compile protobuf types
	cd "${HOME}/.platformio/lib/Nanopb/generator/proto" ;  make
	cd "src" ; \
	protoc --plugin=protoc-gen-nanopb=${HOME}/.platformio/lib/Nanopb/generator/protoc-gen-nanopb \
		--nanopb_out=./ \
		rfquack.proto \
		--python_out=client/

examples:  ## Compile examples
	for example in $(EXAMPLES); do \
		pio ci -c $$example/platformio.ini -l src/ $$example; \