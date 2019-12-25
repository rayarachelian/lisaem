#!/bin/bash

export UPXCMD="$1"
export file="$2"

"$UPXCMD" -q --test "$file" >/dev/null 2>/dev/null && exit 0
"$UPXCMD" -q --best "$file" >/dev/null 2>/dev/null
exit $?
