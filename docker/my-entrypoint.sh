#!/bin/sh
set -e

# Compile template
j2 /quack/src/main.cpp.j2 > /quack/src/main.cpp

# Exec cmd
exec "$@"
