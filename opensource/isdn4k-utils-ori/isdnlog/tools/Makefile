CC=gcc -m486 -O2 -Wall -DSTANDALONE -I. -I.. -I../isdnlog -I../connect -I../../lib
#CC=gcc -m486 -O2 -g -pg -Wall -DSTANDALONE -I. -I.. -I../isdnlog

all: rate-at

rate: rate.u zone.u country.u holiday.u dest.u telnum.u
	${CC} -lgdbm -o rate rate.u zone.u dest.u holiday.u telnum.u

telnum: rate.u zone.u country.u holiday.u dest.u telnum.o1
	${CC} -lgdbm -o rate rate.u zone.u dest.u holiday.u telnum.o1

AT:	rate-at country
	./rate-at > rate-at.dat
	(cd dest;./pp_rate ../rate-at.dat  ../../rate-at.dat)
	rm rate-at.dat

AT-leo:	rate-at
	./rate-at -leo > rate-at.dat
	(cd dest;./pp_rate ../rate-at.dat  ../../rate-at.dat)
	cp ../rate-at.dat /usr/lib/isdn/rate-at.dat
	rm rate-at.dat

#zone.exe: zone.c zone.h
#	${CC} -DZONETEST -lgdbm -o zone.exe zone.c
# this is built in tools/zone

country: country.c country.h
	${CC} -DCOUNTRYTEST -o country country.c

holiday: holiday.c holiday.h
	${CC} -DHOLITEST -lm -o holiday holiday.c

rate-at: rate-at.c
	${CC} -o rate-at rate-at.c

rate.u: rate.c rate.h holiday.h
	${CC} -c -o rate.u rate.c

zone.u: zone.c zone.h
	${CC} -c -o zone.u zone.c

country.u: country.c country.h
	${CC} -c -o country.u country.c

dest.u: dest.c dest.h
	${CC} -c -o dest.u dest.c

telnum.u: telnum.c telnum.h
	${CC} -c -o telnum.u telnum.c

telnum.o1: telnum.c telnum.h
	${CC} -DTEST_TELNUM -c -o telnum.u telnum.o1

holiday.u: holiday.c holiday.h
	${CC} -c -o holiday.u holiday.c

clean:
	rm -f rate holiday *.{u,o} *~ rate-at
