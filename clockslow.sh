#!/bin/bash

# Clockslow -- A tool to trick app to let it think time goes slower or faster
# Copyright (C) 2014  StarBrilliant <m13253@hotmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

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
