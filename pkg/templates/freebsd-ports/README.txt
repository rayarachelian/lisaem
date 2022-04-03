Here's a preliminary port, it won't work until lisaem-1.2.7.tar.xz is released/published
but manually tested. It will ofc need the size, sha256 hash modified to whatever the final
release will be, and whatever patches the FreeBSD 13 ports team deems appropriate.

The directory should also be renamed to /usr/ports/emulators/lisaem from lisaem-1.2.7
but since the existing port for lisaem is for 1.2.6.2, I renamed this one.

Note that this does not use our custom wxWidgets install, which is built from the 
lisaem-1.2.7/scripts/build-wx3.1.5-gtk.sh script, so it may behave a bit different from the
package release of LisaEm 1.2.7, or it might just work fine. Testing will be needed to
hammer that out.


