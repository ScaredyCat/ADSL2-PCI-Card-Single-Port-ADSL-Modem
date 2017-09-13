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
  *******************************************************************************************
  * $Author: jasonw $
  * history:$Date: 2007-06-08 02:22:47 $
  *******************************************************************************************
  */
 
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#include "logger.h"

static FILE *log = NULL;

/* Global Varibles */
int log_out_mode = 0;          //pattern chose to output, 0-stdout:1-logger:>2-stdout&logger
int def_level = DEBUG;         //defined start level,only less than it will logger
int log_size = 50;             //control the log file size
int log_module = all;          //default module is all module

/*
 ********************************************************************************************
 * logger function:write log
 * arguments:
 *     level: log start-level 
 *     file: log occur which file 
 *     line: log occur which line
 *     function: log occur which function 
 *     fmt,...: support variable numbers of parameter
 * return value:
 *     void
 ********************************************************************************************
 */

int logger(int level, const char *file, int line, const char *function, const char *fmt, ...)
{
    FILE *logger = NULL;
    va_list ap;
    char time_buf[25];   
    int  flags = 1; 
    time_t time_now;
    struct tm *timenow;
    struct stat stat_buf;     

    //start to parse variable arguments
    va_start(ap, fmt);
    time(&time_now);
    timenow = localtime(&time_now);
    strcpy(time_buf, asctime(timenow));
    time_buf[sizeof(time_buf) - 1] = '\0';
    
    //judge whether log file openned
    if (log == NULL) {
        printf("%s,logger.c[72](logger):log file has not been open, must initial logger before use LOG(...)\n", time_buf);
        return -1;
    }

    //judge the size of log file, if above max size, backup and rewrite log file
    if (stat(log_file_path, &stat_buf) < 0) {
        printf("%s,logger.c[71](logger):read log file failure\n", time_buf);
    }

    if (stat_buf.st_size > log_size*1024) { 
        fclose(log);
        
        //backup log file
        if (rename(log_file_path, log_file_bak) == -1) {
            printf("%s,logger.c[81](logger):%s\n", time_buf, strerror(errno));
            return -1;
        }
        
        //creat a new log file
        log = fopen (log_file_path, "a");
        if (!log) {
            fprintf(stderr, "%s, logger.c[89](logger): %s\n", time_buf, strerror(errno));
            return -1;
        }           
    }
   
                                                                                                                        
    //only output in screen
    if (log_out_mode == 0) { 
        logger = stderr;
        if (flags) {               
            fprintf(logger, "%s,%s[%d](%s): ", time_buf,file, line, function);
            vfprintf(logger, fmt, ap);
         }
    }
  
    //only write in log
    else if (log_out_mode == 1) {
	     logger = log;
	     if (flags) {	       
	         fprintf(logger, "%s,%s[%d](%s): ", time_buf, file, line, function);
	         vfprintf(logger, fmt, ap);	
	      }
          }

          //output in screen and in log
          else if (log_out_mode == 2) {
                   if (flags) {    
                       logger = stderr;
                       fprintf(logger, "%s,%s[%d](%s): ", time_buf, file, line, function);
                       vfprintf(logger, fmt, ap);
                       fflush(logger);

                       logger = log;
                       fprintf(logger, "%s,%s[%d](%s): ", time_buf, file, line, function);
                       vfprintf(logger, fmt, ap);
                     }
                }

    fflush (logger);

    //end to parse variable arguments
    va_end (ap);  
     
    return 0;
}

/*
 ********************************************************************************************
 * init_logger function: open log file
 * arguments:
 *     void
 * retuen value: 
 *     if right return 0, else return -1
 ********************************************************************************************
 */

int init_logger(void)
{ 
    log = fopen(log_file_path, "a");
    if (!log) {
        fprintf(stderr, "logger.c[155](init_logger):Create log file, %s\n", strerror(errno));
        return -1;
    }
   
    return 0;	
}

/*
 ********************************************************************************************
 * exit_logger function: close the log file
 * arguments:
 *     void 
 * return value:
 *     void
 ********************************************************************************************
 */

void exit_logger(void)
{
    LOG(all, DEBUG, "Closed logger\n");
    if (log)
        fclose(log);
}

/*
 ********************************************************************************************
 * logger_set  function: change the log level and output mode
 * arguments:
 *    level:   log start level
 *    mode:    log output mode
 *    size:size of log file
 *    module:  which module's log message will be out
 * return value:
 *    right return 0
 *    wrong return -1
 ********************************************************************************************
 */

int set_logger(int level, int mode, int size, int module)
{
    LOG(m_tools, DEBUG, "received parameter value\nlevel=%d mode=%d log_size= %d module=%d\n", level, mode, size, module);
    
    def_level = level;
    log_out_mode = mode;
    if (size != 0)
        log_size = size;
    log_module = module;
   
    LOG(m_tools, DEBUG, "after modified parameter value\nlevel=%d mode=%d log_size= %d module=%d\n", def_level, log_out_mode, log_size, log_module);
        
    return 0;  
}

