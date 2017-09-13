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
/**********************************************************************
*
*	cpe_agent.c - The main routine for Agent
*
*	$Author: joinsonj $
*
*	history: $Date: 2007-06-08 02:22:17 $, initial version by simonl
*
***********************************************************************/


/***********************************************************************
*
*	include file
*
***********************************************************************/

#include "res/global_res.h"
#include "init/init.h"
#include "event/event.h"
#include "CLI/CLI.h"
#include "tools/conn_req_server.h"

/************************************************************************
*
*	main - the main routine for Agent
*
************************************************************************/

int main(int argc, char *argv[])
{
    int res;
    int ch;

    while((ch = getopt(argc, argv, "F:")) != -1)
    switch(ch) {
       
        case 'F':
            memset(conf_dir, '\0', sizeof(conf_dir));
            strcpy(conf_dir, optarg);
            break;
    }
 
    /* Initialize global variables */
    res = init_cpe();
    if(res != 0) {
        LOG(all, ERROR, "Initialize global variables failed\n");
        return -1;    
    } else {
        LOG(all, DEBUG, "Initialize global variables success\n");
    }
    
    /* Initialize for download */
    res = init_download();
    if(res != 0) {
        LOG(all, ERROR, "Initialize for download failed\n");
        return -1;    
    } else {
        LOG(all, DEBUG, "Initialize for download success\n");
    }
    
    /* Invoke CLI module */
    res = init_CLI();
    if(res != 0) {
        LOG(all, ERROR, "Invoke CLI module failed\n");
        return -1;    
    } else {
        LOG(all, DEBUG, "Invoke CLI module success\n");
    }
    
    /* Invoke connection request server*/
    res = init_CRS();
    if(res != 0) {
        LOG(all, ERROR, "Invoke init_CRS() failed\n");
        return -1;
    } else {
        LOG(all, DEBUG, "Invoke init_CRS() success\n");
    }
    
    /* Invoke event module */
    res = init_event();
    if(res != 0) {
        LOG(all, ERROR, "Invoke event module failed\n");
        return -1;    
    } else {
        LOG(all, DEBUG, "Invoke event module success\n");
    }

    return 0;
}


