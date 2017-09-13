/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * base64.c
 *
 * $Id: base64.c,v 1.17 2005/09/30 02:22:23 tyhuang Exp $
 */

#include <string.h>

#include "base64.h"

#define BASE64BINLEN                    (3)
#define BASE64TEXTLEN                   (4)

#define BASE64TXTLINELEN                (76)
#define BASE64BINLINELEN                ((BASE64TXTLINELEN/BASE64TEXTLEN)*BASE64BINLEN)


const unsigned char _Base64EncodeMap[65] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
    'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
    'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    'w', 'x', 'y', 'z', '0', '1', '2', '3',
    '4', '5', '6', '7', '8', '9', '+', '/',
    '=' };

const unsigned char _Base64DecodeMap[256] = {
    66, 66, 66, 66, 66, 66, 66, 66,
    66, 66, 66, 66, 66, 66, 66, 66,
    66, 66, 66, 66, 66, 66, 66, 66,
    66, 66, 66, 66, 66, 66, 66, 66,
    66, 66, 66, 66, 66, 66, 66, 66,
    66, 66, 66, 62, 66, 66, 66, 63,
    52, 53, 54, 55, 56, 57, 58, 59,
    60, 61, 66, 66, 66, 64, 66, 66,
    66,  0,  1,  2,  3,  4,  5,  6,
     7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22,
    23, 24, 25, 66, 66, 66, 66, 66,
    66, 26, 27, 28, 29, 30, 31, 32,
    33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48,
    49, 50, 51, 66, 66, 66, 66, 66,
    66, 66, 66, 66, 66, 66, 66, 66,
    66, 66, 66, 66, 66, 66, 66, 66,
    66, 66, 66, 66, 66, 66, 66, 66,
    66, 66, 66, 66, 66, 66, 66, 66,
    66, 66, 66, 66, 66, 66, 66, 66,
    66, 66, 66, 66, 66, 66, 66, 66,
    66, 66, 66, 66, 66, 66, 66, 66,
    66, 66, 66, 66, 66, 66, 66, 66,
    66, 66, 66, 66, 66, 66, 66, 66,
    66, 66, 66, 66, 66, 66, 66, 66,
    66, 66, 66, 66, 66, 66, 66, 66,
    66, 66, 66, 66, 66, 66, 66, 66,
    66, 66, 66, 66, 66, 66, 66, 66,
    66, 66, 66, 66, 66, 66, 66, 66,
    66, 66, 66, 66, 66, 66, 66, 66,
    66, 66, 66, 66, 66, 66, 66, 66 };

CCLAPI
/*void base64decode(char* source, char* target, int targetlen)*/
long base64decode(char* source, char* target, int targetlen)
{
    int         cc, c;
    long        count = 0;
    long	slen_count = 0;
    long	tlen_count = 0;
    union{
        struct{
		unsigned c3 : 6;
		unsigned c2 : 6;
		unsigned cx1: 4;
		unsigned cx2: 4;
		unsigned c1 : 6;
		unsigned c0 : 6;
        } Base64;
        unsigned char	Binary[4];
        unsigned long   lbuffer;
    } Mapper;

	if(!source||!target)	return -1;
	if((4*(targetlen-1)) <= (((int)strlen(source))*3)) return -1;

    Mapper.lbuffer = 0;
    *target = 0;

    while((cc = source[slen_count]) > 0 && tlen_count<targetlen ){
        if ( (c = _Base64DecodeMap[cc]) < 64 ){
            switch(count%4){
                case 0: Mapper.Base64.c0 = c;
                        break;
                case 1: Mapper.Base64.c1 = c;
                        break;
                case 2: Mapper.Base64.c2 = c;
                        break;
                case 3: Mapper.Base64.c3 = c;
                        target[tlen_count++] = Mapper.Binary[3];
                        target[tlen_count++] = (Mapper.Binary[1]|Mapper.Binary[2]);
                        target[tlen_count++] = Mapper.Binary[0];
                        Mapper.lbuffer = 0; /* clear mapper buffer;*/
                        break;
            } /*end switch case */
            count++;
        }
        else if ( cc == '=' )
        {
            switch(count%4){
                case 0: /* error */
                case 1: continue; /* error ignore */
                case 2: target[tlen_count++] = Mapper.Binary[3];
                        break;
                case 3:
                        target[tlen_count++] = Mapper.Binary[3];
                        target[tlen_count++] = (Mapper.Binary[1]|Mapper.Binary[2]);
                        break;
            }
			target[tlen_count++] = '\0';
            return slen_count;
        }

		slen_count++;
    } /* End of while */
    target[tlen_count++] = '\0';

	return 0;
}

CCLAPI
long base64encode(char* source, char* target, int targetlen)
{
    int			c;
    long        count = 0;
	long		slen_count = 0;
	long		tlen_count = 0;
    union
    {
        struct
        {
            unsigned c3 : 6;
            unsigned c2 : 6;
            unsigned cx1: 4;
            unsigned cx2: 4;
            unsigned c1 : 6;
            unsigned c0 : 6;
        } Base64;
        unsigned char   Binary[4];
        unsigned long   lbuffer;
    } Mapper;

    if(!source||!target)	return -1;
	if( (3*(targetlen-1)) < (int)(4*strlen(source))) return -1;

    Mapper.lbuffer = 0;
    *target = 0;

    while( (c=source[slen_count]) > 0 && tlen_count<targetlen )
    {
        switch(count%3)
        {
            case 0: Mapper.Binary[3] = (unsigned char)c;
                    break;
            case 1: Mapper.Binary[1] = Mapper.Binary[2] = (unsigned char)c;
                    break;
            case 2: Mapper.Binary[0] = (unsigned char)c;
                    target[tlen_count++] = _Base64EncodeMap[Mapper.Base64.c0];
                    target[tlen_count++] = _Base64EncodeMap[Mapper.Base64.c1];
                    target[tlen_count++] = _Base64EncodeMap[Mapper.Base64.c2];
                    target[tlen_count++] = _Base64EncodeMap[Mapper.Base64.c3];
                    Mapper.lbuffer = 0; /* clear buffer */
                    if ( count == BASE64BINLINELEN-1 )
                    {
                        target[tlen_count++] = '\n';
                        count = 0; /* reset count */
						slen_count++;
                        continue;
                    }
        }
        count++;
		slen_count++;
    }

    switch(count%3)
    {
        case 0: break;
        case 1:
                target[tlen_count++] = _Base64EncodeMap[Mapper.Base64.c0];
                target[tlen_count++] = _Base64EncodeMap[Mapper.Base64.c1];
                target[tlen_count++] = '=';
                target[tlen_count++] = '=';
                break;
        case 2:
                target[tlen_count++] = _Base64EncodeMap[Mapper.Base64.c0];
                target[tlen_count++] = _Base64EncodeMap[Mapper.Base64.c1];
                target[tlen_count++] = _Base64EncodeMap[Mapper.Base64.c2];
                target[tlen_count++] = '=';
                break;
    }
    target[tlen_count++] = '\0';
	return 0;
}
