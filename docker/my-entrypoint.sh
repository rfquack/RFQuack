#!/bin/sh
set -e

# Compile template
j2 /quack/src/main.cpp.j2 > /quack/src/main.cpp

if [ -z "$BOARD" ]
then
	echo "No board selected, defaults to HUZZAH32"
	BOARD="HUZZAH32"
fi

platformio run -t upload -e $BOARD
