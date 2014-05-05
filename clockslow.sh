#!/bin/bash

if [ "$#" -lt 2 ]
then
    echo 'Clockslow: Trick app to let it think time goes slower or faster, inspired by'
    echo 'Speed Gear.'
    echo "Usage $0 [-v] speed_factor [command]"
    echo ''
    exit 1
fi
if [ -e ./libclockslow.so ]
then
    if [ -n "$LD_LIBRARY_PATH" ]
    then
        NEW_LD_LIBRARY_PATH="$(pwd):$LD_LIBRARY_PATH"
    else
        NEW_LD_LIBRARY_PATH="$(pwd)"
    fi
else
    NEW_LD_LIBRARY_PATH="$LD_LIBRARY_PATH"
fi
if [ "$1" = "-v" ]
then
    export CLOCKSLOW_VERBOSE=1
    shift
fi
if [ -n "$1" ]
then
    export CLOCKSLOW_FACTOR="$1"
    shift
fi
export CLOCKSLOW_START="$(date +%s)"
export LD_LIBRARY_PATH="$NEW_LD_LIBRARY_PATH"
export LD_PRELOAD=libclockslow.so
exec "$@"
exec bash -i
