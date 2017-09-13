/* $Id: asn1.h,v 1.7 2000/01/20 07:30:09 kai Exp $
 *
 * ISDN accounting for isdn4linux. (ASN.1 parser)
 *
 * Copyright 1995 .. 2000 by Andreas Kool (akool@isdn4linux.de)
 *
 * ASN.1 parser written by Kai Germaschewski <kai@thphy.uni-duesseldorf.de>
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
 * $Log: asn1.h,v $
 * Revision 1.7  2000/01/20 07:30:09  kai
 * rewrote the ASN.1 parsing stuff. No known problems so far, apart from the
 * following:
 *
 * I don't use buildnumber() anymore to translate the numbers to aliases, because
 * it apparently did never work quite right. If someone knows how to handle
 * buildnumber(), we can go ahead and fix this.
 *
 * Revision 1.6  1999/12/31 13:30:01  akool
 * isdnlog-4.00 (Millenium-Edition)
 *  - Oracle support added by Jan Bolt (Jan.Bolt@t-online.de)
 *
 * Revision 1.5  1999/10/30 13:42:36  akool
 * isdnlog-3.60
 *   - many new rates
 *   - compiler warnings resolved
 *   - fixed "Sonderrufnummer" Handling
 *
 * Revision 1.4  1999/06/26 10:12:02  akool
 * isdnlog Version 3.36
 *  - EGCS 1.1.2 bug correction from Nima <nima_ghasseminejad@public.uni-hamburg.de>
 *  - zone-1.11
 *
 * Revision 1.3  1999/05/04 19:32:32  akool
 * isdnlog Version 3.24
 *
 *  - fully removed "sondernummern.c"
 *  - removed "gcc -Wall" warnings in ASN.1 Parser
 *  - many new entries for "rate-de.dat"
 *  - better "isdnconf" utility
 *
 * Revision 1.2  1999/04/26 22:11:50  akool
 * isdnlog Version 3.21
 *
 *  - CVS headers added to the asn* files
 *  - repaired the "4.CI" message directly on CONNECT
 *  - HANGUP message extended (CI's and EH's shown)
 *  - reactivated the OVERLOAD message
 *  - rate-at.dat extended
 *  - fixes from Michael Reinelt
 *
 *
 * Revision 0.1  1999/04/25 20:00.00  akool
 * Initial revision
 *
 */

#ifndef __ASN1_H__
#define __ASN1_H__

// TODO: 
// - displaying numbers does not use the provided isdnlog capabilities 
//   at this time
// - documentation

struct Aoc {
  int       type;
  char      currency[11];
  int       amount;
  float     multiplier;
  char      msg[255];
  int       type_of_charging_info;
};


#include <stdio.h>
#include <sys/types.h>
#include "isdnlog.h" //  for print_msg

int ParseASN1(u_char *p, u_char *end, int level);

int ParseTag(u_char *p, u_char *end, int *tag);
int ParseLen(u_char *p, u_char *end, int *len);

#define ASN1_TAG_BOOLEAN           (0x01) // is that true?
#define ASN1_TAG_INTEGER           (0x02)
#define ASN1_TAG_BIT_STRING        (0x03)
#define ASN1_TAG_OCTET_STRING      (0x04)
#define ASN1_TAG_NULL              (0x05)
#define ASN1_TAG_OBJECT_IDENTIFIER (0x06)
#define ASN1_TAG_ENUM              (0x0a)
#define ASN1_TAG_SEQUENCE          (0x30)
#define ASN1_TAG_SET               (0x31)
#define ASN1_TAG_NUMERIC_STRING    (0x12)
#define ASN1_TAG_PRINTABLE_STRING  (0x13)
#define ASN1_TAG_IA5_STRING        (0x16)
#define ASN1_TAG_UTC_TIME          (0x17)

#define ASN1_TAG_CONSTRUCTED       (0x20)
#define ASN1_TAG_CONTEXT_SPECIFIC  (0x80)

#define ASN1_TAG_EXPLICIT          (0x100)
#define ASN1_TAG_OPT               (0x200)
#define ASN1_NOT_TAGGED            (0x400)

#define CallASN1(ret, p, end, todo) do { \
        ret = todo; \
	if (ret < 0) { \
                print_msg(PRT_DEBUG_DECODE, " DEBUG> err 2 %s:%d\n", __FUNCTION__, __LINE__); \
                return -1; \
        } \
        p += ret; \
} while (0)

