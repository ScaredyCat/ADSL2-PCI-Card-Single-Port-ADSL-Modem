/* $Id: capiinfo.c,v 1.6 2002/07/11 09:29:53 armin Exp $
 *
 * A CAPI application to get infomation about installed controllers
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
 * $Log: capiinfo.c,v $
 * Revision 1.6  2002/07/11 09:29:53  armin
 * sync with new cvs server.
 *
 * Revision 1.5  2001/01/15 10:22:50  calle
 * - error reasons now also as strings using function capi_info2str().
 *
 * Revision 1.4  2000/11/12 16:06:42  kai
 * fix backwards compatibility in capi20 library, small other changes
 *
 * Revision 1.3  2000/06/12 08:51:04  kai
 * show supported supplementary services
 *
 * Revision 1.2  1999/09/10 17:20:33  calle
 * Last changes for proposed standards (CAPI 2.0):
 * - AK1-148 "Linux Extention"
 * - AK1-155 "Support of 64-bit Applications"
 *
 * Revision 1.1  1999/04/16 15:40:48  calle
 * Added first version of capiinfo.
 *
 *
 */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <capi20.h>
#include <linux/capi.h>

struct bittext {
   __u32 bit;
   char *text;
};

struct bittext goptions[] = {
/*  0 */ { 0x0001, "internal controller supported" },
/*  1 */ { 0x0002, "external equipment supported"},
/*  2 */ { 0x0004, "handset supported" },
/*  3 */ { 0x0008, "DTMF supported" },
/*  4 */ { 0x0010, "Supplementary Services supported" },
/*  5 */ { 0x0020, "channel allocation supported (leased lines)" },
 { 0, 0 }
};

struct bittext b1support[] = {
/*  0 */ { 0x0001, "64 kbit/s with HDLC framing" },
/*  1 */ { 0x0002, "64 kbit/s bit-transparent operation" },
/*  2 */ { 0x0004, "V.110 asynconous operation with start/stop byte framing" },
/*  3 */ { 0x0008, "V.110 synconous operation with HDLC framing" },
/*  4 */ { 0x0010, "T.30 modem for fax group 3" },
/*  5 */ { 0x0020, "64 kbit/s inverted with HDLC framing" },
/*  6 */ { 0x0040, "56 kbit/s bit-transparent operation" },
/*  7 */ { 0x0080, "Modem with all negotiations" },
/*  8 */ { 0x0100, "Modem asyncronous operation with start/stop byte framing" },
/*  9 */ { 0x0200, "Modem syncronous operation with HDLC framing" },
 { 0, 0 }
};
struct bittext b2support[] = {
/*  0 */ { 0x0001, "ISO 7776 (X.75 SLP)" },
/*  1 */ { 0x0002, "Transparent" },
/*  2 */ { 0x0004, "SDLC" },
/*  3 */ { 0x0008, "LAPD with Q.921 for D channel X.25 (SAPI 16)" },
/*  4 */ { 0x0010, "T.30 fro fax group 3" },
/*  5 */ { 0x0020, "Point-to-Point Protocol (PPP)" },
/*  6 */ { 0x0040, "Tranparent (ignoring framing errors of B1 protocol)" },
/*  7 */ { 0x0080, "Modem error correction and compression (V.42bis or MNP5)" },
/*  8 */ { 0x0100, "ISO 7776 (X.75 SLP) with V.42bis compression" },
/*  9 */ { 0x0200, "V.120 asyncronous mode" },
/* 10 */ { 0x0400, "V.120 asyncronous mode with V.42bis compression" },
/* 11 */ { 0x0800, "V.120 bit-transparent mode" },
/* 12 */ { 0x1000, "LAPD with Q.921 including free SAPI selection" },
 { 0, 0 }
};
struct bittext b3support[] = {
/*  0 */ { 0x0001, "Transparent" },
/*  1 */ { 0x0002, "T.90NL, T.70NL, T.90" },
/*  2 */ { 0x0004, "ISO 8208 (X.25 DTE-DTE)" },
/*  3 */ { 0x0010, "X.25 DCE" },
/*  4 */ { 0x0020, "T.30 for fax group 3" },
/*  5 */ { 0x0040, "T.30 for fax group 3 with extensions" },
/*  6 */ { 0x0080, "reserved" },
/*  7 */ { 0x0100, "Modem" },
 { 0, 0 }
};

struct bittext SupportedServices[] = {
/*  0 */ { 0x0001, "Hold / Retrieve" },
/*  1 */ { 0x0002, "Terminal Portability" },
/*  2 */ { 0x0004, "ECT" },
/*  3 */ { 0x0008, "3PTY" },
/*  4 */ { 0x0010, "Call Forwarding" },
/*  5 */ { 0x0020, "Call Deflection" },
/*  6 */ { 0x0040, "MCID" },
/*  7 */ { 0x0080, "CCBS" },
 { 0, 0 }
};

static void showbitvalues(struct bittext *p, __u32 value)
{
   while (p->text) {
      if (value & p->bit) printf("   %s\n", p->text);
      p++;
   }
}

