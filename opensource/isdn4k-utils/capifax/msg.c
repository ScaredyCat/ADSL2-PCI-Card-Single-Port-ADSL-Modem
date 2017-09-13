/* $Id: msg.c,v 1.2 1998/10/23 12:50:58 fritz Exp $
 *
 * CAPI message logging (mainly for debugging).
 * This stuff is based heavily on AVM's CAPI-adk for linux.
 *
 * This program is free software; you can redistribute it and/or modify          * it under the terms of the GNU General Public License as published by          * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *                                                                               * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Log: msg.c,v $
 * Revision 1.2  1998/10/23 12:50:58  fritz
 * Added RCS keywords and GPL notice.
 *
 */
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>

#include <linux/capi.h>
#include "capi20.h"

typedef enum {
    CAPI_PROTOCOL_HEADER,   /* occurs only once at CAPI_PROTOCOL_INIT */
    CAPI_PROTOCOL_MSG,      /* output is a CAPI Message */
    CAPI_PROTOCOL_TXT,      /* output is text caused by CAPI_PROTOCOL_TEXT */
} CAPI_PROTOCOL_TYP;

typedef struct {
    int typ;
    unsigned off;
} _cdef;

#define _CBYTE	       1
#define _CWORD	       2
#define _CDWORD        3
#define _CSTRUCT       4
#define _CMSTRUCT      5
#define _CEND	       6

static _cdef cdef[] = {
    /*00*/{_CEND},
    /*01*/{_CEND},
    /*02*/{_CEND},
    /*03*/{_CDWORD,     (unsigned)(unsigned long)&((_cmsg *)0)->adr.adrController},
    /*04*/{_CMSTRUCT,   (unsigned)(unsigned long)&((_cmsg *)0)->AdditionalInfo},
    /*05*/{_CSTRUCT,    (unsigned)(unsigned long)&((_cmsg *)0)->B1configuration},
    /*06*/{_CWORD,      (unsigned)(unsigned long)&((_cmsg *)0)->B1protocol},
    /*07*/{_CSTRUCT,    (unsigned)(unsigned long)&((_cmsg *)0)->B2configuration},
    /*08*/{_CWORD,      (unsigned)(unsigned long)&((_cmsg *)0)->B2protocol},
    /*09*/{_CSTRUCT,    (unsigned)(unsigned long)&((_cmsg *)0)->B3configuration},
    /*0a*/{_CWORD,      (unsigned)(unsigned long)&((_cmsg *)0)->B3protocol},
    /*0b*/{_CSTRUCT,    (unsigned)(unsigned long)&((_cmsg *)0)->BC},
    /*0c*/{_CSTRUCT,    (unsigned)(unsigned long)&((_cmsg *)0)->BChannelinformation},
    /*0d*/{_CMSTRUCT,   (unsigned)(unsigned long)&((_cmsg *)0)->BProtocol},
    /*0e*/{_CSTRUCT,    (unsigned)(unsigned long)&((_cmsg *)0)->CalledPartyNumber},
    /*0f*/{_CSTRUCT,    (unsigned)(unsigned long)&((_cmsg *)0)->CalledPartySubaddress},
    /*10*/{_CSTRUCT,    (unsigned)(unsigned long)&((_cmsg *)0)->CallingPartyNumber},
    /*11*/{_CSTRUCT,    (unsigned)(unsigned long)&((_cmsg *)0)->CallingPartySubaddress},
    /*12*/{_CDWORD,     (unsigned)(unsigned long)&((_cmsg *)0)->CIPmask},
    /*13*/{_CDWORD,     (unsigned)(unsigned long)&((_cmsg *)0)->CIPmask2},
    /*14*/{_CWORD,      (unsigned)(unsigned long)&((_cmsg *)0)->CIPValue},
    /*15*/{_CDWORD,     (unsigned)(unsigned long)&((_cmsg *)0)->Class},
    /*16*/{_CSTRUCT,    (unsigned)(unsigned long)&((_cmsg *)0)->ConnectedNumber},
    /*17*/{_CSTRUCT,    (unsigned)(unsigned long)&((_cmsg *)0)->ConnectedSubaddress},
    /*18*/{_CDWORD,     (unsigned)(unsigned long)&((_cmsg *)0)->Data},
    /*19*/{_CWORD,      (unsigned)(unsigned long)&((_cmsg *)0)->DataHandle},
    /*1a*/{_CWORD,      (unsigned)(unsigned long)&((_cmsg *)0)->DataLength},
    /*1b*/{_CSTRUCT,    (unsigned)(unsigned long)&((_cmsg *)0)->FacilityConfirmationParameter},
    /*1c*/{_CSTRUCT,    (unsigned)(unsigned long)&((_cmsg *)0)->Facilitydataarray},
    /*1d*/{_CSTRUCT,    (unsigned)(unsigned long)&((_cmsg *)0)->FacilityIndicationParameter},
    /*1e*/{_CSTRUCT,    (unsigned)(unsigned long)&((_cmsg *)0)->FacilityRequestParameter},
    /*1f*/{_CSTRUCT,    (unsigned)(unsigned long)&((_cmsg *)0)->FacilityResponseParameters},
    /*20*/{_CWORD,      (unsigned)(unsigned long)&((_cmsg *)0)->FacilitySelector},
    /*21*/{_CWORD,      (unsigned)(unsigned long)&((_cmsg *)0)->Flags},
    /*22*/{_CDWORD,     (unsigned)(unsigned long)&((_cmsg *)0)->Function},
    /*23*/{_CSTRUCT,    (unsigned)(unsigned long)&((_cmsg *)0)->HLC},
    /*24*/{_CWORD,      (unsigned)(unsigned long)&((_cmsg *)0)->Info},
    /*25*/{_CSTRUCT,    (unsigned)(unsigned long)&((_cmsg *)0)->InfoElement},
    /*26*/{_CDWORD,     (unsigned)(unsigned long)&((_cmsg *)0)->InfoMask},
    /*27*/{_CWORD,      (unsigned)(unsigned long)&((_cmsg *)0)->InfoNumber},
    /*28*/{_CSTRUCT,    (unsigned)(unsigned long)&((_cmsg *)0)->Keypadfacility},
    /*29*/{_CSTRUCT,    (unsigned)(unsigned long)&((_cmsg *)0)->LLC},
    /*2a*/{_CSTRUCT,    (unsigned)(unsigned long)&((_cmsg *)0)->ManuData},
    /*2b*/{_CDWORD,     (unsigned)(unsigned long)&((_cmsg *)0)->ManuID},
    /*2c*/{_CSTRUCT,    (unsigned)(unsigned long)&((_cmsg *)0)->NCPI},
    /*2d*/{_CWORD,      (unsigned)(unsigned long)&((_cmsg *)0)->Reason},
    /*2e*/{_CWORD,      (unsigned)(unsigned long)&((_cmsg *)0)->Reason_B3},
    /*2f*/{_CWORD,      (unsigned)(unsigned long)&((_cmsg *)0)->Reject},
    /*30*/{_CSTRUCT,    (unsigned)(unsigned long)&((_cmsg *)0)->Useruserdata},
};

