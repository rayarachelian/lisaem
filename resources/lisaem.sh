#!/usr/bin/env bash

#---------------------------------------------------------------------------
# macos x launcher for LisaEm
#
# Copyright (C) 2020 by Ray Arachelian, All Rights Reserved.
# Copyright (C) 2023 by Friends of Ray Arachelian
# Released under the terms of the GPL v3
#
# This detects the version of macos x and architecture, and finds the latest
# version compiled that will work on it.
#
#---------------------------------------------------------------------------

# Prefer arm64, but fall back to x86_64 in the event this is not Apple Silicon
MACHINE=$( ARCHPREFERENCE=arm64,x86_64 arch uname -m )
result=$?
if [ "$result" -ne 0 ]; then
  MACHINE=$( ARCHPREFERENCE=x86_64 arch uname -m )
fi

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

# fallback to x86_64 if we're on M1 but don't have any ARM binaries, but Rosetta2 is installed.
if [[ "$MACHINE" == "arm64" ]]; then
   if [[ -n "$(pkgutil --pkgs 2>/dev/null | grep Rosetta )" ]]; then
      [[ -z "$( ls -1 lisaem-arm64* 2> /dev/null)" && MACHINE="arm64" ]] && export MACHINE="x86_64"
   fi
fi

for lisaem in $( ls -1 lisaem-${MACHINE}-* 2>/dev/null ); do
    [[ "$WANT" == "$lisaem" ]] && export EXEC="$lisaem" # Found exact match for this macos?
    [[ "$lisaem" < "$WANT" ]] && [[ "$lisaem" > "$EXEC" ]] && export EXEC="$lisaem" # find latest executable
done

if  [[ -z "$EXEC" ]]; then
    osascript <<-EndOfScript
    display dialog "I can't find a proper LisaEm executable for your machine ${WANT} in $(pwd) 0=$0" with title "Sorry"
EndOfScript
else


    if [[ "$1" == "--diet" ]]; then
       export DELETED="Keeping $EXEC $( printf '\r\n' )"
       for lisaem in $( ls -1 lisaem-*-* 2>/dev/null ); do
	  [[ "$EXEC" != "$lisaem" ]] && export DELETED="${DELETED}Removed $lisaem $(printf '\r\n')" && rm "$lisaem"
       done
       osascript <<-EndDiet
       display dialog "$DELETED in ${pwd} 0=$0" with title "Diet"
EndDiet
      exit 0
    fi 


    if [[ "$1" == "--list" ]]; then
       export LIST="Will Use: $EXEC $( printf '\r\n\r\n' )"
       for lisaem in $( ls -1 lisaem-*-* 2>/dev/null ); do
	  [[ "$EXEC" != "$lisaem" ]] && export LIST="${LIST}          $lisaem $(printf '\r\n')"
       done
       osascript <<-EndDiet
       display dialog "$LIST in ${pwd} 0=$0" with title "List of LisaEm Binaries Available"
EndDiet
      exit 0
    fi 


    if [[ "$1" == "--clean-preferences" ]]; then
       rm -rf $HOME/Library/Saved\ Application\ State/net.sunder.lisaem.savedState
       rm -f $HOME/Library/Preferences/lisaem*
       rm -f $HOME/Library/Preferences/net.sunder.lisaem.plist
       osascript <<-EndClearPrefs
       display dialog "Preferences for LisaEm were cleaned" with title "Cleaned Preferences"
EndClearPrefs
      exit 0
    fi


    exec ./${EXEC} $@

fi
