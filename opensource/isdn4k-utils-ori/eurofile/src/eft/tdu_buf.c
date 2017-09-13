/* $Id: tdu_buf.c,v 1.1 1999/06/30 17:18:43 he Exp $ */
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
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <tdu_user.h>
#include "tdu.h"
/*
 * parse a byte encoded parameter
 * return values: the parsed byte >= 0;
 *                -1 if length of parameter value is not equal to 1
 */
int tdu_parse_byte(struct tdu_buf * tb)
{
	int len = tb->pn - tb->data, pv = *(tb->data);

	tb->data = tb->pn;
	if( len != 1 ) return -1;
	return pv;
}

/*
 * parse an unsigned integer with explicit length encoding
 * (as used i.e. for length indicator of TDU's)
 * return values: the parsed integer if >= 0
 *                -1 if buffer overflow is detected
 */
long tdu_parse_le(struct tdu_buf * tb)
{
	long len;

	tdu_printf(TDU_LOG_TRC, "tdu_parse_le()\n");
	len = *tb->data;
	tb->data++;
	if( len == 0xff ) {
		if( tb->data+2 > tb->tail ) return -1;
		len = 0x100 * *(tb->data)  +  *(tb->data+1);
		tb->data += 2;
	}
	if( tb->data > tb->tail ) return -1;
	return len;
}

/*
 * parse an unsigned integer. The length of the encoding byte sequence
 * is given implicitly by the delimiting position of the next parameter
 * (as used i.e. for encoding integer TDU parameter values)
 * Returns the parsed integer
 */
long tdu_parse_li(struct tdu_buf * tb)
{
	long len=0;

	while( tb->data < tb->pn ) {
		len *= 0x100;
		len += *(tb->data);
		(tb->data)++;
	};
	return len;
}

/*
 * Parse a byte string type parameter value (delimited by next parameter)
 * and copy it to a destination.
 * Return value: 
 *    The number of bytes copied on success.
 *    -(number of uncopied bytes) if string length exceeds limit.
 */
long tdu_parse_string(struct tdu_buf * tb, unsigned char * dest, long limit)
{
	long len, ret;

	len = tb->pn - tb->data;
	if( len <= limit ){
		ret = len;
	} else {
		ret = len - limit;
		len = limit;
	}
	memcpy(dest,tb->data,len);
	tb->data += len;
	return ret;
}

/*
 * stick a command header in front of a tdu packet
 */
int tdu_add_ci_header(struct tdu_buf * tb, int ci)
{
	int ret;

	ret = tdu_stick_le( tb, tb->tail - tb->data );
	*(--(tb->data)) = ci;
	tb->ci = tb->data;
	return ret+1;
}

/*
 * stick a parameter header in front of a tdu parameter value string
 */
int tdu_add_pi_header(struct tdu_buf * tb, int pi)
{
	int len = tb->pn - tb->data;

	if( len >  254 )
		tdu_printf(TDU_LOG_WARN, "tdu_add_pi_header(): "
			   "parameter len %d > 254\n", len); 
	len = tdu_stick_le( tb, len );
	/* len contains the len of the added header now */
	*(--(tb->data)) = pi;
	tb->pn = tb->data;
	return len+1;
}

/*
 * encode and stick a reason paramater value in front of a buffer
 */
int tdu_add_reason(struct tdu_buf * tb, int in_response_to,
		   int reason, unsigned char * other_reason)
{
	int len;

	if( reason == TDU_RE_OTHER_REASON ) {
		int len = strlen(other_reason);
		if( len > 62 ) len = 62;
		tb->data -= len;
		memcpy( tb->data, other_reason, len );
	}
	if( reason ) 
		*(--tb->data) = reason;
	if( in_response_to ) 
		*(--tb->data) = in_response_to;
	len = tdu_add_pi_header(tb, TDU_PI_RESULT_REASON);
	return len;
}

/*
 * encode and stick a string type parameter in front of a tdu buffer
 */
int tdu_add_string_par(struct tdu_buf * tb, int pi, unsigned char * s
		   , unsigned int len)
{
	if( s ){
		tb->data -= len;
		if(tb->data < tb->head){
			tdu_printf( TDU_LOG_ERR, "string parameter too large "
				    "(%d bytes), buffer overflow\n",len);
			tb->data = tb->pn;
			return 0;
		}
		memcpy( tb->data, s, len );
		len = tdu_add_pi_header(tb, pi/*, tmp*/);
	}
	return len;
}

/*
 * encode and stick a one_byte parameter in front of a tdu buffer
 */
int tdu_add_byte_par(struct tdu_buf * tb, int pi, unsigned char par )
{
	tb->data--;
	*(tb->data) = par;
	return tdu_add_pi_header(tb,pi);
}

/*
 * encode and stick an integer parameter in front of a tdu buffer
 */
int tdu_add_int_par(struct tdu_buf * tb, int pi, unsigned long par )
{
	tdu_stick_li( tb, par);
	return tdu_add_pi_header(tb,pi);
}

