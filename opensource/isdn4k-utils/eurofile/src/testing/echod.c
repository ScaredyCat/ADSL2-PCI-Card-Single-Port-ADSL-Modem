/* $Id: echod.c,v 1.1 1999/06/30 17:19:18 he Exp $ */
/*
  Copyright 1997 by Henner Eisen
  Some code fragments taken from Jonathan Naylor's x25-based telnetd

    This code is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This code is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/ 

/*

  "Hello world" example of a server listening to an x25 socket and
  echoing back all packets received.

*/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <linux/x25.h>

/* is in net/x25.h, not in the public header file linux/x25.h. Why?*/ 
#ifndef X25_ADDR_LEN
#define X25_ADDR_LEN 16
#endif

int main(int argc, char** argv)
{
	int s, ns, c, foo, i;
	struct sockaddr_x25 sx25 = { AF_X25 };
	int on = 1;
	unsigned char buf[4097];

	if (argc > 2 || argc < 1 ) {
	    perror("usage: x25.echod [ X25_ADDRESS ]");
	    exit(1);
	}

	if (argc == 2) {
		argv++;
		sx25.sx25_family = AF_X25;
		strncpy(sx25.sx25_addr.x25_addr, *argv, X25_ADDR_LEN);
	}

	s = socket(AF_X25, SOCK_SEQPACKET, 0);
	if (s < 0) {
		perror("x25.echod: socket creation failed");;
		exit(1);
	}
	/* append |SO_DEBUG to socket options for x25 debug messages*/
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	/* first byte of every packet to be interpreted as
	   the X.25 Q-bit value */
	setsockopt(s, SOL_X25, X25_QBITINCL, &on, sizeof(on));

	/* Without a bind system call the server will listen on all
	   addresses (unfortunatly, it currently does'nt -- maybe I
	   misunderstood the af_x25 kernel sources).  A dedicated
	   address to listen on may have been
	   supplied by a command line argument. 
	   */ 
	if( argc == 2 ){
		if (bind(s, (struct sockaddr *)&sx25, sizeof sx25) < 0) {
			perror("x25.echod: bind failed");
			exit(1);
		} 
	}
	if (listen(s, 1) < 0) {
		perror("x25.echod: listen failed");
		exit(1);
	}
	foo = sizeof sx25;
	ns = accept(s, (struct sockaddr *)&sx25, &foo);
	if (ns < 0) {
		perror("x25.echod: accept failed");
		exit(1);
	}
	printf("x25.echod: connection accepted\n");
	close(s);
	/* As the Q-Bit is included, a successful read will return
	   at least one byte (even when the x25 packet was empty).
	   */
	while ( (c = read(ns,buf,4097)) > 0 ){
		fprintf( stdout, "x25.echod: packet with %d bytes received, "
			 "Q-bit was %sset\n", c-1, buf[0] ? "" : "not " );
		for(i=1; i<c; i++){
			if( buf[i] < 32 || (buf[i] >= 127 && buf[i] < 160) ){
				printf( "." );
			} else {
				printf( "%c", (int) (buf[i]) );
			}
		};
		printf( "\n" );
		/* echo received packet back to sender */
		if( write( ns, buf, c) != c ){
			if( c < 0 ) {
				perror("x25.echod: write failed");
			} else {
				fprintf(stderr, "x25.echod: incomplete write\n");
			}
			exit(1);
		}
	}; 
	if( c < 0 ) perror("x25.echod: read error");
	printf("x25.echod: finishing\n");
	return 0;
}

