/* $Id: isdn_dwabclib.c,v 1.2 2001/03/01 14:59:15 paul Exp $
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Log: isdn_dwabclib.c,v $
 * Revision 1.2  2001/03/01 14:59:15  paul
 * Various patches to fix errors when using the newest glibc,
 * replaced use of insecure tempnam() function
 * and to remove warnings etc.
 *
 * Revision 1.1  1999/11/07 22:04:05  detabc
 * add dwabc-udpinfo-utilitys in isdnctrl
 *
 */


#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>


#define RXRSZ 1500


static int txrx_udp_socket(char *dest,char *em,char *buf,int bytes)
/*****************************************************************

	sizeof(*em) must be >= 1500 bytes

	returns:

	>= 0  readed bytes from isdn-link-level peer
	< 0	  -errno

*******************************************************************/
{
	struct hostent *he = NULL;
	short po = 0;
	int sock = -1;
	struct sockaddr_in SooooK, *ms;

	ms = &SooooK;

	if(dest == NULL || em == NULL) {

		sprintf(em,"No Destination-Addr or error-pointer defined");
		return(-9999);
	}

	if((he = gethostbyname(dest)) == NULL) {

		sprintf(em,"gethostbyaddr(%s) failt errno %d",dest,errno);
err:;

		if(sock > -1)
			close(sock);

		if(errno < sys_nerr)
			sprintf(em + strlen(em)," (%s)",sys_errlist[errno]);

		return(-errno);
	}

	if((sock = socket(AF_INET,SOCK_DGRAM,0)) < 0) {

		sprintf(em,"socket creat faild errno %d",errno);
		goto err;
	}

	memset((void *)ms,0,sizeof(*ms));
	ms->sin_family = he->h_addrtype;

	errno = 8888;

	for(po = 3999; po >= 0; po--) {

		ms->sin_port = htons(20000+po);

		if(!bind(sock, (struct sockaddr *)ms, sizeof(*ms)))
			break;
	}

	if(po < 0) {

		sprintf(em,"cannot bind socket to (23999-20000)");
		goto err;
	}

	memset((void *)ms,0,sizeof(*ms));
	ms->sin_family = he->h_addrtype;
	ms->sin_port = htons(25001);
	memcpy(&ms->sin_addr.s_addr,*(he->h_addr_list),he->h_length);

	if(sendto(sock,buf,bytes,0,(struct sockaddr *)ms,sizeof(*ms)) != bytes) {

		sprintf(em,"sendto <%s> failt errno %d",dest,errno);
		goto err;
	}

	{
		fd_set rs;
		struct timeval tv;

		memset(&rs,0,sizeof(rs));
		FD_SET(sock,&rs);

		bytes = 0;
		tv.tv_sec = 3;
		tv.tv_usec = 0;

		if((bytes = select(sock+1,&rs,NULL,NULL,&tv)) > 0) {

			if(FD_ISSET(sock,&rs)) {

				if((bytes = read(sock,em,RXRSZ)) < 0) {

					sprintf(em,"read from socket failt errno %d",errno);
					goto err;
				}
			}

		} else if(bytes < 0) {

			sprintf(em,"select failt errno %d",errno);
			goto err;
		}
	}

	close(sock);
	return(bytes);
}


static int do_udp(char *dest,char **errm,int todo)
{
	int r = 0;
	char em[RXRSZ];
	char sb[2];

	sb[0] = sb[1] = todo; 
	todo++;

	if((r = txrx_udp_socket(dest,em,sb,2)) >= 2) {

		if(em[0] == todo && em[1] == todo)
			return(1);

	} else if(r < 0) {

		if(errm != NULL) {

			if((*errm = (char *)malloc(strlen(em)+1)) != NULL)
				strcpy(*errm,em);
		}

		return(-r);
	}

	return(0);
}



int isdn_udp_isisdn(char *dest, char **errm)
{ 
	return(do_udp(dest,errm,0x28));
}

int isdn_udp_online(char *dest, char **errm)
{ 
	return(do_udp(dest,errm,0x30));
}

int isdn_udp_dial(char *dest, char **errm)
{ 
	return(do_udp(dest,errm,0x32));
}

int isdn_udp_hangup(char *dest, char **errm)
{ 
	return(do_udp(dest,errm,0x2a));
}

int isdn_udp_clear_ggau(char *dest, char **errm)
{ 
	return(do_udp(dest,errm,0x11));
}
