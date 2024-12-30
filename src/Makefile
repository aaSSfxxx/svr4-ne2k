BUILDCFLAGS = -D_KERNEL -O3 -I. -c $(CFLAGS)

all: Driver.o
	

Driver.o: ne2k.o
	ld -r -o Driver.o ne2k.o

ne2k.o: ne2k.c
	cc $(BUILDCFLAGS) ne2k.c

clean:
	rm -f *.o
