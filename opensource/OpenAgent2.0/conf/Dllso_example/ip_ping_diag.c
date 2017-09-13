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
                                                                                                                             

//IPPingDiagnostics
int dev_IPPing_DiagnosticsState(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
    	//Call native function to get IPPing DiagnosticsState -- TO DO
        strcpy((char *)mthdval_struct, "1111111111");
        printf("The IPPing DiagnosticsState is %s.\n", (char *)mthdval_struct);
        return 0;
    }
    else           // set function
    {
    	//Call native function to set IPPing DiagnosticsState -- TO DO
    	printf("Set IPPing DiagnosticsState to %s success.\n", (char *)mthdval_struct);
        return 0;
    }
}
int dev_IPPing_Interface(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
    	//Call native function to get IPPing Interface -- TO DO
	    strcpy((char *)mthdval_struct, "1111111111");
        printf("The IPPing Interface is %s.\n", (char *)mthdval_struct);
        return 0;
    }
    else           // set function
    {
    	//Call native function to set IPPing Interface -- TO DO
    	printf("Set IPPing Interface to %s success.\n", (char *)mthdval_struct);
        return 0;
    }
}
int dev_IPPing_Host(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
    	//Call native function to get IPPing Host -- TO DO
	    strcpy((char *)mthdval_struct, "1111111111");
        printf("The IPPing Host is %s.\n", (char *)mthdval_struct);
        return 0;
    }
    else           // set function
    {
    	//Call native function to set IPPing Host -- TO DO
        printf("Set IPPing Host to %s success.\n", (char *)mthdval_struct);
        return 0;
    }
}
int dev_IPPing_NumberOfRepetitions(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
    	//Call native function to get IPPing NumberOfRepetitions -- TO DO
    	*((unsigned int *)mthdval_struct) = 10;
    	printf("The IPPing NumberOfRepetitions is %d.\n", *((unsigned int *)mthdval_struct));
        return 0;
    }
    else           // set function
    {
    	//Call native function to set IPPing NumberOfRepetitions -- TO DO
    	printf("Set IPPing NumberOfRepetitions to %d success.\n", *((unsigned int *)mthdval_struct));
        return 0;
    }
}
int dev_IPPing_Timeout(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
    	//Call native function to get IPPing Timeout -- TO DO
    	*((unsigned int *)mthdval_struct) = 10;
    	printf("The IPPing Timeout is %d.\n", *((unsigned int *)mthdval_struct));
        return 0;
    }
    else           // set function
    {
    	//Call native function to set IPPing Timeout -- TO DO
    	printf("Set IPPing Timeout to %d success.\n", *((unsigned int *)mthdval_struct));
        return 0;
    }
}
int dev_IPPing_DataBlockSize(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
    	//Call native function to get IPPing DataBlockSize -- TO DO
    	*((unsigned int *)mthdval_struct) = 10;
    	printf("The IPPing DataBlockSize is %d.\n", *((unsigned int *)mthdval_struct));
        return 0;
    }
    else           // set function
    {
    	//Call native function to set IPPing DataBlockSize -- TO DO
    	printf("Set IPPing DataBlockSize to %d success.\n", *((unsigned int *)mthdval_struct));
        return 0;
    }
}

int dev_IPPing_DSCP(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
    	//Call native function to get IPPing DSCP -- TO DO
    	*((unsigned int *)mthdval_struct) = 10;
    	printf("The IPPing DSCP is %d.\n", *((unsigned int *)mthdval_struct));
        return 0;
    }
    else           // set function
    {
    	//Call native function to set IPPing DSCP -- TO DO
    	printf("Set IPPing DSCP to %d success.\n", *((unsigned int *)mthdval_struct));
        return 0;
    }
}

int dev_IPPing_SuccessCount(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
    	//Call native function to get IPPing SuccessCount -- TO DO
    	*((unsigned int *)mthdval_struct) = 10;
    	printf("The IPPing SuccessCount is %d.\n", *((unsigned int *)mthdval_struct));
        return 0;
    }
    else           // set function
    {
    	printf("IPPing SuccessCount is unwritable.\n", *((unsigned int *)mthdval_struct));
        return -1;
    }
}
int dev_IPPing_FailureCount(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
    	//Call native function to get IPPing FailureCount -- TO DO
    	*((unsigned int *)mthdval_struct) = 10;
    	printf("The IPPing FailureCount is %d.\n", *((unsigned int *)mthdval_struct));
        return 0;
    }
    else           // set function
    {
    	printf("IPPing FailureCount is unwritable.\n", *((unsigned int *)mthdval_struct));
        return -1;
    }
}
int dev_IPPing_AverageResponseTime(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
    	//Call native function to get IPPing AverageResponseTime -- TO DO
    	*((unsigned int *)mthdval_struct) = 10;
    	printf("The IPPing AverageResponseTime is %d.\n", *((unsigned int *)mthdval_struct));
        return 0;
    }
    else           // set function
    {
    	printf("IPPing AverageResponseTime is unwritable.\n", *((unsigned int *)mthdval_struct));
        return -1;
    }
}
int dev_IPPing_MinimumResponseTime(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
    	//Call native function to get IPPing MinimumResponseTime -- TO DO
    	*((unsigned int *)mthdval_struct) = 10;
     	printf("The IPPing MinimumResponseTime is %d.\n", *((unsigned int *)mthdval_struct));
        return 0;
    }
    else           // set function
    {
    	printf("IPPing MinimumResponseTime is unwritable.\n", *((unsigned int *)mthdval_struct));
        return -1;
    }
}
int dev_IPPing_MaximumResponseTime(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
    	//Call native function to get IPPing MaximumResponseTime -- TO DO
    	*((unsigned int *)mthdval_struct) = 10;
    	printf("The IPPing MaximumResponseTime is %d.\n", *((unsigned int *)mthdval_struct));
        return 0;
    }
    else           // set function
    {
    	printf("IPPing MaximumResponseTime is unwritable.\n", *((unsigned int *)mthdval_struct));
        return -1;
    }
}

