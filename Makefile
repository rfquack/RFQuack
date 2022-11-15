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
#
# Thanks to https://marmelab.com/blog/2016/02/29/auto-documented-makefile.html

.PHONY: help

.DEFAULT_GOAL := help

APP_PREFIX := rfquack
APP_NAME := rfquack
APP := $(APP_PREFIX)/$(APP_NAME)
RFQ_VOLUME := /tmp/RFQuack
SHELL := /bin/bash
EXAMPLES := $(wildcard examples/*)

help: ## This help.
	@awk 'BEGIN {FS = ":.*?## "} /^[a-zA-Z_-]+:.*?## / {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}' $(MAKEFILE_LIST)

clean: ## Clean build environment
	rm -rf .pio

docker-build-image: ## Build the container
	docker build \
		--progress plain \
		-t $(APP) .

docker-build-image-nc: ## Build the container without caching
	docker build \
		--progress plain \
		--no-cache \
		-t $(APP) .

docker-stop-container: ## Stop and remove a running container
	docker stop $(APP_NAME)
	docker rm $(APP_NAME)

build-in-docker: ## Spawn a shell in Docker container
	docker run \
		--rm -it \
		--volume ${PWD}:$(RFQ_VOLUME) $(APP) \
		make build

flash-via-docker: ## Flash firmware image to $PORT
	docker run \
		--rm -it \
		--volume ${PWD}:$(RFQ_VOLUME) \
		--device=${PORT}:/board $(APP) \
		make flash

build: ## Build firmware image via PlatformIO
	pio run

flash: ## Flash firmware
	pio run -t upload

console: ## Serial console
	pio device monitor

build-ci: ## Run CI-based script on $EXAMPLE and $BOARD
	pio ci \
		-O "build_unflags=-fno-rtti" \
		-O "custom_nanopb_protos=+<lib/RFQuack/src/rfquack.proto>" \
		-O "custom_nanopb_options=--error-on-unmatched" \
		--exclude=lib/RFQuack/lib \
		--lib=lib/RadioLib \
		--lib="." \
		--board $(BOARD) \
		$(EXAMPLE)

build-ci-tmp: ## Run CI-based script on $EXAMPLE and $BOARD (wipe and keep /tmp/build)
	rm -rf /tmp/build
	pio ci \
		-O "build_unflags=-fno-rtti" \
		-O "custom_nanopb_protos=+<lib/RFQuack/src/rfquack.proto>" \
		-O "custom_nanopb_options=--error-on-unmatched" \
		--exclude=lib/RFQuack/lib \
		--lib=lib/RadioLib \
		--lib="." \
		--board $(BOARD) \
		--keep-build-dir \
		--build-dir /tmp/build \
		$(EXAMPLE)
	
proto-dev: ## Compile protobuf types (for dev purposes only, makes lots of assumptions)
	pio pkg install \
		-f -l \
		nanopb/Nanopb
	protoc \
		-I . \
		--plugin=protoc-gen-nanopb=.pio/libdeps/${BOARD}/Nanopb/generator/protoc-gen-nanopb \
		--nanopb_out=. \
		--python_out=client/rfquack \
		src/rfquack.proto

lsd: ## Print list of serial USB devices connected
	pio device list

gen-requirements:  ## Generates requirements.pip
	poetry export > requirements.pip
