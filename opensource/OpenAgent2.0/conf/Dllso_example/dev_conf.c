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
                                                                                                                             

//deviceconfig
int dev_PersistentData(int opt_flag, void *mthdval_struct, int locate[])
{
	if (!opt_flag) // get function
    {
    	//Call native function to get persistent data -- TO DO
	    strcpy((char *)mthdval_struct, "1111111111");
        printf("The persistent data is %s.\n", (char *)mthdval_struct);
        return 0;
    }
    else           // set function
    {
    	//Call native function to set persistent data -- TO DO
        printf("Set persistent data to %s success.\n", (char *)mthdval_struct);
        return 0;
    }
}
int dev_ConfigFile(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
    	//Call native function to get config file -- TO DO
	    strcpy((char *)mthdval_struct, "1111111111");
        printf("The config file is %s.\n", (char *)mthdval_struct);
        return 0;
    }
    else           // set function
    {
    	//Call native function to set config file -- TO DO
    	printf("Set config file to %s success.\n", (char *)mthdval_struct);
        return 0;
    }
}
