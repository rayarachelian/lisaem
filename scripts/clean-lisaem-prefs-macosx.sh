#!/bin/bash

echo "Remove LisaEm Preferences?"

if [[ "$1" == "-y" ]]; then
   export answer="y"
else
   echo -n  "type in y and hit enter y/N: "
   read answer
   export answer
fi

if [[ "$answer" == "y" ]]; then
   echo "Cleaned"
   rm -rf "${HOME}/Library//Preferences/LisaEm Preferences" "${HOME}/Library//Preferences/lisaem.conf" "${HOME}/Library//Preferences/net.sunder.lisaem.plist"
else
   echo "Not cleaned"
fi

