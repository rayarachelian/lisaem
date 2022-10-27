#!/usr/bin/env bash

#echo      >>/tmp/cc-strip-upx.txt
#date      >>/tmp/cc-strip-upx.txt
#echo "$@" >>/tmp/cc-strip-upx.txt
#echo      >>/tmp/cc-strip-upx.txt

app="$( basename $1 )" # get output name and also command output to run against strip, upx, and hashes
fullapp="${TLD}/bin/$app"
#echo "fullapp: $fullapp"  >>/tmp/cc-strip-upx.txt
#echo "app: $app"  >>/tmp/cc-strip-upx.txt
#echo              >>/tmp/cc-strip-upx.txt

shift

COMPILE="$@"

#echo "COMPILE: $COMPILE"  >>/tmp/cc-strip-upx.txt
#echo                      >>/tmp/cc-strip-upx.txt


flag=""

#${COMPILE} 2>&1 | tee -a /tmp/cc-strip-upx.txt || exit $?
${COMPILE} 2>&1 || exit $?

#env       >>/tmp/cc-strip-upx.txt

COMPILEDSIZE="  Compiled size: $( printf '%8s %s ' $(ls -lh ${fullapp} 2>/dev/null | awk '{print $5}' )) "

if [[ -z "$WITHOUTSTRIP" ]] && [[ -n "$STRIP" ]]; then
   "$STRIP" "${fullapp}" 2>/dev/null
   COMPILEDSIZE="$COMPILEDSIZE stripped: $( printf '%-8s %s ' $(ls -lh ${fullapp}  2>/dev/null | awk '{print $5}' )) "
   flag="yes"
fi

if [[ -z "$WITHOUTUPX" ]] && [[ -n "$UPXCMD" ]]; then
   "${TLD}/bashbuild/upxcmd.sh" "$UPXCMD" "${fullapp}"
   COMPILEDSIZE="$COMPILEDSIZE UPX Size $( printf '%-8s %s ' $(ls -lh ${fullapp}  2>/dev/null | awk '{print $5}' )) "
   flag="yes"
fi

[[ -n "$MD5SUM"  ]] && $MD5SUM  $HASOPTS "${fullapp}" >>hashes.txt 2>/dev/null
[[ -n "$SHA1SUM" ]] && $SHA1SUM $HASOPTS "${fullapp}" >>hashes.txt 2>/dev/null
[[ -n "$SHA256"  ]] && $SHA256  $HASOPTS "${fullapp}" >>hashes.txt 2>/dev/null
echo                                                  >>hashes.txt 2>/dev/null

[[ -n "$flag" ]] && COMPILEDSIZE="$COMPILEDSIZE Final: $( printf '%-8s %s ' $(ls -lh ${fullapp}  2>/dev/null | awk '{print $5}' )) $app"

echo "$COMPILEDSIZE" 1>&2
#echo "COMPILEDSIZE: $COMPILEDSIZE"             >>/tmp/cc-strip-upx.txt
