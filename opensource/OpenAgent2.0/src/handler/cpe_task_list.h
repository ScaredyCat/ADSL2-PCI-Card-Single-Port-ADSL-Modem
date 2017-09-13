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
 * Prevent multiple inclusion...
 */

#ifndef CPE_TASK_LIST_H_
#define CPE_TASK_LIST_H_
#define TASK_LIST_NULL 1

/*
 * Include necessary headers...
 */
#include "../res/global_res.h"
#include "./methods/download.h"

/*
 * Data types...			
 */
 
typedef struct cpe_task_list{
    char func_name[256]; 
    void * func_param_struct; 
    struct list_head node;
}TR_cpe_task_list;

/*
 * Arguments...
 */
 
struct list_head cpe_task_list_head;

/*

 ******************************************************************************

 *			FUNCTION PROTOTYPES	

 ******************************************************************************

 */
int init_task_list();
int add_task_list(char *func_name, void *func_param_struct);
int count_task_list(struct list_head  *_lst);
int exec_task();
int handle_download_task(TR_download  *down);

#endif   /* CPE_TASK_LIST_H_ */
