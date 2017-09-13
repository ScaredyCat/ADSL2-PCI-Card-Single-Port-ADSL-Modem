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
 
#ifndef HANDLER_H_
#define HANDLER_H_
/*
 * Include head file
 */

#include <errno.h>
#include "../res/global_res.h"
#include "soap/soap.h"
#include "methods/inform.h"
#include "../event/event.h"
#include "../CLI/CLI.h"

/*
 * Define return value
 */

#define INIT_HANDLE_SUCCESS 0
#define INIT_HANDLE_FAILED  -1

/*
 * Arguments
 */

struct list_head cpe_req_list_head;

/*
 * Data types...
 */

typedef struct {
    int retry_count;
    char req_name[256];
    void * req_param_struct;
    struct list_head node;
}TR_cpe_req_list;
                                                                                                                                         
/*
 * Define function..
 */

extern int init_handler();
extern int init_request(); 


extern int add_req(char *req_name, void *req_param_struct);
extern int del_req(TR_cpe_req_list *req);
extern int count_req_list(struct list_head  *_lst);
extern TR_cpe_req_list *search_req_node(char *req_name);
extern int insert_request(TR_cpe_req_list *req, TRF_node *xmlroot);

#endif  /* HANDLER_H_ */
