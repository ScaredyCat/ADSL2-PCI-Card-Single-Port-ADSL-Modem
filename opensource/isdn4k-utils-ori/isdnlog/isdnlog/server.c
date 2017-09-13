/* $Id: server.c,v 1.7 2002/03/11 16:17:10 paul Exp $
 *
 * ISDN accounting for isdn4linux.
 *
 * Copyright 1996, 1999 by Stefan Luethje (luethje@sl-gw.lake.de)
 * 	     	       and Andreas Kool (akool@isdn4linux.de)
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
 * $Log: server.c,v $
 * Revision 1.7  2002/03/11 16:17:10  paul
 * DM -> EUR
 *
 * Revision 1.6  1999/10/25 18:33:15  akool
 * isdnlog-3.57
 *   WARNING: Experimental version!
 *   	   Please use isdnlog-3.56 for production systems!
 *
 * Revision 1.5  1999/03/20 14:33:15  akool
 * - isdnlog Version 3.08
 * - more tesion)) Tarife from Michael Graw <Michael.Graw@bartlmae.de>
 * - use "bunzip -f" from Franz Elsner <Elsner@zrz.TU-Berlin.DE>
 * - show another "cheapest" hint if provider is overloaded ("OVERLOAD")
 * - "make install" now makes the required entry
 *     [GLOBAL]
 *     AREADIFF = /usr/lib/isdn/vorwahl.dat
 * - README: Syntax description of the new "rate-at.dat"
 * - better integration of "sondernummern.c" from mario.joussen@post.rwth-aachen.de
 * - server.c: buffer overrun fix from Michael.Weber@Post.RWTH-Aachen.DE (Michael Weber)
 *
 * Revision 1.4  1998/09/22 20:59:22  luethje
 * isdnrep:  -fixed wrong provider report
 *           -fixed wrong html output for provider report
 *           -fixed strange html output
 * kisdnlog: -fixed "1001 message window" bug ;-)
 *
 * Revision 1.3  1998/03/08 11:42:58  luethje
 * I4L-Meeting Wuerzburg final Edition, golden code - Service Pack number One
 *
 * Revision 1.2  1997/04/03 22:34:51  luethje
 * splitt the files callerid.conf and ~/.isdn.
 *
 */

/****************************************************************************/

#define  _SERVER_C_

/****************************************************************************/

#include "isdnlog.h"

/****************************************************************************/

static char **Old_Prints = NULL;
static char **Old_Info   = NULL;
static char **Cur_Info   = NULL;
static int Cur_Info_Size = 0;

/****************************************************************************/

static int new_client(int sock);
static int del_Cur_Info(int channel);
static int add_Cur_Info(int channel, char *String);
static int append_Old_Info(char *String);
static char *Get_Address(char *Number);
static int Set_Address(char *String);
static int Exec_Remote_Cmd(char *String);
static int Write_Caller_Message(int sock, CALL *info);
static char *Build_Call_Info(CALL *call, int chan);
static int save_messages(char *String, int channel, int stat);
static int broadcast_from_server(char* Num, int Flag, int Message, char *String);
static int append_Old_Prints(char *String);
static int String_For_Output(char *String);

/****************************************************************************/

extern time_t  cur_time;

/****************************************************************************/

int listening(int port)
{
	int sock;
	struct servent *sp = NULL;


	if ((sock = server_connect(&sp,port)) < 0)
	{
		if (sock == NO_SOCKET)
		{
			print_msg(PRT_NORMAL,"Can not open socket %d: %s\n",sock,strerror(errno));
		}
		else
		if (sock == NO_BIND)
		{
			print_msg(PRT_NORMAL,"Can not bind socket: %s\n",strerror(errno));
		}
		else
		if (sock == NO_LISTEN)
		{
			print_msg(PRT_NORMAL,"Can not start listening: %s\n",strerror(errno));
		}

		return -1;
	}

	if (sp)
	{
		print_msg(PRT_DEBUG_CS,"Got service %s on port %d\n",
			sp->s_name,ntohs(sp->s_port));
	}
	else
	{
		print_msg(PRT_DEBUG_CS,"Got port %d\n", port);
	}

	return sock;
}

