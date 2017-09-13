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
                                                                                                                             

int dev_landev_num(int opt_flag, void *mthdval_struct, int locate[])
{
	unsigned int lan_dev_num;
	
    if (!opt_flag) // get function
    {
    	//Call native function to get lan_dev_num -- TO DO
    	lan_dev_num = 10;
    	*((unsigned int *)mthdval_struct) = lan_dev_num;
    	printf("The Number of LAN Device is %d.\n", lan_dev_num);
        return 0;
    }
    else           // set function
    {
    	printf("LAN Device Number is unwritable.\n");
        return -1;
    }
}
int dev_wandev_num(int opt_flag, void *mthdval_struct, int locate[])
{
    unsigned int wan_dev_num;
	
    if (!opt_flag) // get function
    {
    	//Call native function to get wan_dev_num -- TO DO
    	wan_dev_num = 10;
    	*((unsigned int *)mthdval_struct) = wan_dev_num;
    	printf("The Number of WAN Device is %d.\n", wan_dev_num);
        return 0;
    }
    else           // set function
    {
    	printf("WAN Device Number is unwritable.\n");
        return -1;
    }
}

