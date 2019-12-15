#!/usr/bin/env bash
# This is an interim build script, goes up one dir and runs build from there.
# place a link to this in any directories which don't have their own build script
cd ..
source ./build.sh $@