/****************************************************************************/

int new_client(int sock)
{
	int Accept;
	int Cnt;
	int RetCode;
	char *Host = NULL;
	static CALL *Info = NULL;


	if ((RetCode = read_user_access()) != 0)
	{
		print_msg(PRT_ERR,"Error while reading file \"%s\"!\n",userfile());
		Exit(24);
	}

	sockets[sock].f_username = (char*) calloc(strlen(sockets[sock].msgbuf.buf)+1,
		sizeof(char));
	strcpy(sockets[sock].f_username,sockets[sock].msgbuf.buf);

	Accept = user_has_access(sockets[sock].f_username,sockets[sock].f_hostname);


	sockets[sock].msg = MSG_VERSION;
	msgcpy(&(sockets[sock]),PROT_VERSION,strlen(PROT_VERSION)+1);

	if (Write(&(sockets[sock])) < 1)
	{
	 	disconnect_client(sock);
		return 0;
	}

	Host = sockets[sock].f_hostname != NULL ? sockets[sock].f_hostname : "unknown";
	print_msg(PRT_DEBUG_CS,"connection from client %s@%s %s\n",
		sockets[sock].f_username,
		Host,
		Accept != -1?"accepted":"rejected");

	if (Accept == -1)
	{ /* Ablehnung der Verbindung */
		sockets[sock].waitstatus = WF_CLOSE;
		sockets[sock].msg = MSG_ANN_REJ;
		sockets[sock].msgbuf.used = 0;

		Write(&(sockets[sock]));

		/* Die naechste Zeile wird nicht gebraucht
		disconnect_client(sock);
		*/
		return 0;
	}
	else
	{ /* Verbindung wird angenommen */
		sockets[sock].msg = MSG_ANN_ACC;
		msgcpy(&(sockets[sock]),itos(Accept,_MSG_2B),_MSG_2B+1);

		if (Write(&(sockets[sock])) < 1)
		{
		 	disconnect_client(sock);
			return 0;
		}
	}

	sockets[sock].waitstatus = WF_NOTHING;

	if (Old_Info != NULL)
		for (Cnt = 0; Cnt < mcalls; Cnt++)
			if (Old_Info[Cnt] != NULL &&
			    Set_Info_Struct(&Info,Old_Info[Cnt]) >= 0 &&
			    User_Get_Message(sockets[sock].f_username,sockets[sock].f_hostname,Info->num[_ME(Info)],0) == 0)
			{
				sockets[sock].msg = MSG_CALL_INFO;
				msgcpy(&(sockets[sock]),Old_Info[Cnt],strlen(Old_Info[Cnt])+1);

				if (Write(&(sockets[sock])) < 1)
				{
				 	disconnect_client(sock);
				 	return 0;
				}
			}

	if (Cur_Info != NULL)
		for (Cnt = 0; Cnt < Cur_Info_Size; Cnt++)
			if (Cur_Info[Cnt] != NULL &&
			    Set_Info_Struct(&Info,Cur_Info[Cnt]) >= 0 &&
			    User_Get_Message(sockets[sock].f_username,sockets[sock].f_hostname,Info->num[_ME(Info)],0) == 0)
			{
				sockets[sock].msg = MSG_CALL_INFO;
				msgcpy(&(sockets[sock]),Cur_Info[Cnt],strlen(Cur_Info[Cnt])+1);

				if (Write(&(sockets[sock])) < 1)
				{
				 	disconnect_client(sock);
				 	return 0;
				}
			}

	if (Old_Prints != NULL && User_Get_Message(sockets[sock].f_username,sockets[sock].f_hostname,NULL,T_PROTOCOL) == 0) {
		for (Cnt = 0; Cnt < xlog; Cnt++)
			if (Old_Prints[Cnt] != NULL)
			{
				sockets[sock].msg = MSG_SERVER;
				msgcpy(&(sockets[sock]),Old_Prints[Cnt],strlen(Old_Prints[Cnt])+1);

				if (Write(&(sockets[sock])) < 1)
				{
				 	disconnect_client(sock);
				 	return 0;
				}
			}
			else
				break;
	}

	return 0;
}

