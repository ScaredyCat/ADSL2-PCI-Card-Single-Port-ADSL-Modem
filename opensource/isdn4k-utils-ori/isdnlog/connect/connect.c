/*
 * ISDN accounting for isdn4linux. 
 *
 * Copyright 1996 by Stefan Luethje (luethje@sl-gw.lake.de)
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
 */

/****************************************************************************/

#define _CONNECT_C_

/****************************************************************************/

#include "socket.h"

/****************************************************************************/

int server_connect(struct servent **sp, int port)
{
  int sock;
  struct sockaddr_in server;


	if (!port)
		if ((*sp = getservbyname(SERV_ISDNLOG,"tcp")) != NULL)
  		port = (*sp)->s_port;
  	else
 			port = htons(SERV_PORT);
 	else
 		port = htons (port);

  memset(&server, 0, sizeof(server));
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = port;

  if ((sock = socket(AF_INET,SOCK_STREAM,0)) < 0)
    return NO_SOCKET;

  if (bind(sock,(struct sockaddr*) &server,sizeof(server)) < 0)
    return NO_BIND;

  if (listen(sock,MAX_CLIENTS_LISTEN))
    return NO_LISTEN;

  return sock;
}

/****************************************************************************/

int client_connect(char *name, int port)
{
	int    sock;
	struct sockaddr_in server;
	struct servent   *sp;
	struct hostent   *hp;

	if ((hp = gethostbyname (name)) == NULL)
		return NO_HOST;

	if((sock = socket (AF_INET, SOCK_STREAM, 0)) < 0)
		return NO_SOCKET;

	if (!port)
 		if ((sp = getservbyname(SERV_ISDNLOG,"tcp")) != NULL)
  		port = sp->s_port;
  	else
  		port = htons(SERV_PORT);
 	else
 		port = htons (port);

	server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = port;

	memcpy  ((char *) &server.sin_addr, (char *) hp->h_addr, hp->h_length);

	if (connect (sock, (struct sockaddr *) &server, sizeof (server)) < 0)
		return NO_CONNECT;

	return sock;
}
