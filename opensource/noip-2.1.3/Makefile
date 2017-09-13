TGT=noip2
#CC=gcc

PREFIX= 
CONFDIR=${PREFIX}/etc
BINDIR=${PREFIX}/bin

# these defines are for Linux
LIBS=
ARCH=mips-linux

# for Mac OS X and BSD systems that have getifaddr(), uncomment the next line
#ARCH=bsd_with_getifaddrs

# for early BSD systems without getifaddrs(), uncomment the next line
#ARCH=bsd


# for solaris, uncomment the next two lines
# LIBS=-lsocket -lnsl
# ARCH=sun

${TGT}: Makefile ${TGT}.c 
	${CC} -Wall -g -O2 -DPREFIX=\"${PREFIX}\" ${TGT}.c -o ${TGT} ${LIBS}
#	${CC} -Wall -g -O2 ${TGT}.c -o ${TGT} ${LIBS}	

install: ${TGT} 
	if [ ! -d ${BINDIR} ]; then mkdir -p ${BINDIR};fi
	if [ ! -d ${CONFDIR} ]; then mkdir -p ${CONFDIR};fi
	cp ${TGT} ${BINDIR}/${TGT}
	${BINDIR}/${TGT} -C -c /tmp/no-ip2.conf
	mv /tmp/no-ip2.conf ${CONFDIR}/no-ip2.conf

package: ${TGT}
	rm  -f *.bak
	mv ${TGT} binaries/${TGT}-`uname -s`
	cd ..; tar zcvf /tmp/noip-2.1.tgz noip-2.0/*

clean: 
	rm -f *o
	rm -f ${TGT}