/****************************************************************************/

int disconnect_client(int sock)
{
	char User[SHORT_STRING_SIZE];
	char Host[SHORT_STRING_SIZE];

	if (sockets[sock].f_username == NULL)
		strcpy(User,"unknown");
	else
		strcpy(User,sockets[sock].f_username);

	if (sockets[sock].f_hostname == NULL)
		strcpy(Host,"unknown");
	else
		strcpy(Host,sockets[sock].f_hostname);

	close(sockets[sock].descriptor);

	if (del_socket(&sockets,sock))
		Exit(16);

	print_msg(PRT_DEBUG_CS,"disconnect from client %s@%s!\n",User,Host);
	print_msg(PRT_DEBUG_CS,"I/O error: %s!\n",strerror(errno));

	return 0;
}

/****************************************************************************/

int eval_message (int sock)
{
	long int Value;
	int      Msg;

	do
	{
		if ((Value = Read(&(sockets[sock]))) < 0)
		{
			disconnect_client(sock);
			break;
		}
		else
		{
			Msg = sockets[sock].msg;
			switch(sockets[sock].waitstatus)
			{
				case WF_ACC:
					switch (Msg)
					{
						case MSG_ANNOUNCE:
							new_client(sock);
							break;

						default:
							/* Meldung: Unbekannter Message-Typ Msg */
							print_msg(PRT_ERR,"Unknown Message %d: %s\n",Msg,sockets[sock].msgbuf.buf);
							disconnect_client(sock);
							break;
					}
					break;

				case WF_CLOSE:
					switch (Msg)
					{
						case MSG_CLOSE:
							disconnect_client(sock);
							break;

						default:
							/* Meldung: Unbekannter Message-Typ Msg */
							print_msg(PRT_ERR,"Unknown Message %d: %s\n",Msg,sockets[sock].msgbuf.buf);
							disconnect_client(sock);
							break;
					}
					break;

				default:
					switch (Msg)
					{
						case MSG_ISDN_CMD:
							Exec_Remote_Cmd(sockets[sock].msgbuf.buf);
							break;
						case MSG_NEW_CALLER:
							Set_Address(sockets[sock].msgbuf.buf);
							break;
						default:
							/* Meldung: Unbekannter Message-Typ Msg */
							print_msg(PRT_ERR,"Unknown Message %d: %s\n",Msg,sockets[sock].msgbuf.buf);
							disconnect_client(sock);
							break;
					}
					break;
			}
		}
	} while (sockets[sock].status == NEXT_MSG);


	return 0;
}

/****************************************************************************/

int broadcast_from_server(char* Num, int Flag, int Message, char *String)
{
	int Cnt;

	for(Cnt = first_descr; Cnt < socket_size(sockets); Cnt++)
	{
		if (sockets[Cnt].fp == NULL               &&
		    sockets[Cnt].waitstatus == WF_NOTHING &&
		    User_Get_Message(sockets[Cnt].f_username,sockets[Cnt].f_hostname,Num,Flag) == 0)
		{
			sockets[Cnt].msg = Message;
			msgcpy(&(sockets[Cnt]),String,strlen(String)+1);

			if (Write(&(sockets[Cnt])) < 1)
 				disconnect_client(Cnt);
 		}
	}

	return  0;
}

/****************************************************************************/

int print_from_server(char *String)
{
	int RetCode;
	char NewString[LONG_STRING_SIZE];
	time_t t = time(NULL);
	struct tm *tm_time = localtime(&t);


	tm_time->tm_isdst = 0;

	strftime(NewString,LONG_STRING_SIZE,"%b %d %X ",tm_time);
	strncat(NewString, String, sizeof(NewString) - strlen(NewString) - 1);

	if ((RetCode = String_For_Output(NewString)) < 0)
		return RetCode;

	if ((RetCode = append_Old_Prints(NewString)) < 0)
		return RetCode;

	return broadcast_from_server(NULL,T_PROTOCOL,MSG_SERVER,NewString);
}

