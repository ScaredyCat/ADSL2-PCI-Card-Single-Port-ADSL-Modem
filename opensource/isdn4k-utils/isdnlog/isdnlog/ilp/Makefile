# Makefile

# adjust here (look at /usr/src/linux/Makefile)
CC=gcc -V2.7.2.3

FLAGS := -Wall -DMODULE -D__KERNEL__ -DLINUX -O2

modilp.o:	modilp.c /usr/include/linux/version.h Makefile
	$(CC) $(FLAGS) -c modilp.c

ilp:	ilp.c
	$(CC) -Wall -g -DSTANDALONE ilp.c -o ilp

clean:
	rm *.o *~ ilp

distlean:
	clean

