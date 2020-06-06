#!/usr/bin/env bash

#---------------------------------------------------------------------------
# macos x launcher for LisaEm
#
# Copyright (C) 2020 by Ray Arachelian, All Rights Reserved.
# Released under the terms of the GPL v3
#
# This detects the version of macos x and architecture, and finds the latest
# version compiled that will work on it. While I could use lipo to glue 
# PPC32 PPC64, i386, and x86_64, x86_64 has various versions such as for
# macos 10.8, or 10.10, or 10.15. For macos 10.8 latest wxWidget that works
# is 3.0.4, but for 10.15 we can use 3.13, etc., and lipo can't help there
#
#---------------------------------------------------------------------------

MACHINE=$( uname -m )
# x86_64

ME="$0"
#MYDIR="$( dirname ${0})"  # <- this fails when dirname contains spaces
MYDIR="${ME%/*}"
cd "$MYDIR"
if [[ "$?" -ne 0 ]]; then
    osascript <<-EOSCD
    display dialog "Failed to change into directory for $ME, wound up in $(pwd) instead." with title "bug in lisaem.sh launch script"
EOSCD
exit 1
fi


OSVER=$( sw_vers -productVersion        )
OSMAJOR=$( echo $OSVER | cut -d'.' -f1  )
OSMIDDLE=$( printf "%02d" $( echo $OSVER | cut -d'.' -f2 ))

WANT="lisaem-$MACHINE-$OSMAJOR.$OSMIDDLE-wx99999999"
EXEC=""

for lisaem in $( ls -1 lisaem-${MACHINE}-* 2>/dev/null ); do
    [[ "$WANT" == "$lisaem" ]] && export EXEC="$lisaem" # Found exact match for this macos?
    [[ "$lisaem" < "$WANT" ]] && [[ "$lisaem" > "$EXEC" ]] && export EXEC="$lisaem" # find latest executable
done

if  [[ -z "$EXEC" ]]; then
    osascript <<-EndOfScript
    display dialog "I can't find a proper LisaEm executable for your machine ${WANT} in $(pwd) 0=$0" with title "Sorry"
EndOfScript
else
    exec ./${EXEC} $@
fi