/****************************************************************************/

int message_from_server(CALL *call, int chan)
{
	int Cnt;
	int RetCode;
	char *String = Build_Call_Info(call,chan);

	if ((RetCode = String_For_Output(String)) < 0)
		return RetCode;

	if ((RetCode = save_messages(String,chan,call->stat)) < 0)
		return RetCode;

	for(Cnt = first_descr; Cnt < socket_size(sockets); Cnt++)
	{
		if (sockets[Cnt].fp == NULL                &&
		    sockets[Cnt].waitstatus == WF_NOTHING &&
		    User_Get_Message(sockets[Cnt].f_username,sockets[Cnt].f_hostname,call->num[_ME(call)],0) == 0)
		{
			sockets[Cnt].msg = MSG_CALL_INFO;
			msgcpy(&(sockets[Cnt]),String,strlen(String)+1);

			if (Write(&(sockets[Cnt])) < 1)
 				disconnect_client(Cnt);
 			else
 			if (Write_Caller_Message(Cnt,call) < 0)
 				disconnect_client(Cnt);
 		}
	}

	return 0;
}

/****************************************************************************/

char *Build_Call_Info(CALL *call, int chan)
{
	static char RetCode[LONG_STRING_SIZE];

/* Anmerkungen dazu:

     1. ``connect'' kann auch (time_t)0 sein!
     	Dann besteht noch gar keine Verbindung - z.b. weil es gerade
        klingelt, man aber noch nicht abgenommen hat!

     2. Die ``Dauer'' zu uebertragen halte ich fuer unwichtig - bitte
     	korrigiert mich, wenn ich da falsch liege!
        Die Dauer ist doch ``(jetzt) - connect'' ?

     3. Die Einheiten sind:

     	  0         : kostet nix (0130 Gespraech, oder man wird angerufen)
          -1 .. -x  : Gebuehrenimpuls
          1 .. x    : AOCE (Gebuehreninfo am Ende)
*/

      sprintf(RetCode, "%d|%d|%d|%s|%s|%s|%s|%s|%s|%s|%d|%d|%d|%s|%s|%ld|%ld|%g|%g|%s\n",
        chan,                                                                                    /* Kennung: 0 = erste, 1 = zweite laufende Verbindung */
        call->stat,			     	 						       /* Satzart fuer _diese_  Meldung */
        call->dialin,	     	 						       /* 1 = ein Anruf, 0 = man waehlt raus */
        call->num[_ME(call)],            	 					     /* Eigene MSN */
        call->alias[_ME(call)], 	     	 					       /* Alias de eigenen MSN */
        call->num[_OTHER(call)],         	 					       /* Telefonnummer des Anrufers/Angerufenen (wie von der VSt uebermittelt) */
        call->vorwahl[_OTHER(call)],     	 					       /* Vorwahl des Anrufers/Angerufenen */
        call->rufnummer[_OTHER(call)],   	 					       /* Rufnummer des Anrufers/Angerufenen */
        call->alias[_OTHER(call)], 	     	 					       /* Alias des Anrufers/Angerufenen */
        call->area[_OTHER(call)], 	     	 						       /* Ortsnetz des Anrufers/Angerufenen */
        (int)(call->connect?call->connect:cur_time),     /* Beginn (als (time_t) ) */
        (int)(call->connect?cur_time - call->connect:0), /* aktuelle Dauer - in Sekunden seit CONNECT */
        call->aoce,		     	                         /* Einheiten (negativ: laufende Impulse, positiv: AOCE) */
        double2str(abs(call->aoce) * currency_factor, 6, 2, DEB), /* kostet gerade */
        currency_factor ? currency : "EUR", 	       		       	 	      	       /* Waehrung */
        call->ibytes,	     	 						       /* Frank's ibytes */
        call->obytes,	     	 						       /* Frank's obytes */
        call->ibps,									       /* Aktueller Durchsatz INPUT: Bytes/Sekunde */
        call->obps,									       /* Aktueller Durchsatz OUTPUT: Bytes/Sekunde */
		    call->msg);			     	 						       /* Status */

	return RetCode;
}

