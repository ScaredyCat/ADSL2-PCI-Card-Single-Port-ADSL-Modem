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
 ********************************************************************************************
 * $Author: jasonw $
 * history:$Date: 2007-06-08 02:22:44 $
 ********************************************************************************************
 */

/*
 ********************************************************************************************
 *  how to use logger
 *  For example:
 *  char *s = "hello";
 *  if (s)
 *      n_LOG (m_auth, INFO, "we have a string: %s\n", s);
 * 
 *  see follow message afer run program
 *    we have a string: hello
 *
 *  more detail reference to manual
 ********************************************************************************************
 */
 
#ifndef LOGGER_H_
#define LOGGER_H_ 

// GLOBAL Varibles                                                                              
int def_level;
int log_out_mode;
int log_size;
int log_module;

#include <stdio.h>
#include <stdarg.h>

#include "../res/global_res.h"

// Log file, default is ./logger.log, the path may be change in future
//#define LOG_FILE  "./cpe.log"
//#define LOG_FILE_BAK  "./cpe.log.bak"

// Log levels 
#define FATAL 0
#define	ERROR 1
#define	WARN  2
#define INFO  3
#define DEBUG 4

// modules

#define all 0
#define m_auth 1
#define m_bin  2
#define m_CLI  3
#define m_comm  4
#define m_daemon  5
#define m_device  6
#define m_event  7
#define m_handler  8
#define m_init  9
#define m_protocols 10
#define m_res  11
#define m_tools  12

// GLOBAL MACROS
#undef 	LOG_ARGS

//macro of current file ,line, and function name
#define LOG_ARGS __FILE__, __LINE__, __FUNCTION__
#undef LOG   
#define LOG(module, level, fmt, args...)                              \
        switch(log_module) {                                            \
            case 0:                                                     \
                  do {                                                  \
                      if (level <= def_level) {                         \
                         logger (level, LOG_ARGS, fmt, ## args);        \
                       }                                                \
                  } while (0);                                          \
                  break;                                                \
             default:                                                   \
                  if (module == log_module)                             \
                  {                                                     \
                      do {                                              \
                          if (level <= def_level) {                     \
                              logger (level, LOG_ARGS, fmt, ## args);   \
                          }                                             \
                      } while (0);                                      \
                  }                                                     \
        }

// FUNCTION PROTOTYPES
int logger(int level, const char *file, int line, const char* function, const char *fmt,...);
int  init_logger(void);
void exit_logger(void);
int set_logger(int level, int mode, int size, int module);

#endif