int main(int argc, char **argv)
{
   struct capi_profile cprofile;
   unsigned char buf[64];
   unsigned long *vbuf;
   unsigned char *s;
   int ncontr, i;
   unsigned j;
   int isAVM;
   unsigned err, ApplId, MsgId = 1, SSInfo, SuppServices;
   _cmsg cmsg;

   if (CAPI20_ISINSTALLED() != CapiNoError) {
      fprintf(stderr, "capi not installed - %s (%d)\n", strerror(errno), errno);
      return 2;
   }

   CAPI20_GET_PROFILE(0, (CAPI_MESSAGE)&cprofile);
   ncontr = cprofile.ncontroller;
   printf("Number of Controllers : %d\n", ncontr);
 
   err = CAPI20_REGISTER(1, 1, 2048, &ApplId);
   if (err != CapiNoError) {
       fprintf(stderr, "could not register - %s (%#x)\n", capi_info2str(err), err);
       return 1;
   }

   for (i = 1; i <= ncontr; i++) {
       isAVM = 0;
       printf("Controller %d:\n", i);
       CAPI20_GET_MANUFACTURER (i, buf);
       printf("Manufacturer: %s\n", buf);
       if (strstr((char *)buf, "AVM") != 0) isAVM = 1;
       CAPI20_GET_VERSION (i, buf);
       vbuf = (unsigned long *)buf;
       printf("CAPI Version: %lu.%lu\n",vbuf[0], vbuf[1]);
       if (isAVM) {
          printf("Manufacturer Version: %lu.%02lu-%02lu  (%lu.%lu)\n",
                  (vbuf[2]>>4) & 0x0f,
                  (((vbuf[2]<<4) & 0xf0) | ((vbuf[3]>>4) & 0x0f)),
                  vbuf[3] & 0x0f,
                  vbuf[2], vbuf[3] );
       } else {
          printf("Manufacturer Version: %lu.%lu\n",vbuf[2], vbuf[3]);
       }
       CAPI20_GET_SERIAL_NUMBER (i, buf);
       printf("Serial Number: %s\n", (char *)buf);
       CAPI20_GET_PROFILE(i, (CAPI_MESSAGE)&cprofile);
       printf("BChannels: %u\n", cprofile.nbchannel);
       printf("Global Options: 0x%08x\n", cprofile.goptions);
       showbitvalues(goptions, cprofile.goptions);
       printf("B1 protocols support: 0x%08x\n", cprofile.support1);
       showbitvalues(b1support, cprofile.support1);
       printf("B2 protocols support: 0x%08x\n", cprofile.support2);
       showbitvalues(b2support, cprofile.support2);
       printf("B3 protocols support: 0x%08x\n", cprofile.support3);
       showbitvalues(b3support, cprofile.support3);
       for (j=0, s = (unsigned char *)&cprofile; j < sizeof(cprofile); j++) {
           switch (j) {
	      case 0: printf("\n  "); break;
	      case 2: printf("\n  "); break;
	      case 4: printf("\n  "); break;
	      case 8: printf("\n  "); break;
	      case 12: printf("\n  "); break;
	      case 16: printf("\n  "); break;
	      case 20: printf("\n  "); break;
	      case 44: printf("\n  "); break;
	      case 64: printf("\n  "); break;
              default: if ((j % 4) == 0) printf(" ");
	   }
           printf("%02x", s[j]);
       }

       printf("\n");

       FACILITY_REQ_HEADER(&cmsg, ApplId, MsgId++, i);
       cmsg.FacilitySelector = 0x0003;
       cmsg.FacilityRequestParameter = "\x03""\x00\x00""\x00"; // GetSupportedServices

       err = CAPI_PUT_CMSG(&cmsg);
       if (err != CapiNoError) {
	   fprintf(stderr, "FAC REQ - %s (%#x)\n", capi_info2str(err), err);
	   continue;
       }
       err = capi20_waitformessage(ApplId, 0);
       if (err != CapiNoError) {
	   fprintf(stderr, "FAC WAIT - %s (%#x)\n", capi_info2str(err), err);
	   continue;
       }
       err = CAPI_GET_CMSG(&cmsg, ApplId);
       if (err != CapiNoError) {
	   fprintf(stderr, "FAC GET - %s (%#x)\n", capi_info2str(err), err);
	   continue;
       }
       if (cmsg.Info != 0x0000) {
	   fprintf(stderr, "FAC GET - Info: %s (%#x)\n", capi_info2str(cmsg.Info), cmsg.Info);
	   continue;
       }
       if (cmsg.FacilityConfirmationParameter[0] != 0x09) {
	   fprintf(stderr, "FAC GET - (len)\n");
	   continue;
       }
       SSInfo = cmsg.FacilityConfirmationParameter[4];
       SSInfo |= cmsg.FacilityConfirmationParameter[5] << 8;

       SuppServices = cmsg.FacilityConfirmationParameter[6];
       SuppServices |= cmsg.FacilityConfirmationParameter[7] << 8;
       SuppServices |= cmsg.FacilityConfirmationParameter[8] << 16;
       SuppServices |= cmsg.FacilityConfirmationParameter[9] << 24;
       
       printf("\nSupplementary services support: 0x%08x\n", SuppServices);
       showbitvalues(SupportedServices, SuppServices);
       printf("\n");
   }
   return 0;
}