/****************************************************************************/

int Write_Caller_Message(int sock, CALL *Info)
{
	char *Address = NULL;

	if (Info->num[0][0] == '\0')
		return 0;

	if (User_Get_Message(sockets[sock].f_username,sockets[sock].f_hostname,NULL,T_ADDRESSBOOK) != 0)
		return 0;

	/* Dirty Hack fuer emulation von neuen Anrufern */
	if (Info->stat == STATE_CONNECT)
 	{
 		if ((Address = Get_Address(Info->num[0])) == NULL)
		{
			sockets[sock].msg = MSG_WHO_IS;
			msgcpy(&(sockets[sock]),Info->num[0],strlen(Info->num[0])+1);

			if (Write(&(sockets[sock])) < 1)
			{
 				disconnect_client(sock);
 				return -1;
 			}
 		}
 		else
 		{
			sockets[sock].msg = MSG_CALLER;
			msgcpy(&(sockets[sock]),Address,strlen(Address)+1);

			if (Write(&(sockets[sock])) < 1)
			{
 				disconnect_client(sock);
 				return -1;
 			}
 		}
 	}

	return 0;
}

/****************************************************************************/

char *Get_Address(char *Number)
{
	/* Hier sollte die Adresse zur uebergebenen Telefonummer geliefert werden */
	return NULL;
	return "Burmester:Fred:2::Lohe 4:D:23869:Fischbek:2:04532/123:04532/124:0:0::Elmenhorster Str. 16:D:20000:Hamburg:0:0:1:fred@wo.auch.immer:08.01.68";
}

/****************************************************************************/

int Set_Address(char *String)
{
	/* Hier sollte die Adresse zur uebergebenen Telefonummer geliefert werden */
	print_msg(PRT_DEBUG_CS,"New Address*%s*\n",String);
	return 0;
}

/****************************************************************************/

int Exec_Remote_Cmd(char *String)
{
	/* Hier ein Befehl ausfuehrt */
	print_msg(PRT_DEBUG_CS,"Exec Command *%s*\n",String);
	return 0;
}

/****************************************************************************/

int save_messages(char *String, int channel, int stat)
{
	int RetCode;


	if (stat == STATE_HANGUP)
	{
		if ((RetCode = del_Cur_Info(channel)) < 0)
		{
			print_msg(PRT_DEBUG_CS,"Invalid Channel No. %d\n",channel);
		}

		if ((RetCode = append_Old_Info(String)) < 0)
			return RetCode;
	}
	else
	{
		if ((RetCode = add_Cur_Info(channel,String)) < 0)
			return RetCode;
	}

	return 0;
}

/****************************************************************************/

int del_Cur_Info(int channel)
{
	if (Cur_Info == NULL || Cur_Info[channel] == NULL ||
	    channel >= Cur_Info_Size || channel < 0         )
		return -1;

	free(Cur_Info[channel]);
	Cur_Info[channel] = NULL;
	return 0;
}

/****************************************************************************/

int append_Old_Prints(char *String)
{
	int Cnt = 0;

	if (xlog < 1)
		return 0;

	if (Old_Prints == NULL)
		if ((Old_Prints = (char**) calloc(xlog,sizeof(char*))) == NULL)
			return NO_MEMORY;


	while(Cnt < xlog && Old_Prints[Cnt] != NULL)
		Cnt++;

	if (Cnt == xlog)
	{
		Cnt = 1;

		if (Old_Prints[0] != NULL)
			free(Old_Prints[0]);

		while(Cnt < xlog)
		{
			Old_Prints[Cnt-1] = Old_Prints[Cnt];
			Cnt++;
		}

		Cnt = xlog - 1;
	}

	if ((Old_Prints[Cnt] = (char*) calloc(strlen(String)+1,sizeof(char))) == NULL)
		return NO_MEMORY;
	else
		strcpy(Old_Prints[Cnt],String);

	return 0;
}

/****************************************************************************/

