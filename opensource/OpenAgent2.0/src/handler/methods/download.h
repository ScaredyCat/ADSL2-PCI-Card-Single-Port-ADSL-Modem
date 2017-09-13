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
 
#ifndef DOWNLOAD_H_
#define DOWNLOAD_H_

/*
 * Include necessary headers...
 */

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "../../res/global_res.h"
#include "../soap/soap.h"
#include "../soap/http.h"

/*
 * Data types...
 */

typedef struct download
{
    char   command_key[33];
    char   file_type[65];
    char   url[257];
    char   username[257];
    char   password[257];
    unsigned int   file_size;
    char   target_file_name[257];
    unsigned int   delay_seconds;
    char   success_url[257];
    char   failure_url[257];
}TR_download;

typedef struct dowmloadresp
{
    int     status;
    time_t  start_time;
    time_t  complete_time;
}TR_download_resp;

char *imageptr;
int dl_len;
int imagetype;

int download_at_once_flag;

/*
 * macrodefinition
 */
 
#define first_reboot  0         // 1 -- first reboot before download, 0 -- first download before reboot
#define filesize      100       // The size of the file which will be download
#define delayseconds  10        // The number of seconds from the time this method id called to the 
                                // time the cpe is request to init the download
#define MAX_DATA_SIZE 20480 
#define TEMP_FILE_NAME "download_file"
#define DOWN_TIME_OUT 15

#define TRF_DOWNLOAD_IMAGE 0
#define TRF_DOWNLOAD_WEB   1
#define TRF_DOWNLOAD_SETTINGS 2

#define DOWNLOAD_OVERHEAD 256
#define MIN_DOWNLOAD_LEN 2048

#define NO_FORMAT 0
#define IMAGE_FORMAT 1
#define WEB_FORMAT 2
#define CONFIG_FORMAT 3
/* 
 * Prototypes
 */

int process_dl(TRF_node *method);
int trf_download(TR_download *down, time_t *starttime, time_t *completetime);
#endif  /* DOWNLOAD_H_ */