/*
 * get next parameter from a tdu buffer and determine position of next
 * parameter.
 * Returns:
 *   The (positive) parameter id on success
 *   a negative number (-TDU_RE_* code) on failure
 */
int tdu_get_next_pi(struct tdu_buf *tb)
{
	int pi;
	long len;

	tdu_printf(TDU_LOG_TRC, "tdu_get_next_pi()\n");
	pi = *tb->data;
	tb->data++;
	len = tdu_parse_le(tb);

	if( len < 0 ) return -TDU_RE_SYNTAX_ERROR;
	tb->pn = tb->data + len;
	if( tb->pn > tb->tail ){
		tb->pn = tb->tail;
		tdu_printf(TDU_LOG_ERR, "param length > remaining packet length\n");
		return -TDU_RE_SYNTAX_ERROR;
	}
	return pi;
}


/*
 * Encode and stick an integer in front of a tdu_buf. The number of digits
 * (base 256) is not encoded explicitly.
 */
int tdu_stick_li( struct tdu_buf * tb, unsigned long i )
{
	int cnt = 0;

	while( i > 0 ){
		tb->data--;
		*tb->data = i % 256;
		i /= 256;
		cnt++;
	};
	if( cnt ) return cnt;

	*(--tb->data) = 0;
	return 1;
}

/*
 * Encode and stick an integer in front of a tdu_buf. The number of digits
 * is encoded explicitly (used i.e. for length indicators of TDU's).
 */
int tdu_stick_le( struct tdu_buf * tb, unsigned long i )
{
	int cnt = 1;

	tb->data--;
	if( i < 255 ){
		*tb->data = i;
	} else {
		*tb->data = i % 256;
		i /= 256;
		tb->data--;
		*tb->data = i;
		tb->data--;
		*tb->data = 255;
		cnt += 2;
	};
	return cnt;
}

/*
 * initialize a tdu buffer (for composing outgoing packets)
 */
void tdu_init_tb( struct tdu_buf * tb )
{
	memset( tb, 0, sizeof(*tb) );
	tb -> tail = tb -> end;
	tb -> data = tb -> tail;
	tb -> pn = tb -> tail;
	tb -> eof = 0;
	tb -> ci = NULL;
	tb -> md = tb -> head + 12;
}

/*
 * Sends out a packet. This implementation assumes an underlaying linux
 * x25 socket with the X25_QBITINCL socket option set.
 */
int tdu_send_packet( struct tdu_buf * tb, struct tdu_fsm * fsm )
{
	int ct = TDU_LOG_OUT;
	char ci = *tb->data;

	if( ci == TDU_CI_T_RESPONSE_NEGATIVE ||
	    ci == TDU_CI_T_P_EXCEPTION ||
	    ci == TDU_CI_T_TRANSFER_REJECT ||
	    ci == TDU_CI_T_ABORT                ){
		ct |= TDU_LOG_OER;
	}
	
	tdu_printf(ct, "SENDING: "); tdu_print_cmd(ct, tb->data, tb->tail,4);
	tb->data[-1] = 0 /* don't set Q-Bit */;
	return write( fsm->socket, tb->data-1, tb->tail - tb->data + 1 ) - 1; 
}	

int tdu_fd_isdnlog = -1;

void tdu_open_isdnlog(char * fn)
{
	tdu_fd_isdnlog = open(fn,O_RDONLY|O_NONBLOCK);
}
/*
 * log any data from /dev/isdnlog 
 */
void tdu_isdnlog()
{
	char isdnlog_buf[5001];
	int n;

	n=read(tdu_fd_isdnlog,isdnlog_buf,5000);
	if( n>0 ){
		isdnlog_buf[n]=0;
		write(2,isdnlog_buf,n);
	}
}

/*
 * Receive a packet from the communication channel.
 * This implementation assumes an underlaying linux
 * x25 socket with the X25_QBITINCL socket option set.
 */
int tdu_recv_packet( struct tdu_buf * tb, struct tdu_fsm * fsm )
{
	int len, QBit;

	/* tb->data = tb->head+3; */
	/* we need to keep plenty of space at the head for the tdu_echo_pos
	   hack */
	tb->data = tb->head+10;
	tdu_printf(TDU_LOG_TRC, "tdu_recv_packet()\n");
	tdu_isdnlog();
	len = read( fsm->socket, tb->data, tb->end - tb->data );
	if( len < 0 ) {
		perror("tdu_recv_packet:read():");
		return -TDU_CI_SOCK_READ_ERROR;
	} else {
		tdu_isdnlog();
		tb->tail = tb->data + len;
		QBit = *tb->data;
		tb->data += 1;

		if( QBit != 0 ){ /* Q-Bit is set */
			/* Assuming (without checking) that an SBV_TPD_END_TC
			 * was received (what else should it be? -:) */
		return -TDU_CI_TC_DISCONNECT;
		}
	return len-1;
	}
}
