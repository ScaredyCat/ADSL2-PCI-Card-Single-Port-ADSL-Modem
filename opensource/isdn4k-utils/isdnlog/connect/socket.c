
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

#define _SOCKET_C_

/****************************************************************************/

#include "socket.h"

/****************************************************************************/

int get_msg(socket_queue *sock, buffer *buf);
int bufcat(buffer *s1,buffer *s2, int first, int len);
int initbuf(buffer *buf);

/****************************************************************************/

int Write(socket_queue* sock)
{
  /* ACHTUNG IN DIESER FKT DARF KEIN print_msg() AUFGERUFEN WERDEN !!!!! */
	int RetCode;
	static buffer *buf = NULL;
	char          *Ptr;

	if (sock->descriptor  < 0)
	{
#ifdef DEBUG
	syslog(LOG_DEBUG,"Write: Invalid Descriptor %d",sock->descriptor);
#endif
		return 1;
	}

	if (!buf)
	{
		buf = (buffer*) calloc(1,sizeof(buffer));

		if (!buf)
			return NO_MEMORY;

		if ((RetCode = initbuf(buf)) != 0)
			return RetCode;
	}

	if ((Ptr = itos(sock->msgbuf.used+_MSG_LEN+_MSG_MSG,_MSG_LEN)) == NULL)
		return -1;

	memcpy(buf->buf,Ptr,_MSG_LEN);
	buf->used = _MSG_LEN;

	if ((Ptr = itos(sock->msg,_MSG_MSG)) == NULL)
		return -1;

#ifdef DEBUG
	syslog(LOG_DEBUG,"Write: Message %d:*%s*",sock->msg,sock->msgbuf.buf);
#endif
	sock->msg = NO_MSG;

	memcpy(buf->buf+_MSG_LEN,Ptr,_MSG_MSG);
	buf->used += _MSG_MSG;

	if ((RetCode = bufcat(buf,&(sock->msgbuf),0,sock->msgbuf.used)) != 0)
		return RetCode;

	return write(sock->descriptor,buf->buf,buf->used);
}

/****************************************************************************/

int Read(socket_queue* sock)
{
	int RetCode;
	int SelRet;
	fd_set readmask;
	struct timeval timeout = {0,0};
	static buffer *buf = NULL;


	if (sock->descriptor  < 0)
	{
#ifdef DEBUG
	syslog(LOG_DEBUG,"Write: Invalid Descriptor %d",sock->descriptor);
#endif
		return 1;
	}

	if (sock->restbuf.len > BUF_SIZE && sock->restbuf.used == 0)
	{
		free(sock->restbuf.buf);
		sock->restbuf.buf = NULL;
		initbuf(&(sock->restbuf));
	}

	if (sock->msgbuf.len > BUF_SIZE && sock->msgbuf.used == 0)
	{
		free(sock->msgbuf.buf);
		sock->msgbuf.buf = NULL;
		initbuf(&(sock->msgbuf));
	}
	else
		sock->msgbuf.used = 0;
	
	if (!buf)
	{
		if ((buf = (buffer*) calloc(1,sizeof(buffer))) == NULL)
			return NO_MEMORY;

		if ((RetCode = initbuf(buf)) != 0)
			return RetCode;
	}
	else
		if (buf->len > BUF_SIZE)
		{
			free(buf->buf);
			buf->buf = NULL;
			initbuf(buf);
		}

	FD_ZERO(&readmask);
	FD_SET(sock->descriptor,&readmask);

	while ((SelRet = select(sock->descriptor+1,&readmask,NULL,NULL,&timeout)) > 0 &&
	       FD_ISSET(sock->descriptor,&readmask))
  	if ((RetCode = buf->used = read(sock->descriptor,buf->buf,buf->len)) > 0)
  	{
  		if ((RetCode = bufcat(&(sock->restbuf),buf,0,buf->used)) != 0)
  			return RetCode;
  	}
  	else
  		return -1;

  if (SelRet < 0)
  	return -1;

	if (sock->restbuf.used)
		if ((RetCode = get_msg(sock,buf)) != 0)
			return RetCode;

	return -1; /* Sollte nur ein Wert > 0 zurueckliefern */
}

/****************************************************************************/

int init_socket(socket_queue *sock)
{
	int RetCode;


	if (!sock)
		return -1;

	if ((RetCode = initbuf(&(sock->msgbuf))) != 0)
		return RetCode;

	if ((RetCode = initbuf(&(sock->restbuf))) != 0)
		return RetCode;

	return 0;
}

/****************************************************************************/

