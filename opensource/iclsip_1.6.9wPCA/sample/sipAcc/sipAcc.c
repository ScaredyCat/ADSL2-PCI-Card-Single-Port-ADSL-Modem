/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * sipAcc.c
 *
 * $Id: sipAcc.c,v 1.14 2007/01/12 02:26:10 tyhuang Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sipAcc.h"

int main(void)
{
	RCODE rCode=RC_OK;

	printf("#############################################################\n");
	printf(" SIP Acceptance Test Program\n");
	printf("#############################################################\n");
#if 0
	rCode|=accCommon();
	rCode|=accLow();
	rCode|=accAdt();
	rCode|=accSDP();
	rCode|=accSip();

	if(rCode==RC_OK)
		printf("SIP Acceptance Test Program Successed.\n");
	else
		printf("SIP Acceptance Test Program Failed.\n");
#endif		
	return rCode;
}
