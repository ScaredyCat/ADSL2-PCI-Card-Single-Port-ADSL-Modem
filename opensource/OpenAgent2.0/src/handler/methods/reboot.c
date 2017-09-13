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

#include "methods.h"
#include "../../tools/agent_conf.h"
#include "../cpe_task_list.h"

static int get_reboot_command_key(TRF_node *, char *);
static int gen_reboot_resp();
static int process_reboot_info(TRF_node * _method);

typedef struct reboot {
    char command_key[COMMAND_KEY_LEN + 1];
}TR_reboot;

/*
 ***************************************************************************
 * Function name: process_reboot(TRF_node *)
 * Description:  porcess reboot method
 * Parameter:
 *		TRF_node type pointer
 * return value:
 *		return code: 0, -1
 ***************************************************************************
 */
int process_reboot(TRF_node *method)
{
    int res = 0;

    res = process_reboot_info(method);
    if (res != 0) {
        //generate method fault
        if (gen_method_fault(xmlroot, "Reboot", INTERNAL_ERROR) == NULL) {
            return -1;
        }
        return 0;
    }
    //generate reboot response
    res = gen_reboot_resp();
    if (res != 0) {
        return -1;
    }
    
    return 0;
}

/*
 ***************************************************************************
 * Function name: process_reboot_info(TRF_node *)
 * Description:  porcess reboot info
 * Parameter:
 *              TRF_node type pointer
 * return value:
 *              return code: 0, -1
 ***************************************************************************
 */
static int process_reboot_info(TRF_node * _method)
{
    TR_reboot tr_reboot;
    int res = 0;

    //check _method is invalid
    if ( _method == NULL ) {
        return -1;
    }
	
    //check get_reboot_command_key() return value is invalid
    if ( get_reboot_command_key(_method, tr_reboot.command_key) != 0 ) {
        return -1;
    }

    //if( write_reboot_conf(tr_reboot.command_key) != 0 )
    /*if (dev_reboot_cmdkey(1, tr_reboot.command_key) != 0) {
        return -1;
    }*/
    //set commandkey value
    agent_conf a_conf;
    
    if (get_agent_conf(&a_conf) != AGENT_CONF_SUCCESS) {
        return -1;
    }
    strcpy(a_conf.command_key, tr_reboot.command_key);
    a_conf.flag_reboot = 1;
    
    if (set_agent_conf(&a_conf) != AGENT_CONF_SUCCESS) {
        return -1;
    }

    //add reboot to task list
    res = add_task_list("dev_reboot", NULL);
    if (res != 0) {
        //LOG(m_handler, ERROR, "Add dev_reboot task to task list failed\n");
        return -1;
    }

    return 0;
}

/*
 **************************************************************************************
 * Function: get_reboot_command_key(TRF_node *, char *) 
 * Description: get reboot struct element value
 * Parameter:
 *	TRF_node type pointer, char type pointer
 * return value:
 *	return code: 0, -1
 **************************************************************************************
 */

static int get_reboot_command_key(TRF_node * _method, char * _command_key)
{
    int ret = -1;

    _method = _method->child;
	
    while (_method != NULL) {
    	if ( !strcmp(_method->value.element.name, "CommandKey") ) {
	    if (_method->child != NULL) {
                strcpy(_command_key, _method->child->value.opaque);
            }
	    else {
	        strcpy(_command_key, "");
            }
	    ret = 0;
			
	    break;
	}
	_method = _method->child;
    }

    return ret;
}


/*
 ****************************************************************************
 * Function: gen_reboot_resp() -- make reboot response
 * Parameter: 
 *		none
 * return value:
 *		return code: 0, -1
 ***************************************************************************
 */
static int gen_reboot_resp()
{
    char name[] = "RebootResponse";
    
    //generate soap frame
    if (gen_soap_frame(name) == NULL) {
        return -1;
    }
    
    return 0;
}