static unsigned char *cpars[] = {
    /*00*/ 0,
    /*01 ALERT_REQ*/            (unsigned char*)"\x03\x04\x0c\x28\x30\x1c\x01\x01",
    /*02 CONNECT_REQ*/          (unsigned char*)"\x03\x14\x0e\x10\x0f\x11\x0d\x06\x08\x0a\x05\x07\x09\x01\x0b\x29\x23\x04\x0c\x28\x30\x1c\x01\x01",
    /*03*/ 0,
    /*04 DISCONNECT_REQ*/       (unsigned char*)"\x03\x04\x0c\x28\x30\x1c\x01\x01",
    /*05 LISTEN_REQ*/           (unsigned char*)"\x03\x26\x12\x13\x10\x11\x01",
    /*06*/ 0,
    /*07*/ 0,
    /*08 INFO_REQ*/             (unsigned char*)"\x03\x0e\x04\x0c\x28\x30\x1c\x01\x01",
    /*09 FACILITY_REQ*/         (unsigned char*)"\x03\x20\x1e\x01",
    /*0a SELECT_B_PROTOCOL_REQ*/ (unsigned char*)"\x03\x0d\x06\x08\x0a\x05\x07\x09\x01\x01",
    /*0b CONNECT_B3_REQ*/       (unsigned char*)"\x03\x2c\x01",
    /*0c*/ 0,
    /*0d DISCONNECT_B3_REQ*/    (unsigned char*)"\x03\x2c\x01",
    /*0e*/ 0,
    /*0f DATA_B3_REQ*/          (unsigned char*)"\x03\x18\x1a\x19\x21\x01",
    /*10 RESET_B3_REQ*/         (unsigned char*)"\x03\x2c\x01",
    /*11*/ 0,
    /*12*/ 0,
    /*13 ALERT_CONF*/           (unsigned char*)"\x03\x24\x01",
    /*14 CONNECT_CONF*/         (unsigned char*)"\x03\x24\x01",
    /*15*/ 0,
    /*16 DISCONNECT_CONF*/      (unsigned char*)"\x03\x24\x01",
    /*17 LISTEN_CONF*/          (unsigned char*)"\x03\x24\x01",
    /*18 MANUFACTURER_REQ*/     (unsigned char*)"\x03\x2b\x15\x22\x2a\x01",
    /*19*/ 0,
    /*1a INFO_CONF*/            (unsigned char*)"\x03\x24\x01",
    /*1b FACILITY_CONF*/        (unsigned char*)"\x03\x24\x20\x1b\x01",
    /*1c SELECT_B_PROTOCOL_CONF*/ (unsigned char*)"\x03\x24\x01",
    /*1d CONNECT_B3_CONF*/      (unsigned char*)"\x03\x24\x01",
    /*1e*/ 0,
    /*1f DISCONNECT_B3_CONF*/   (unsigned char*)"\x03\x24\x01",
    /*20*/ 0,
    /*21 DATA_B3_CONF*/         (unsigned char*)"\x03\x19\x24\x01",
    /*22 RESET_B3_CONF*/        (unsigned char*)"\x03\x24\x01",
    /*23*/ 0,
    /*24*/ 0,
    /*25*/ 0,
    /*26 CONNECT_IND*/          (unsigned char*)"\x03\x14\x0e\x10\x0f\x11\x0b\x29\x23\x04\x0c\x28\x30\x1c\x01\x01",
    /*27 CONNECT_ACTIVE_IND*/   (unsigned char*)"\x03\x16\x17\x29\x01",
    /*28 DISCONNECT_IND*/       (unsigned char*)"\x03\x2d\x01",
    /*29*/ 0,
    /*2a MANUFACTURER_CONF*/    (unsigned char*)"\x03\x2b\x15\x22\x2a\x01",
    /*2b*/ 0,
    /*2c INFO_IND*/             (unsigned char*)"\x03\x27\x25\x01",
    /*2d FACILITY_IND*/         (unsigned char*)"\x03\x20\x1d\x01",
    /*2e*/ 0,
    /*2f CONNECT_B3_IND*/       (unsigned char*)"\x03\x2c\x01",
    /*30 CONNECT_B3_ACTIVE_IND*/ (unsigned char*)"\x03\x2c\x01",
    /*31 DISCONNECT_B3_IND*/    (unsigned char*)"\x03\x2e\x2c\x01",
    /*32*/ 0,
    /*33 DATA_B3_IND*/          (unsigned char*)"\x03\x18\x1a\x19\x21\x01",
    /*34 RESET_B3_IND*/         (unsigned char*)"\x03\x2c\x01",
    /*35 CONNECT_B3_T90_ACTIVE_IND*/ (unsigned char*)"\x03\x2c\x01",
    /*36*/ 0,
    /*37*/ 0,
    /*38 CONNECT_RESP*/         (unsigned char*)"\x03\x2f\x0d\x06\x08\x0a\x05\x07\x09\x01\x16\x17\x29\x04\x0c\x28\x30\x1c\x01\x01",
    /*39 CONNECT_ACTIVE_RESP*/  (unsigned char*)"\x03\x01",
    /*3a DISCONNECT_RESP*/      (unsigned char*)"\x03\x01",
    /*3b*/ 0,
    /*3c MANUFACTURER_IND*/     (unsigned char*)"\x03\x2b\x15\x22\x2a\x01",
    /*3d*/ 0,
    /*3e INFO_RESP*/            (unsigned char*)"\x03\x01",
    /*3f FACILITY_RESP*/        (unsigned char*)"\x03\x20\x1f\x01",
    /*40*/ 0,
    /*41 CONNECT_B3_RESP*/      (unsigned char*)"\x03\x2f\x2c\x01",
    /*42 CONNECT_B3_ACTIVE_RESP*/ (unsigned char*)"\x03\x01",
    /*43 DISCONNECT_B3_RESP*/   (unsigned char*)"\x03\x01",
    /*44*/ 0,
    /*45 DATA_B3_RESP*/         (unsigned char*)"\x03\x19\x01",
    /*46 RESET_B3_RESP*/        (unsigned char*)"\x03\x01",
    /*47 CONNECT_B3_T90_ACTIVE_RESP*/ (unsigned char*)"\x03\x01",
    /*48*/ 0,
    /*49*/ 0,
    /*4a*/ 0,
    /*4b*/ 0,
    /*4c*/ 0,
    /*4d*/ 0,
    /*4e MANUFACTURER_RESP*/    (unsigned char*)"\x03\x2b\x15\x22\x2a\x01",
};

