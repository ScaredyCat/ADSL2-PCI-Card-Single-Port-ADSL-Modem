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

#ifndef UPLOAD_H_
#define UPLOAD_H_

/*
 * Include necessary file ....
 */

#include "../soap/soap.h"
#include "../cpe_task_list.h"
#include "../../tools/logger.h"
#include "../../res/global_res.h"


/*
 * Data Types
 */

#define UP_MAX_DATA_SIZE 13660   //(UP_FILE_LENGTH/3+2)*4
#define UP_TIME_OUT    20
#define UP_FILE_LENGTH 10240

typedef struct upload
{
    char command_key[33];
    char file_type[65];
    char url[257];
    char username[257];
    char password[257];
    unsigned int   delay_seconds;
}TR_upload;

typedef struct uploadresp
{
    int     status;
    time_t  start_time;
    time_t  complete_time;
}TR_upload_resp;

/*
 * Prototypes.....
 */

int process_ul(TRF_node *method);
int trf_upload(TR_upload *up, time_t *starttime, time_t *completetime);

#endif /*UPLOAD_H_*/
