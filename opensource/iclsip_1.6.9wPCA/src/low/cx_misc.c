/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * cx_misc.c
 *
 * $Id: cx_misc.c,v 1.3 2002/07/31 02:45:10 yjliao Exp $
 */

#define _POSIX_C_SOURCE 199309L /* struct timespec */

#include "cx_misc.h"

#ifdef UNIX
#include <time.h>
void  sleeping(unsigned int msec)
{
	struct timespec rqtp;
	
	rqtp.tv_sec = msec/1000; rqtp.tv_nsec = (msec%1000)*1000000;/* 10ms */
	nanosleep(&rqtp,NULL);
}
#endif