#define TYP (cdef[cmsg->par[cmsg->p]].typ)

#define byteTLcpy(x,y)        *(_cbyte *)(x)=*(_cbyte *)(y);
#define wordTLcpy(x,y)        *(_cword *)(x)=*(_cword *)(y);
#define dwordTLcpy(x,y)       memcpy(x,y,4);
#define structTLcpy(x,y,l)    memcpy (x,y,l)
#define structTLcpyovl(x,y,l) memmove (x,y,l)

#define byteTRcpy(x,y)        *(_cbyte *)(y)=*(_cbyte *)(x);
#define wordTRcpy(x,y)        *(_cword *)(y)=*(_cword *)(x);
#define dwordTRcpy(x,y)       memcpy(y,x,4);
#define structTRcpy(x,y,l)    memcpy (y,x,l)
#define structTRcpyovl(x,y,l) memmove (y,x,l)

static unsigned command_2_index (unsigned c, unsigned sc) {
    if (c & 0x80) c = 0x9+(c&0x0f);
    else if (c<=0x0f) ;
    else if (c==0x41) c = 0x9+0x1;
    else if (c==0xff) c = 0x00;
    return (sc&3)*(0x9+0x9)+c;
}

#define OFF (((char *)cmsg)+cdef[cmsg->par[cmsg->p]].off)

