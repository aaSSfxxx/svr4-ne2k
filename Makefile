all: 
	cd src; make
	cp ID/* out/
	cp src/Driver.o out
	cp src/Space.c out

install:
	cp src/sys/ne2k.h /usr/include/sys/
	/etc/conf/bin/idinstall -d ne2k
	cd out; /etc/conf/bin/idinstall -a ne2k
	/etc/conf/bin/idbuild

