/* $Id: tdud.c,v 1.1 1999/06/30 17:19:21 he Exp $ */
/*
  Copyright 1998 by Henner Eisen

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

  server listening to an x25 socket. packets received are encapsulated 
  in T_Response_Positive CI and echoed back. This allows for some very
  simple local eftp testing.
*/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <linux/x25.h>
#include "../eft/include/tdu_user.h"
#include "../eft/tdu.h"

/* is in net/x25.h, not in the public header file linux/x25.h. Why?*/ 
#ifndef X25_ADDR_LEN
#define X25_ADDR_LEN 16
#endif

int main(int argc, char** argv)
{
	int s, ns, c, foo, i, ret, ci;
	struct sockaddr_x25 sx25 = { AF_X25 };
	int on = 1;
	unsigned char buf[4097];
	struct tdu_fsm fsm[1];
	struct tdu_buf tb[1];

	if (argc > 2 || argc < 1 ) {
	    perror("usage: tdud [ X25_ADDRESS ]");
	    exit(1);
	}

	if (argc == 2) {
		argv++;
		sx25.sx25_family = AF_X25;
		strncpy(sx25.sx25_addr.x25_addr, *argv, X25_ADDR_LEN);
	}

	s = socket(AF_X25, SOCK_SEQPACKET, 0);
	if (s < 0) {
		perror("tdud: socket creation failed");;
		exit(1);
	}
	/* append |SO_DEBUG to socket options for x25 debug messages*/
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	/* first byte of every packet to be interpreted as
	   the X.25 Q-bit value */
	setsockopt(s, SOL_X25, X25_QBITINCL, &on, sizeof(on));

	/* Without a bind system call the server will listen on all
	   addresses (unfortunatly, it currently does'nt -- maybe I
	   misunderstood the af_x25.c kernel sources).  A dedicated
	   address to listen on may have been
	   supplied by a command line argument. 
	   */ 
	if( argc == 2 ){
		if (bind(s, (struct sockaddr *)&sx25, sizeof sx25) < 0) {
			perror("tdud: bind failed");
			exit(1);
		} 
	}
	if (listen(s, 1) < 0) {
		perror("tdud: listen failed");
		exit(1);
	}
	foo = sizeof sx25;
	ns = accept(s, (struct sockaddr *)&sx25, &foo);
	if (ns < 0) {
		perror("tdud: accept failed");
		exit(1);
	}
	printf("tdud: connection accepted\n");
	close(s);
	/* This specifies the amount of (Debugging) output printed to stderr*/
	/* tdu_stderr_mask = TDU_LOG_FH | TDU_LOG_REW | TDU_LOG_ERR; */
	tdu_stderr_mask = TDU_LOG_ERR | TDU_LOG_IER | TDU_LOG_HASH | TDU_LOG_TMP;
	/* for maximum amount of debugging output use */  
	   tdu_stderr_mask = -1;
	   /*	   */

	fsm->socket = ns;
	fsm->idle.initiator = 0;
	/*	tdu_init_master(fsm); */
	tdu_init_slave(fsm);
	tdu_initial_set_idle(fsm);

	c = read(ns,buf,4097);
	fprintf( stdout, "tdud: packet with %d bytes received, "
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
			perror("tdud: write failed");
		} else {
			fprintf(stderr, "tdud: incomplete write\n");
		}
		exit(1);
	}
	
	while ( (ret=tdu_recv_packet(tb,fsm)>0) ) {
		ci = *tb->data;
		tb->ci = tb->data;

		tdu_printf( -1, "tdud: RECEIVED: "); 
		tdu_print_cmd( -1, tb->ci, tb->tail, 4);

		tb->data++;
	        tb->pn = tb->data+3;
		tdu_parse_le(tb);

		tb->p0 = tb->data;
		tb->pn = tb->p0;
		if( ci == TDU_CI_T_WRITE )
			tb->tail = tb->data + 2 + *(tb->data+1);

		tdu_add_reason(tb,ci,0,0);
		tdu_add_ci_header(tb,TDU_CI_T_RESPONSE_POSITIVE);

		tdu_send_packet(tb,fsm);
	};
	printf("tdud: exiting\n");
	return 0;
}
