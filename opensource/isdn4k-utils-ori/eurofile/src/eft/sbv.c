/* $Id: sbv.c,v 1.1 1999/06/30 17:18:36 he Exp $ */
/*
  Copyright 1997 by Henner Eisen

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
#include <sys/time.h>
#include <unistd.h>
#include <tdu_user.h>
#include "tdu.h"
#include "sbv.h"

/* Wait for a response arriving on an x.25 socket */

static int read_response(int s, char* buf, struct timeval timeout)
{
	int count, selval;
	fd_set rfds;
	
	FD_ZERO(&rfds);
	FD_SET(s, &rfds);

	tdu_printf(TDU_LOG_TRC,"read_response()\n");

	selval = select(s+1, &rfds, NULL, NULL, &timeout);
	if ( selval > 0 ) {
		count = read( s, buf, 1025);
		if ( count < 0 ) {
			perror("read_response: read()");
		} 
	} else if (selval == 0) {
		tdu_printf( TDU_LOG_ERR, "read_response(): timeout\n" );
	} else {
		perror("read_response(): select()");
	}
	return count;
}

/*
 * establish SBV transparent connection.
 */
int tdu_sbv_connect(int s)
{
	int i, ret;
	struct timeval timeout = {5,0};
	char buf[1025];

	/* hard coded SBV_TPD_Begin_TC request (establ. "transport" layer) */
	ret = write( s, "\x01@\x9e\x08\x00", 5);
	if( ret<5 ) {
		perror("tdu_sbv_connect()");
		goto error;
	}

	ret = read_response(s,buf,timeout);
	/* FIXME: we need to analyse the contents for verifying that our
	   request has really been accepted */
	if( ret>0 && *buf /* Q-bit is set */ ) {
		return 0;
	} else if( ret>0 ) {
		tdu_printf(TDU_LOG_ERR, "tdu_sbv_connect(): received response without Q-bit: ");
		for(i=1;i<10;i++)tdu_printf(TDU_LOG_ERR, "%02x ", buf[i] );
	}

error:
	tdu_printf(TDU_LOG_ERR, "tdu_sbv_connect(): request failed\n");
	return -4;
}

/*
 * release SBV transparent connection.
 */
int tdu_sbv_disconnect(int s)
{
	int ret;

	/* hard coded SBV_TPD_End_TC request (release "transport" layer) */

	if( s < 0 ) return -1;
	
	ret = write( s, "\x01@\x9e\x09\x00", 5);
	if( ret<5 ) {
		perror("tdu_sbv_disconnect()");
		tdu_printf(TDU_LOG_ERR, "tdu_sbv_disconnect(): request failed\n");
		return -3;
	}
	return 0;
}

/*
 * accept incoming SBV transparent connection.
 */
int tdu_sbv_accept(int s)
{
	int ret, i;
	unsigned char buf[10];
	
	ret = read( s, buf, 10);
	if( ret<2 ) {
		if( ret < 0 ) perror("tdu_sbv_accept()");
		tdu_printf(TDU_LOG_ERR, "tdu_sbv_accept(): failed to receive indication\n");
		return -3;
	}

	if( buf[0] == 0 ){
		tdu_printf(TDU_LOG_ERR, "tdu_sbv_accept(): received packet without Q-bit: ");
		for(i=1;i<10;i++)tdu_printf(TDU_LOG_ERR, "%02x ", buf[i] );
		return -3;
	}

	if( (ret=write( s, "\x01@\x9e\x08\x02\xd1\x00", 7)) < 7 ){
		if( ret < 0 ) {
			perror("tdu_sbv_accept(): response failed");
		} else {
			tdu_printf(TDU_LOG_ERR,"tdu_sbv_accept(): incomplete write\n");
		}
		return -3;
	}
	return 0;
}