static void jumpcstruct (_cmsg *cmsg) {
    unsigned layer;
    for (cmsg->p++,layer=1; layer;) {
	assert (cmsg->p);
	cmsg->p++;
	switch (TYP) {
	    case _CMSTRUCT:
		layer++;
		break;
	    case _CEND:
		layer--;
		break;
	}
    }
}

static char *pnames[] = {
    /*00*/0,
    /*01*/0,
    /*02*/0,
    /*03*/"Controller/PLCI/NCCI",
    /*04*/"AdditionalInfo",
    /*05*/"B1configuration",
    /*06*/"B1protocol",
    /*07*/"B2configuration",
    /*08*/"B2protocol",
    /*09*/"B3configuration",
    /*0a*/"B3protocol",
    /*0b*/"BC",
    /*0c*/"BChannelinformation",
    /*0d*/"BProtocol",
    /*0e*/"CalledPartyNumber",
    /*0f*/"CalledPartySubaddress",
    /*10*/"CallingPartyNumber",
    /*11*/"CallingPartySubaddress",
    /*12*/"CIPmask",
    /*13*/"CIPmask2",
    /*14*/"CIPValue",
    /*15*/"Class",
    /*16*/"ConnectedNumber",
    /*17*/"ConnectedSubaddress",
    /*18*/"Data",
    /*19*/"DataHandle",
    /*1a*/"DataLength",
    /*1b*/"FacilityConfirmationParameter",
    /*1c*/"Facilitydataarray",
    /*1d*/"FacilityIndicationParameter",
    /*1e*/"FacilityRequestParameter",
    /*1f*/"FacilityResponseParameters",
    /*20*/"FacilitySelector",
    /*21*/"Flags",
    /*22*/"Function",
    /*23*/"HLC",
    /*24*/"Info",
    /*25*/"InfoElement",
    /*26*/"InfoMask",
    /*27*/"InfoNumber",
    /*28*/"Keypadfacility",
    /*29*/"LLC",
    /*2a*/"ManuData",
    /*2b*/"ManuID",
    /*2c*/"NCPI",
    /*2d*/"Reason",
    /*2e*/"Reason_B3",
    /*2f*/"Reject",
    /*30*/"Useruserdata",
};