int get_msg(socket_queue *sock, buffer *buf)
{
	int len; 
	int RetCode;

	buf->used = 0;
	sock->msgbuf.used = 0;

	if (sock->restbuf.used < _MSG_LEN + _MSG_MSG)
	{
		sock->status = NO_NEXT_MSG;
		sock->msg = NO_MSG;
		return 0;
	}

	len = (int) stoi(sock->restbuf.buf,_MSG_LEN);

	/* printf("stoi %d,%d\n",len,sock->restbuf.used); */

	if (len > sock->restbuf.used)
	{
		sock->status = NO_NEXT_MSG;
		sock->msg = NO_MSG;
		return 0;
	}

	sock->msg = (int) stoi(sock->restbuf.buf+_MSG_LEN,_MSG_MSG);

	if ((RetCode = bufcat(buf,&(sock->restbuf),0,sock->restbuf.used)) != 0)
		return RetCode;

	if ((RetCode = bufcat(&(sock->msgbuf),buf,_MSG_LEN+_MSG_MSG,len - _MSG_LEN - _MSG_MSG)) != 0)
		return RetCode;

	sock->restbuf.used = 0;
	if ((RetCode = bufcat(&(sock->restbuf),buf,len,buf->used - len)) != 0)
		return RetCode;
	
	if (sock->restbuf.used >= _MSG_LEN + _MSG_MSG          &&
	    stoi(sock->restbuf.buf,_MSG_LEN) <= sock->restbuf.used)
		sock->status = NEXT_MSG;
	else
		sock->status = NO_NEXT_MSG;

	return buf->used;
}

/****************************************************************************/

int msgcpy(socket_queue *sock, char *String, int len)
{
	if (sock->msgbuf.len < len)
	{
		sock->msgbuf.len = len;
		if ((sock->msgbuf.buf = (char*) realloc(sock->msgbuf.buf, len * sizeof(char))) == NULL)
			return NO_MEMORY;
	}

	memcpy(sock->msgbuf.buf,String,len);
	sock->msgbuf.used = len;

	return 0;
}

/****************************************************************************/

int bufcat(buffer *s1, buffer *s2, int first, int len)
{
	if (s1->used + len - first > s1->len)
	{
		s1->len = s1->used + len - first;
		s1->buf = (char*) realloc(s1->buf, (s1->used + len - first) * sizeof(char));

		if (s1->buf == NULL)
			return NO_MEMORY;
	}

	memcpy((void*) (s1->buf + s1->used),(void*) (s2->buf + first), len);
	s1->used += len;

	return 0;
}

/****************************************************************************/

int initbuf(buffer *buf)
{
	if (buf && buf->buf == NULL)
	{
		buf->buf = (char*) calloc(BUF_SIZE,sizeof(char));
		buf->len = BUF_SIZE;
		buf->used = 0;
	}

	return buf->buf?0:NO_MEMORY;
}

/****************************************************************************/

unsigned long stoi (unsigned char* s, int len)
{
	unsigned long val  = 0;
	unsigned long Cnt  = 0;


	if (len > 4)
		return 0;

	while (Cnt < len)
		val = (val << 8) + s[Cnt++];

	return val;
}

/****************************************************************************/

char *itos (unsigned long val, int len)
{
	static char s[16];


	if (len > 4)
		return NULL;

	while(len-- > 0)
	{
		s[len] = (char) val % 256;
		val = val >> 8;
	}

	return s;
}

/****************************************************************************/

int add_socket(socket_queue **sock, int new_socket)
{
	int Cnt = 0;


	if ((*sock) == NULL)
	{
		if (!((*sock) = (socket_queue*) calloc(2,sizeof(socket_queue))))
			return NO_MEMORY;

 		(*sock)[0].descriptor = new_socket;
 		(*sock)[1].descriptor = NO_SOCKET;
 	}
 	else
 	{
		Cnt = socket_size((*sock));
 		(*sock) = (socket_queue*) realloc((*sock),sizeof(socket_queue)*(Cnt+2));
 		memset(&((*sock)[Cnt+1]),0,sizeof(socket_queue));
 		(*sock)[Cnt+1].descriptor = NO_SOCKET;

 		(*sock)[Cnt].descriptor = new_socket;
 	}

	if (init_socket(&((*sock)[Cnt])))
 		return NO_MEMORY;

	return 0;
}

/****************************************************************************/

