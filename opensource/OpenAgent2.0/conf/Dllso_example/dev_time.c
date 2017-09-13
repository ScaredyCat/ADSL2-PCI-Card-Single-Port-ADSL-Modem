/*
 * Copyright(c) 2006-2007, Works Systems, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, 
 *    this list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright notice, 
 *    this list of conditions and the following disclaimer in the documentation 
 *    and/or other materials provided with the distribution. 
 * 3. Neither the name of the vendors nor the names of its contributors 
 *    may be used to endorse or promote products derived from this software 
 *    without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/*
 * $Author: joinsonj $
 * $Date: 2007-06-08 02:14:47 $
 */
                                                                                                                             
#include <time.h>

//time
int dev_Time_NTPServer1(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
    	//Call native function to get NTPServer1 -- TO DO
	    strcpy((char *)mthdval_struct, "1111111111");
        //printf("The NTPServer1 is %s.\n", (char *)mthdval_struct);
        return 0;
    }
    else           // set function
    {
    	//Call native function to set NTPServer1 -- TO DO
    	printf("Set NTPServer1 to %s success.\n", (char *)mthdval_struct);
        return 0;
    }
}

int dev_Time_NTPServer2(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
    	//Call native function to get NTPServer2 -- TO DO
	    strcpy((char *)mthdval_struct, "1111111111");
        //printf("The NTPServer2 is %s.\n", (char *)mthdval_struct);
        return 0;
    }
    else           // set function
    {
    	//Call native function to set NTPServer2 -- TO DO
    	printf("Set NTPServer2 to %s success.\n", (char *)mthdval_struct);
        return 0;
    }
}

int dev_Time_NTPServer3(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
    	//Call native function to get NTPServer3 -- TO DO
	    strcpy((char *)mthdval_struct, "1111111111");
        //printf("The NTPServer3 is %s.\n", (char *)mthdval_struct);
        return 0;
    }
    else           // set function
    {
    	//Call native function to set NTPServer3 -- TO DO
    	printf("Set NTPServer3 to %s success.\n", (char *)mthdval_struct);
        return 0;
    }
}

int dev_Time_NTPServer4(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
    	//Call native function to get NTPServer4 -- TO DO
	    strcpy((char *)mthdval_struct, "1111111111");
        //printf("The NTPServer4 is %s.\n", (char *)mthdval_struct);
        return 0;
    }
    else           // set function
    {
    	//Call native function to set NTPServer4 -- TO DO
    	printf("Set NTPServer4 to %s success.\n", (char *)mthdval_struct);
        return 0;
    }
}

int dev_Time_NTPServer5(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
    	//Call native function to get NTPServer5 -- TO DO
	    strcpy((char *)mthdval_struct, "1111111111");
        //printf("The NTPServer5 is %s.\n", (char *)mthdval_struct);
        return 0;
    }
    else           // set function
    {
    	//Call native function to set NTPServer5 -- TO DO
    	printf("Set NTPServer5 to %s success.\n", (char *)mthdval_struct);
        return 0;
    }
}

int dev_Time_CurrentLocalTime(int opt_flag, void *mthdval_struct, int locate[])
{
    time_t CurrentLocalTime;
	
    if (!opt_flag) // get function
    {
    	//Call native function to get CurrentLocalTime -- TO DO
    	//CurrentLocalTime = time((time_t *)0);
    	//*((unsigned int *)mthdval_struct) = CurrentLocalTime;
    	//printf("The CurrentLocalTime of Device is %ld.\n", CurrentLocalTime);
        strcpy((char *)mthdval_struct, "2000-00-00T00:00:00");
        return 0;
    }
    else           // set function
    {
    	printf("The CurrentLocalTime of Device is unwritable.\n");
        return -1;
    }
}

int dev_Time_LocalTimeZone(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
    	//Call native function to get LocalTimeZone -- TO DO
	    strcpy((char *)mthdval_struct, "1111111111");
        //printf("The LocalTimeZone is %s.\n", (char *)mthdval_struct);
        return 0;
    }
    else           // set function
    {
    	//Call native function to set LocalTimeZone -- TO DO
    	printf("Set LocalTimeZone to %s success.\n", (char *)mthdval_struct);
        return 0;
    }
}

int dev_Time_LocalTimeZoneName(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
    	//Call native function to get LocalTimeZoneName -- TO DO
	    strcpy((char *)mthdval_struct, "1111111111");
        //printf("The LocalTimeZoneName is %s.\n", (char *)mthdval_struct);
        return 0;
    }
    else           // set function
    {
    	//Call native function to set LocalTimeZoneName -- TO DO
    	printf("Set LocalTimeZoneName to %s success.\n", (char *)mthdval_struct);
        return 0;
    }
}

int dev_Time_DaylightSavingUsed(int opt_flag, void *mthdval_struct, int locate[])
{
    unsigned int DaylightSavingUsed = 0;
	
    if (!opt_flag) // get function
    {
    	//Call native function to get DaylightSavingUsed -- TO DO
    	DaylightSavingUsed = 1;
    	*((unsigned int *)mthdval_struct) = DaylightSavingUsed;
    	//printf("The DaylightSavingUsed of Device is %d.\n", DaylightSavingUsed);
        return 0;
    }
    else           // set function
    {
    	//Call native function to set DaylightSavingUsed -- TO DO
    	printf("Set DaylightSavingUsed to %d success.\n", DaylightSavingUsed);
        return 0;
    }
}

int dev_Time_DaylightSavingStart(int opt_flag, void *mthdval_struct, int locate[])
{
    time_t DaylightSavingStart = 0;
	
    if (!opt_flag) // get function
    {
    	//Call native function to get DaylightSavingStart -- TO DO
    	//DaylightSavingStart = 999999999;
    	//*((unsigned int *)mthdval_struct) = DaylightSavingStart;
    	//printf("The DaylightSavingStart of Device is %ld.\n", DaylightSavingStart);
        strcpy((char *)mthdval_struct, "2000-00-00T00:00:00");
        return 0;
    }
    else           // set function
    {
    	//Call native function to set DaylightSavingStart -- TO DO
    	printf("Set DaylightSavingStart to %ld success.\n", DaylightSavingStart);
        return 0;
    }
}

int dev_Time_DaylightSavingEnd(int opt_flag, void *mthdval_struct, int locate[])
{
    time_t DaylightSavingEnd = 0;
	
    if (!opt_flag) // get function
    {
    	//Call native function to get DaylightSavingEnd -- TO DO
    	//DaylightSavingEnd = 999999999;
    	//*((unsigned int *)mthdval_struct) = DaylightSavingEnd;
    	//printf("The DaylightSavingEnd of Device is %ld.\n", DaylightSavingEnd);
        strcpy((char *)mthdval_struct, "2000-00-00T00:00:00");
        return 0;
    }
    else           // set function
    {
    	//Call native function to set DaylightSavingEnd -- TO DO
    	printf("Set DaylightSavingEnd to %ld success.\n", DaylightSavingEnd);
        return 0;
    }
}

