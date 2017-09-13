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

#ifndef QBUF_H_
	#define QBUF_H_
	
	/***********************************************************************
	*
	*	include file
	*
	***********************************************************************/

	#include "global_res.h"
	#include "../tools/logger.h"
	
	
	/************************************************************************
	*
	*	public macrodefinition define
	*
	************************************************************************/
	#ifndef MAX_DATA_LEN
		#define MAX_DATA_LEN 35840
	#endif

        #define DEBUG_DISPLAY

	
	/************************************************************************
	*
	*	publish struct define
	*
	************************************************************************/
        typedef struct qbuf {
            char data[MAX_DATA_LEN];
            struct list_head next;
	
            char* (*get_data_ptr)(struct qbuf*);
            void (*set)(struct qbuf*, char*);
            void (*copy)(struct qbuf*, char*);
        } Qbuf_node;

        typedef struct qbuf_list {
            int qbuf_node_num;
            struct list_head header;
            pthread_mutex_t lock;

            int (*create)(struct qbuf_list*, int);
            void (*destroy)(struct qbuf_list*);
            int (*reset_lock)(struct qbuf_list*);
            Qbuf_node* (*head)(struct qbuf_list*);
            void (*append)(struct qbuf_list*, Qbuf_node *);
            int (*qbuf_nodes_num)(struct qbuf_list*);
            int (*move_node)(struct qbuf_list*, struct qbuf_list*);
            #ifdef DEBUG_DISPLAY
            void (*display)(struct qbuf_list*);
            #endif
        } Qbuf_list;
	
	
	/************************************************************************
	*
	*	publish var define
	*
	************************************************************************/
        Qbuf_list empty_qbuf_list;
        Qbuf_list send_qbuf_list;
        Qbuf_list recv_qbuf_list;
	
	/***********************************************************************
	*
	*	publish function declare
	*
	***********************************************************************/
        int init_list_member(Qbuf_list *list);

	
#endif