int del_socket(socket_queue **sock, int position)
{
	int Cnt;

	if (*sock == NULL)
		return -1;

	Cnt = socket_size((*sock));
	if (position < 0 || position >= Cnt)
		return -1;

	free((*sock)[position].msgbuf.buf);
	free((*sock)[position].restbuf.buf);
	free((*sock)[position].f_hostname);
	free((*sock)[position].f_username);

	if (position == 0 && Cnt == 1)
	{
		free(*sock);
		*sock = NULL;
		return 0;
	}

	close((*sock)[position].descriptor);
	memcpy(&((*sock)[position]),
		&((*sock)[Cnt-1]),
		sizeof(socket_queue));

 	memset(&((*sock)[Cnt-1]),0,sizeof(socket_queue));
 	(*sock)[Cnt-1].descriptor = NO_SOCKET;

 	(*sock) = (socket_queue*) realloc((*sock),sizeof(socket_queue)*Cnt);
	
	return (*sock)?0:NO_MEMORY;
}

/****************************************************************************/

int socket_size(socket_queue *sock)
{
	int Cnt = 0;

	if (sock != NULL)
		while(sock[Cnt].descriptor != NO_SOCKET)
			Cnt++;

	return Cnt;
}

/****************************************************************************/

int Set_Info_Struct(CALL **Info, char *String)
{
	int Cnt = 0;
	int channel;
	char** Array = String_to_Array(String,C_DELIMITER);

	while(Array[Cnt++]);

	if (Cnt-1 != PROT_ELEMENTS)
	{
		del_Array(Array);
		fprintf(stderr,"Internal error: wrong structure (%d elements), %d expected!\n",Cnt, PROT_ELEMENTS);
		return -1;
	}

	if (*Info == NULL)
		if (((*Info) = (CALL*) calloc(1,sizeof(CALL))) == NULL)
			return NO_MEMORY;

	Cnt = 0;

  channel = atoi(Array[Cnt++]);
  (*Info)->stat = atoi(Array[Cnt++]);
  (*Info)->dialin = atoi(Array[Cnt++]);
	strcpy((*Info)->num[_ME((*Info))],Array[Cnt++]);  /* Meine MSN */
  strcpy((*Info)->alias[_ME((*Info))],Array[Cnt++]);
	strcpy((*Info)->num[_OTHER((*Info))],Array[Cnt++]);
  strcpy((*Info)->vorwahl[_OTHER((*Info))],Array[Cnt++]);
  strcpy((*Info)->rufnummer[_OTHER((*Info))],Array[Cnt++]);
  strcpy((*Info)->alias[_OTHER((*Info))],Array[Cnt++]);
  strcpy((*Info)->area[_OTHER((*Info))],Array[Cnt++]);
  (*Info)->connect = atoi(Array[Cnt++]);
  (*Info)->t_duration = atoi(Array[Cnt++]);
  (*Info)->aoce = atoi(Array[Cnt++]);
  strcpy((*Info)->money,Array[Cnt++]);
  strcpy((*Info)->currency,Array[Cnt++]);
  (*Info)->ibytes = atoi(Array[Cnt++]);
  (*Info)->obytes = atoi(Array[Cnt++]);
  (*Info)->ibps = atof(Array[Cnt++]);
  (*Info)->obps = atof(Array[Cnt++]);
  strcpy((*Info)->msg,Array[Cnt++]); 

	del_Array(Array);
	return channel;
}

/****************************************************************************/

char *GetHostByAddr(struct sockaddr *Addr)
{
	char *RetCode = NULL;
	char *Ptr;
	struct hostent *hp = NULL;
	struct in_addr *In = &((struct sockaddr_in*) Addr)->sin_addr;


	if ((hp = gethostbyaddr((char*) In,sizeof(long int),AF_INET)) != NULL)
	{
		if ((RetCode = (char*) calloc(strlen(hp->h_name)+1,sizeof(char))) ==NULL)
			return NULL;

		strcpy(RetCode,hp->h_name);
	}
	else
	if ((Ptr = inet_ntoa(*In)) != NULL)
	{
		if ((RetCode = (char*) calloc(strlen(Ptr)+1,sizeof(char))) ==NULL)
			return NULL;

		strcpy(RetCode,Ptr);
	}

	return RetCode;
}

/****************************************************************************/

char *GetHostByName(char *Name)
{
	char *RetCode = NULL;
	struct hostent *hp = NULL;


	if ((hp = gethostbyname(Name)) != NULL)
	{
		if ((RetCode = (char*) calloc(strlen(hp->h_name)+1,sizeof(char))) ==NULL)
			return NULL;

		strcpy(RetCode,hp->h_name);
	}

	return RetCode;
}

/****************************************************************************/