int append_Old_Info(char *String)
{
	int Cnt = 0;


	if (mcalls < 1)
		return 0;

	if (Old_Info == NULL)
		if ((Old_Info = (char**) calloc(mcalls,sizeof(char*))) == NULL)
			return NO_MEMORY;


	while(Cnt < mcalls && Old_Info[Cnt] != NULL)
		Cnt++;

	if (Cnt == mcalls)
	{
		Cnt = 1;
		free(Old_Info[0]);

		while(Cnt < mcalls)
		{
			Old_Info[Cnt-1] = Old_Info[Cnt];
			Cnt++;
		}

		Cnt = mcalls - 1;
	}

	if ((Old_Info[Cnt] = (char*) calloc(strlen(String)+1,sizeof(char))) == NULL)
		return NO_MEMORY;
	else
		strcpy(Old_Info[Cnt],String);

	return 0;
}

/****************************************************************************/

int add_Cur_Info(int channel, char *String)
{
	char **Ptr;


	if (channel < 0)
		return 0;

	if (Cur_Info == NULL)
		if ((Cur_Info = (char**) calloc(channel + 1,sizeof(char*))) == NULL)
			return NO_MEMORY;
		else
			Cur_Info_Size = channel + 1;
	else
	if (channel >= Cur_Info_Size)
	{
		if ((Ptr = (char**) calloc(channel + 1,sizeof(char*))) == NULL)
			return NO_MEMORY;

		memcpy(Ptr,Cur_Info,Cur_Info_Size*sizeof(char*));
		Cur_Info_Size = channel + 1;
		free(Cur_Info);
		Cur_Info = Ptr;
	}

	if (Cur_Info[channel] != NULL)
		free(Cur_Info[channel]);

	if ((Cur_Info[channel] = (char*) calloc(strlen(String)+1,sizeof(char))) == NULL)
		return NO_MEMORY;
	else
		strcpy(Cur_Info[channel],String);

	return 0;
}

/****************************************************************************/

int String_For_Output(char* String)
{
	char *Ptr;
	char *CurPtr;
	char  NewString[LONG_STRING_SIZE];

	strcpy(NewString,String);
	CurPtr = NewString;
	String[0] = '\0';

	while ((Ptr = strtok(CurPtr,"\n\r")) != NULL)
	{
		CurPtr = NULL;
		strcat(String,Ptr);
	}

	return 0;
}

/****************************************************************************/

int change_channel(int old_chan, int new_chan)
{
	int RetCode = 0;
	int Cnt;
	char tmp_string[SHORT_STRING_SIZE];
	char *String = NULL;
	static CALL *Info = NULL;


	print_msg(PRT_DEBUG_CS,"Will change channel from %d to %d\n",old_chan,new_chan);

	if (old_chan < 0 || old_chan >= Cur_Info_Size ||
	    new_chan < 0 || Cur_Info[old_chan] == NULL  )
		return -1;

	if (old_chan == new_chan)
		return 0;

 	if ((RetCode = Set_Info_Struct(&Info,Cur_Info[old_chan])) < 0)
		return RetCode;

 	if ((String = Build_Call_Info(Info, new_chan)) == NULL)
		return -1;

	if ((RetCode = del_Cur_Info(old_chan)) != 0)
		return RetCode;

	if ((RetCode = add_Cur_Info(new_chan,String)) != 0)
		return RetCode;

	print_msg(PRT_DEBUG_CS,"Change channel from %d to %d\n",old_chan,new_chan);

	for(Cnt = first_descr; Cnt < socket_size(sockets); Cnt++)
	{
		if (sockets[Cnt].waitstatus == WF_NOTHING &&
	      User_Get_Message(sockets[Cnt].f_username,sockets[Cnt].f_hostname,Info->num[_ME(Info)],0) == 0)
		{
			sockets[Cnt].msg = MSG_CHANGE_CHAN;
			sprintf(tmp_string,"%d%c%d",old_chan,C_DELIMITER,new_chan);
			msgcpy(&(sockets[Cnt]),tmp_string,strlen(tmp_string)+1);

			if (Write(&(sockets[Cnt])) < 1)
			 	disconnect_client(Cnt);
		}
	}

	return 0;
}

/****************************************************************************/

