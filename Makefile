CC=musl-gcc-x86_32
CFLAGS=-Wall
all:	bi dcfcmd
bi:	bigint.o bi.o
dcfcmd:	bigint.o dcf.o dcfcmd.o sha256.o
clean:	
	rm -f *.o bi dcfcmd
