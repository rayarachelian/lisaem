#!/usr/bin/env bash
echo -n "4. "     >>/tmp/slot.9304.5.sh.out
( gcc -fPIC -I /mnt/z1/RA/sxfer/lisa_projects/lisaem-irae/lisaem-1.2.7/src/host/wxui/include -I /mnt/z1/RA/sxfer/lisa_projects/lisaem-irae/lisaem-1.2.7/src/lisa/crt/hqx/include -I /mnt/z1/RA/sxfer/lisa_projects/lisaem-irae/lisaem-1.2.7/src/lisa/keyboard/include -I /mnt/z1/RA/sxfer/lisa_projects/lisaem-irae/lisaem-1.2.7/src/printer/imagewriter/include -I /mnt/z1/RA/sxfer/lisa_projects/lisaem-irae/lisaem-1.2.7/src/tools/include -I /mnt/z1/RA/sxfer/lisa_projects/lisaem-irae/lisaem-1.2.7/src/include -I /mnt/z1/RA/sxfer/lisa_projects/lisaem-irae/lisaem-1.2.7/src/lib/libGenerator/src/include -I /mnt/z1/RA/sxfer/lisa_projects/lisaem-irae/lisaem-1.2.7/src/lib/libGenerator/include -I /mnt/z1/RA/sxfer/lisa_projects/lisaem-irae/lisaem-1.2.7/src/lib/libdc42/include -I /mnt/z1/RA/sxfer/lisa_projects/lisaem-irae/lisaem-1.2.7/src/lib/libdc42/src/include -I/usr/lib/x86_64-linux-gnu/wx/include/gtk2-unicode-3.0 -I/usr/include/wx-3.0 -D_FILE_OFFSET_BITS=64 -DWXUSINGDLL -D__WXGTK__ -O2 -ffast-math -fomit-frame-pointer -fno-common -fPIC -c cpu68k-4.c -o ../obj/cpu68k-4.o 2>&1 ) 2>&1       >>/tmp/slot.9304.5.sh.out
if [[ $? -ne 0 ]]; then
   mv /tmp/slot.9304.5.sh.out /tmp/slot.9304.5.sh.failed
   cp slot.9304.5.sh /tmp
   echo "see: /tmp/slot.9304.5.sh /tmp/slot.9304.5.sh.failed for details" >>/tmp/slot.9304.5.sh.failed
   echo "failed command was: gcc -fPIC -I /mnt/z1/RA/sxfer/lisa_projects/lisaem-irae/lisaem-1.2.7/src/host/wxui/include -I /mnt/z1/RA/sxfer/lisa_projects/lisaem-irae/lisaem-1.2.7/src/lisa/crt/hqx/include -I /mnt/z1/RA/sxfer/lisa_projects/lisaem-irae/lisaem-1.2.7/src/lisa/keyboard/include -I /mnt/z1/RA/sxfer/lisa_projects/lisaem-irae/lisaem-1.2.7/src/printer/imagewriter/include -I /mnt/z1/RA/sxfer/lisa_projects/lisaem-irae/lisaem-1.2.7/src/tools/include -I /mnt/z1/RA/sxfer/lisa_projects/lisaem-irae/lisaem-1.2.7/src/include -I /mnt/z1/RA/sxfer/lisa_projects/lisaem-irae/lisaem-1.2.7/src/lib/libGenerator/src/include -I /mnt/z1/RA/sxfer/lisa_projects/lisaem-irae/lisaem-1.2.7/src/lib/libGenerator/include -I /mnt/z1/RA/sxfer/lisa_projects/lisaem-irae/lisaem-1.2.7/src/lib/libdc42/include -I /mnt/z1/RA/sxfer/lisa_projects/lisaem-irae/lisaem-1.2.7/src/lib/libdc42/src/include -I/usr/lib/x86_64-linux-gnu/wx/include/gtk2-unicode-3.0 -I/usr/include/wx-3.0 -D_FILE_OFFSET_BITS=64 -DWXUSINGDLL -D__WXGTK__ -O2 -ffast-math -fomit-frame-pointer -fno-common -fPIC -c cpu68k-4.c -o ../obj/cpu68k-4.o" >>/tmp/slot.9304.5.sh.failed
   echo "from: $(/bin/pwd)" >>/tmp/slot.9304.5.sh.failed
else
   mv /tmp/slot.9304.5.sh.out /tmp/slot.9304.5.sh.done
fi
rm slot.9304.5.sh
exit 0
