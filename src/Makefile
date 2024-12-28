all: Driver.o
	

Driver.o: ne2k.o
	ld -r -o Driver.o ne2k.o

ne2k.o: ne2k.c
	cc -I. -c ne2k.c

clean:
	rm *.o
