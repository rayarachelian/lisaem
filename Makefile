# This is a fake Makefile.  For build environments
# that require ./configure && make.  You really
# should use the build.sh script directly.
build:   src/host/wxui/lisaem_wx.cpp
	./build.sh build 

clean:   src/host/wxui/lisaem_wx.cpp
	./build.sh clean

install: src/host/wxui/lisaem_wx.cpp
	./build.sh build  --install

package: src/host/wxui/lisaem_wx.cpp
	./build.sh build  --package
me:
	
a:
	
sandwich: me a 
	[ `id -u` -ne 0 ] && echo "What? Make it yourself." || echo Okay

love:
	#    "Not war?"

war:
	#    "Not love?"