static char *mnames[] = {
    0,
    "ALERT_REQ",
    "CONNECT_REQ",
    0,
    "DISCONNECT_REQ",
    "LISTEN_REQ",
    0,
    0,
    "INFO_REQ",
    "FACILITY_REQ",
    "SELECT_B_PROTOCOL_REQ",
    "CONNECT_B3_REQ",
    0,
    "DISCONNECT_B3_REQ",
    0,
    "DATA_B3_REQ",
    "RESET_B3_REQ",
    0,
    0,
    "ALERT_CONF",
    "CONNECT_CONF",
    0,
    "DISCONNECT_CONF",
    "LISTEN_CONF",
    "MANUFACTURER_REQ",
    0,
    "INFO_CONF",
    "FACILITY_CONF",
    "SELECT_B_PROTOCOL_CONF",
    "CONNECT_B3_CONF",
    0,
    "DISCONNECT_B3_CONF",
    0,
    "DATA_B3_CONF",
    "RESET_B3_CONF",
    0,
    0,
    0,
    "CONNECT_IND",
    "CONNECT_ACTIVE_IND",
    "DISCONNECT_IND",
    0,
    "MANUFACTURER_CONF",
    0,
    "INFO_IND",
    "FACILITY_IND",
    0,
    "CONNECT_B3_IND",
    "CONNECT_B3_ACTIVE_IND",
    "DISCONNECT_B3_IND",
    0,
    "DATA_B3_IND",
    "RESET_B3_IND",
    "CONNECT_B3_T90_ACTIVE_IND",
    0,
    0,
    "CONNECT_RESP",
    "CONNECT_ACTIVE_RESP",
    "DISCONNECT_RESP",
    0,
    "MANUFACTURER_IND",
    0,
    "INFO_RESP",
    "FACILITY_RESP",
    0,
    "CONNECT_B3_RESP",
    "CONNECT_B3_ACTIVE_RESP",
    "DISCONNECT_B3_RESP",
    0,
    "DATA_B3_RESP",
    "RESET_B3_RESP",
    "CONNECT_B3_T90_ACTIVE_RESP",
    0,
    0,
    0,
    0,
    0,
    0,
    "MANUFACTURER_RESP"
};

static void (*signal)(char *, CAPI_PROTOCOL_TYP, CAPI_MESSAGE);
static char *buf=0, *p=0;
/*-------------------------------------------------------*/
void bufprint (char *fmt, ...) {
    va_list f;
    va_start (f,fmt);
    vsprintf (p,fmt,f);
    va_end (f);
    p+=strlen(p);
}

static void printstructlen (_cbyte *m, unsigned len) {
    unsigned hex = 0;
    for (;len;len--,m++)
	if (isalnum (*m) | *m == ' ') {
	    if (hex)
		bufprint (">");
	    bufprint ("%c", *m);
	    hex = 0;
	}
	else {
	    if (!hex)
		bufprint ("<%02x", *m);
	    else
		bufprint (" %02x", *m);
	    hex = 1;
	}
    if (hex)
	bufprint (">");
}

static void printstruct (_cbyte *m) {
    unsigned len;
    if (m[0] != 0xff) {
	len = m[0];
	m+=1;
    }
    else {
	len = ((_cword *)(m+1))[0];
	m+=3;
    }
    printstructlen (m, len);
}

/*-------------------------------------------------------*/
#define NAME (pnames[cmsg->par[cmsg->p]])

