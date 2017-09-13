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
*	init.h - Initialize global resource for Agent
*
*	$Author: joinsonj $
*
*	history: $Date: 2007-06-08 02:23:39 $, initial version by simonl
*
***********************************************************************/


#ifndef INIT_H_
#define INIT_H_

/***********************************************************************
*
*	include file
*
***********************************************************************/

#include "../res/global_res.h"
#include "../res/qbuf.h"
#include "../comm/comm.h"
#include "../handler/handler.h"
#include "../event/event.h"
#include "../tools/dl_conf.h"
#include "../tools/agent_conf.h"
#include "../handler/methods/download.h"
#include "../CLI/CLI.h"
#include "../device/TRF_param.h"
#include "../handler/cpe_task_list.h"

/************************************************************************
*
*	struct define
*
************************************************************************/



/************************************************************************
*
*	macrodefinition
*
************************************************************************/

#define GO_ON    0
#define REBOOT_NOW -1


/************************************************************************
*
*	global var define
*
************************************************************************/



/***********************************************************************
*
*	function declare
*
***********************************************************************/

int init_cpe(); 

int init_download();


#endif /* INIT_H_ */