#define INIT \
	int tag, len; \
	int ret; \
	u_char *beg; \
        \
        print_msg(PRT_DEBUG_DECODE, " DEBUG> %s\n", __FUNCTION__); \
	beg = p; \
	CallASN1(ret, p, end, ParseTag(p, end, &tag)); \
	CallASN1(ret, p, end, ParseLen(p, end, &len)); \
        if (len >= 0) { \
                if (p + len > end) \
                        return -1; \
                end = p + len; \
        }

#define XSEQUENCE_1(todo, act_tag, the_tag, arg1) do { \
	if (p < end) { \
  	        if (((the_tag) &~ ASN1_TAG_OPT) == ASN1_NOT_TAGGED) { \
		        if (((u_char)act_tag == *p) || ((act_tag) == ASN1_NOT_TAGGED)) { \
			        CallASN1(ret, p, end, todo(chanp, p, end, arg1)); \
                        } else { \
                                if (!((the_tag) & ASN1_TAG_OPT)) { \
                                        print_msg(PRT_DEBUG_DECODE, " DEBUG> err 1 %s:%d\n", __FUNCTION__, __LINE__); \
                	    	        return -1; \
                                } \
                        } \
	        } else { \
                        if ((the_tag) & ASN1_TAG_EXPLICIT) { \
		                if ((u_char)(((the_tag) & 0xff) | (ASN1_TAG_CONTEXT_SPECIFIC | ASN1_TAG_CONSTRUCTED)) == *p) { \
                                        int xtag, xlen; \
	                                CallASN1(ret, p, end, ParseTag(p, end, &xtag)); \
			                CallASN1(ret, p, end, ParseLen(p, end, &xlen)); \
  	                                CallASN1(ret, p, end, todo(chanp, p, end, arg1)); \
                                } else { \
                                        if (!(the_tag) & ASN1_TAG_OPT) { \
                                                print_msg(PRT_DEBUG_DECODE, " DEBUG> err 2 %s:%d\n", __FUNCTION__, __LINE__); \
                        	    	        return -1; \
                                        } \
                                } \
                        } else { \
		                if ((u_char)(((the_tag) & 0xff) | (ASN1_TAG_CONTEXT_SPECIFIC | (act_tag & ASN1_TAG_CONSTRUCTED))) == *p) { \
  	                                CallASN1(ret, p, end, todo(chanp, p, end, arg1)); \
                                } else { \
                                        if (!(the_tag) & ASN1_TAG_OPT) { \
                                                print_msg(PRT_DEBUG_DECODE, " DEBUG> err 3 %s:%d\n", __FUNCTION__, __LINE__); \
                        	    	        return -1; \
                                        } \
                                } \
		        } \
		} \
        } else { \
                if (!(the_tag) & ASN1_TAG_OPT) { \
                        print_msg(PRT_DEBUG_DECODE, " DEBUG> err 4 %s:%d\n", __FUNCTION__, __LINE__); \
	    	        return -1; \
                } \
        } \
} while (0)

#define XSEQUENCE_OPT_1(todo, act_tag, the_tag, arg1) \
        XSEQUENCE_1(todo, act_tag, (the_tag | ASN1_TAG_OPT), arg1)

#define XSEQUENCE(todo, act_tag, the_tag) XSEQUENCE_1(todo, act_tag, the_tag, -1)
#define XSEQUENCE_OPT(todo, act_tag, the_tag) XSEQUENCE_OPT_1(todo, act_tag, the_tag, -1)

#define XCHOICE_1(todo, act_tag, the_tag, arg1) \
	if (act_tag == ASN1_NOT_TAGGED) { \
		return todo(chanp, beg, end, arg1); \
        } \
        if (the_tag == ASN1_NOT_TAGGED) { \
		  if (act_tag == tag) { \
                            return todo(chanp, beg, end, arg1); \
                  } \
         } else { \
		  if ((the_tag | (0x80 | (act_tag & 0x20))) == tag) { \
                            return todo(chanp, beg, end, arg1); \
                  } \
	 }

#define XCHOICE(todo, act_tag, the_tag) XCHOICE_1(todo, act_tag, the_tag, -1)

#define XCHOICE_DEFAULT do {\
          print_msg(PRT_DEBUG_DECODE, " DEBUG> err 5 %s:%d\n", __FUNCTION__, __LINE__); \
          return -1; \
	  } while (0)

#define CHECK_P do { \
        if (p >= end) \
                 return -1; \
        } while (0) 


#endif
