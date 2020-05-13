#!/bin/sh
set -e

export UUID=$(python  -c 'import uuid; print(uuid.uuid4())')
echo "Installation ID is $UUID"

# Compile template
j2 /quack/src/main.cpp.j2 > /quack/src/main.cpp

if [ -z "$BOARD" ]
then
	echo "No board selected, defaults to HUZZAH32"
	BOARD="HUZZAH32"
fi


platformio run -t upload -e $BOARD
