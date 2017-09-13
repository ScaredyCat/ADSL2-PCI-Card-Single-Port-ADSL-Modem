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

#ifndef __MY_SOCKET_H_
#define __MY_SOCKET_H_

/****************************************************************************/

#define PUBLIC extern

/****************************************************************************/

#include <tools.h>

/****************************************************************************/

#define SERV_ISDNLOG    			"isdnlog"
#define MAX_CLIENTS_LISTEN    5

#define PROT_VERSION					"V0.2"
#define PROT_ELEMENTS					20

#ifndef SERV_PORT
#	define SERV_PORT 20011
#endif

/****************************************************************************/

#define	NO_NEXT_MSG			0
#define	NEXT_MSG				1

/****************************************************************************/

#define NO_MSG				  	0
#define MSG_WHO_IS		  	1  /* Vom Server: String mit Nummer */
#define MSG_CALL_INFO  		2  /* Vom Server: String mit Gebuehren-Info */
#define MSG_ANNOUNCE      3  /* Vom Client: Anmeldung beim Server mit User als String */
#define MSG_ANN_ACC       4  /* Vom Server: Mit Server-Typ z.B. T_ISDN4LINUX */
#define MSG_ANN_REJ       5  /* Vom Server: Ablehnung ohne weitere Info */
#define MSG_NEW_CALLER    6  /* Vom Client: String mit neuen Anrufer-Daten */
#define MSG_CALLER        7  /* Vom Server: String mit Anrufer-Daten */
#define MSG_TOPICS			  8  /* Vom Server: Topics vom aktuellen Gespraech */
#define MSG_NOTICE			  9  /* Vom Server: Uebermitteln von einer Notiz */
#define MSG_THRUPUT_INFO 10  /* Vom Server: Datendurchsatz */
#define MSG_CLOCK_INFO   11  /* Vom Server: Uhrzeit vom Amt */
#define MSG_SERVER       12  /* Vom Server: print_msg Meldungen */
#define MSG_CHANGE_CHAN  13  /* Vom Server: Kanal wurde gewechselt */
#define MSG_VERSION      14  /* Vom Server: Version des Protokolls: "PROT_VERSION" */
#define MSG_CLOSE				 15  /* Die Verbindung wird beendet ohne Parameter, Dummy-Message */
#define MSG_ISDN_CMD		 16  /* Es kann ein Befehl vom Client ausgefuehrt werden */

/****************************************************************************/

#define _MSG_LEN 4
#define _MSG_MSG 2
#define _MSG_2B  2

/****************************************************************************/

#define WF_NOTHING  		0 /* WF : Wait for */
#define WF_ACC		      1
#define WF_CLOSE	      2

/****************************************************************************/

/* Die folgenden Flags stehen im direkten Bezug zu user_access.c:ValidFlags */
#define T_NOTHING				0	/* Unterbau nicht vorhanden */
#define T_I4LCONF				1	/* Unterbau ist isdn4liunx und darf configuriert werden */
#define T_PROTOCOL			2 /* Meldungen vom S0 */
#define T_ADDRESSBOOK		4 /* Es soll das Adressbuch erlaubt werden. */

/* ACHTUNG: Die folgende muss immer Upgedatet werden */
#define T_ALL						6 /* Dieses ist die Summe aller gueltigen Flags. */

/****************************************************************************/

#define NO_SOCKET		-2
#define NO_BIND			-3
#define NO_LISTEN		-4
#define NO_CONNECT	-5
#define NO_MEMORY  	-6
#define NO_HOST  	  -7

/****************************************************************************/

#define C_DELIMITER  '|'

/****************************************************************************/

typedef struct {
  int   len;
  int   used;
	char *buf;
	} buffer;

typedef struct _socket_queue{
  int    descriptor;
	FILE  *fp;
	pid_t  pid;
	int        chan;
	info_args *info_arg;
	int        call_event;
	int    msg;
	int    status;
	int    waitstatus;
	int    servtyp;
	int    input_id;
	char  *f_hostname;
	char  *f_username;
	int (*eval)(struct _socket_queue*);
	buffer restbuf;
	buffer msgbuf;
  } socket_queue;

typedef struct {
char* Number;
char* Alias;
} PhoneNumber;

typedef struct {
char *Company;
char *Street;
char *Country;
char *PLZ;
char *City;
int   NumTel;
PhoneNumber *Tel;
int   NumFax;
PhoneNumber *Fax;
int   NumEmail;
char **Email;
} Addresses;

typedef struct {
char *NName;
char *FName;
int   NumAdr;
Addresses *Adr;
char* Birthday;
/*
time_t Birthday;
*/
} Address;

/****************************************************************************/

#ifdef _SOCKET_C_
#define _EXTERN
#else
#define _EXTERN extern
#endif

_EXTERN int Write(socket_queue* sock);
_EXTERN int Read(socket_queue* sock);
_EXTERN unsigned long stoi (unsigned char* s, int len);
_EXTERN char *itos (unsigned long val, int len);
_EXTERN int add_socket(socket_queue **sock,int new_socket);
_EXTERN int del_socket(socket_queue **sock,int position);
_EXTERN int socket_size(socket_queue *sock);
_EXTERN int msgcpy(socket_queue *sock, char *String, int len);
_EXTERN int init_socket(socket_queue *sock);
_EXTERN int Set_Info_Struct(CALL **Info, char *String);
_EXTERN char *GetHostByAddr(struct sockaddr *Addr);
_EXTERN char *GetHostByName(char *Name);

#undef _EXTERN

/****************************************************************************/

#ifdef _CONNECT_C_
#define _EXTERN
#else
#define _EXTERN extern
#endif

_EXTERN int client_connect(char *name, int port);
_EXTERN int server_connect(struct servent **sp, int port);

#undef _EXTERN

/****************************************************************************/

#ifdef _CONV_ADDRESS_C_
#define _EXTERN
#else
#define _EXTERN extern
#endif

_EXTERN Address* read_address(char* Ptr1);
_EXTERN char* write_address(Address *Ptr);
_EXTERN void free_Address(Address *APtr);

#undef _EXTERN

/****************************************************************************/

#endif  /* __MY_SOCKET_H_*/