static void PROTOCOL_MESSAGE_2_PARS (_cmsg *cmsg, int level) {
    for (;TYP != _CEND; cmsg->p++) {
	int slen = 29+3-level;
	int i;

	bufprint ("  ");
	for (i=0; i<level-1; i++) bufprint (" ");

	switch (TYP) {
	    case _CBYTE:
                bufprint ("%-*s = 0x%02x\n", slen, NAME, *(_cbyte *)(cmsg->m+cmsg->l));
		cmsg->l++;
		break;
	    case _CWORD:
                bufprint ("%-*s = 0x%04x\n", slen, NAME, *(_cword *)(cmsg->m+cmsg->l));
		cmsg->l+=2;
		break;
	    case _CDWORD:
		if (strcmp(NAME,"Data")==0) {
		    bufprint ("%-*s = ", slen, NAME);
		    printstructlen ((_cbyte *)*(_cdword *)(cmsg->m+cmsg->l),
				   *(_cword *)(cmsg->m+cmsg->l+sizeof(_cdword)));
		    bufprint ("\n");
		}
		else
                    bufprint ("%-*s = 0x%08lx\n", slen, NAME, *(_cdword *)(cmsg->m+cmsg->l));
		cmsg->l+=4;
		break;
	    case _CSTRUCT:
		bufprint ("%-*s = ", slen, NAME);
		if (cmsg->m[cmsg->l]=='\0')
		    bufprint ("default");
		else
		    printstruct (cmsg->m+cmsg->l);
		bufprint ("\n");
		if (cmsg->m[cmsg->l] != 0xff)
		    cmsg->l+= 1+ cmsg->m[cmsg->l];
		else
		    cmsg->l+= 3+ *(_cword *)(cmsg->m+cmsg->l+1);

		break;

	    case _CMSTRUCT:
		/*----- Metastruktur 0 -----*/
		if (cmsg->m[cmsg->l] == '\0') {
		    bufprint ("%-*s = default\n", slen, NAME);
		    cmsg->l++;
		    jumpcstruct (cmsg);
		}
		else {
		    char *name = NAME;
		    unsigned _l = cmsg->l;
		    bufprint ("%-*s\n", slen, name);
		    cmsg->l = (cmsg->m+_l)[0] == 255 ? cmsg->l+3 : cmsg->l+1;
		    cmsg->p++;
		    PROTOCOL_MESSAGE_2_PARS (cmsg, level+1);
		}
		break;
	}
    }
}
/*-------------------------------------------------------*/
#ifndef NOCLOCK
#include <time.h>
#endif
void CAPI_PROTOCOL_INIT(char *_buf, void (*_signal)(char *, CAPI_PROTOCOL_TYP, CAPI_MESSAGE)) {

#ifndef NOCLOCK
    time_t  ltime;
    char    *date;
#endif

    buf = _buf;
    signal = _signal;
    p = buf; p[0]=0;
    if (buf==NULL||signal==NULL) return;

    bufprint ("+---------------------------------------------------------------------\n");
    bufprint ("|   COMMON-ISDN-API   Development Kit      AVM-Berlin      Version 2.0\n");
    bufprint ("|\n");
#ifndef NOCLOCK
    time (&ltime);
    date = ctime (&ltime);
    date[24]='\0';
    bufprint ("|%*s\n", 69/2 + strlen(date)/2, date);
#endif
    bufprint ("+---------------------------------------------------------------------\n");
    (*signal)(buf, CAPI_PROTOCOL_HEADER, NULL);
    p = buf;
    p[0]=0;
}

/*-------------------------------------------------------*/
void CAPI_PROTOCOL_TEXT (char *fmt, ...) {
    va_list f;
    if (buf==NULL||signal==NULL) return;
    p = buf; p[0]=0;
    va_start (f,fmt);
    vsprintf (p,fmt,f);
    va_end (f);
    (*signal)(p, CAPI_PROTOCOL_TXT, NULL);
}

/*-------------------------------------------------------*/
void CAPI_PROTOCOL_MESSAGE (CAPI_MESSAGE msg) {

#ifndef NOCLOCK
    clock_t lclock;
#endif
    _cmsg   cmsg;
    p = buf;
    if (buf==NULL || signal==NULL) return;
    p[0]=0;

    cmsg.m = msg;
    cmsg.l = 8;
    cmsg.p = 0;
    byteTRcpy (cmsg.m+4, &cmsg.Command);
    byteTRcpy (cmsg.m+5, &cmsg.Subcommand);
    cmsg.par = cpars [command_2_index (cmsg.Command,cmsg.Subcommand)];

#ifndef NOCLOCK
    lclock = clock();
    bufprint ("\n%-26s ID=%03d #0x%04x LEN=%04d     %02ld:%02ld:%02ld:%02ld\n",
		mnames[command_2_index (cmsg.Command,cmsg.Subcommand)],
		((unsigned short *)msg)[1],
		((unsigned short *)msg)[3],
		((unsigned short *)msg)[0],
		(lclock / CLOCKS_PER_SEC / 60l / 60l),
		(lclock / CLOCKS_PER_SEC / 60) % 60l,
		(lclock / CLOCKS_PER_SEC) % 60l,
                ((lclock) % CLOCKS_PER_SEC) / 10);
#else
    bufprint ("\n%-26s ID=%03d #0x%04x LEN=%04d\n",
		mnames[command_2_index (cmsg.Command,cmsg.Subcommand)],
		((unsigned short *)msg)[1],
		((unsigned short *)msg)[3],
                ((unsigned short *)msg)[0]);
#endif

    PROTOCOL_MESSAGE_2_PARS (&cmsg, 1);
    (*signal)(buf, CAPI_PROTOCOL_MSG, msg);
}
