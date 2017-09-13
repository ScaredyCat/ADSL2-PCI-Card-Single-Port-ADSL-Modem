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

#ifndef METHODS_H_
#define METHODS_H_

/*
 * Include necessary file......
 */
#include "../soap/soap.h"


/*
 * Define return value
 */

#define MTH_RET_SUCCESS 0
#define MTH_RET_FAILED  -1

/*
 * Define Constant value
 */
#define DONT_NEED_REBOOT 0
#define NEED_REBOOT      1

/*
 * Define support method name .....
 */
#define VENDOR
#define m0  "GetRPCMethods"
#define m1  "SetParameterValues";
#define m2  "GetParameterValues";
#define m3  "GetParameterNames";
#define m4  "GetParameterAttributes";
#define m5  "SetParameterAttributes";
#define m6  "AddObject";
#define m7  "DeleteObject";
#define m8  "Reboot";
#define m9  "Download";
#define m10 "Upload"

#ifdef VENDOR
#define m11  NULL
#define m12  NULL
#define m13  NULL
#define m14  NULL
#define m15  NULL
#define m16  NULL
#define m17  NULL
#define m18  NULL
#define m19  NULL
#endif

/*
 * data type .....
 */
#define COMMAND_KEY_LEN 32
#define PARAM_FULL_NAME_LEN 256
#define GET_PARAM_VAL_LEN 256

typedef struct transfercomplete {
    char                 command_key[COMMAND_KEY_LEN + 1];    //string(32)
    TR_fault_struct      fault_struct;                        //FaultStruct
    time_t               start_time;                          //dateTime
    time_t               complete_time;                       //dateTime
}TR_tran_comp;

/*
 * Delcear function .........
 */
extern int call_dev_func(char *func_name, int opt_flag, void *value, int locate[]);
extern int process_set_param_val(TRF_node *method);
extern int process_get_param_val(TRF_node *method);
extern int gen_tran_comp_method(TR_tran_comp *tran, TRF_node *xmlroot);
extern int process_tran_comp_resp(TRF_node *method);
extern int process_soap_fault(TRF_node *method);
extern int process_getparamname(TRF_node * method);
extern int process_getrpc(TRF_node *method);
extern int process_reboot(TRF_node *);

#endif  /* METHODS_H_ */

