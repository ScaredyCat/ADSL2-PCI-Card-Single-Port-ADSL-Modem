/*
** $Id: vboxconvert.h,v 1.2 1997/05/10 10:58:55 michael Exp $
**
** Copyright (C) 1996, 1997 Michael 'Ghandi' Herold
*/

#ifndef _VBOX_CONVERT
#define _VBOX_CONVERT 1

/** Defines **************************************************************/

#define SND_FORMAT_MULAW_8		1
#define SND_FORMAT_LINEAR_8	2
#define SND_FORMAT_LINEAR_16	3
#define SND_MAGIC					(0x2e736e64L)
#define SND_HEADER_SIZE       28
#define SND_UNKNOWN_SIZE      ((int)(-1))
#define SUN_FORMAT_MIN			0
#define SUN_FORMAT_MAX			22

/** Structures ***********************************************************/

typedef struct
{
    int magic;               						 /* Magic number SND_MAGIC	*/
    int dataLocation;        				/* Offset or pointer to the data	*/
    int dataSize;            						/* Number of bytes of data	*/
    int dataFormat;          							/* The data format code	*/
    int samplingRate;        								/* The sampling rate	*/
    int channelCount;        						 /* The number of channels	*/
    char info[4];            					 /* Optional text information	*/
} SNDSoundStruct;

typedef struct
{
	int	word;
	int	nleft;
} state_t;

#endif /* _VBOX_CONVERT */
