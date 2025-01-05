all: 
	cd src; make
	cd test; make
	cp ID/* out/
	cp src/Driver.o out
	cp src/Space.c out

install:
	cp src/sys/ne2k.h /usr/include/sys/
	/etc/conf/bin/idinstall -d ne2k
	cd out; /etc/conf/bin/idinstall -a ne2k
	/etc/conf/bin/idbuild

clean:
	cd src; make clean
	cd test; make clean

srcfloppy: clean
	(find * -print | cpio -o; dd if=/dev/zero bs=4096 of=/dev/stdout) | dd if=/dev/stdin bs=1024 count=1200 of=out/sources.img