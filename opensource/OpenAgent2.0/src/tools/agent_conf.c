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
 * Include files
 */

#include "agent_conf.h"

/*
 ***********************************************************************
 * Function name: get_agent_conf
 * Description: get the basic configure infoemation of agent
 * Parameter: agent_conf *pconf -- pointer to agent configure struct
 * Return Value: success return AGENT_CONF_SUCCESS
 *               failed return AGENT_CONF_FAILED
 ***********************************************************************
 */
int get_agent_conf(agent_conf *pconf)
{
    FILE *fp = NULL;
    char info_buf[4096];
    char *pstr = NULL, *pstr_bk = NULL;
    char tmp[32];
    int i = 0;

    /*init variable*/
    memset(info_buf, 0, sizeof(info_buf));
    memset(pconf, 0, sizeof(agent_conf));

    printf("agent_conf_path:%s\n", agent_conf_path);
    if (access(agent_conf_path, F_OK) == -1) {
        printf( "File %s not exist\n", agent_conf_path);
        return AGENT_CONF_FAILED;
    }

    fp = fopen(agent_conf_path, "r");
    if (fp == NULL) {
        printf( "Can't open file %s\n", agent_conf_path);
        return AGENT_CONF_FAILED;
    }

    if (fread(info_buf, sizeof(char), sizeof(info_buf), fp) <= 0) {
        printf( "Read file %s error\n", agent_conf_path);
        fclose(fp);
        return AGENT_CONF_FAILED;
    }
    
    fclose(fp);
    
    /*get parameter value*/
    pstr = info_buf;
    while (pstr != NULL) {
        pstr_bk = strstr(pstr, "\n");
        if (pstr_bk != NULL) {
            memset(tmp, 0, sizeof(tmp));
            i++;
            switch (i) {
                case 1:
                    strncpy(pconf->acs_url, pstr, pstr_bk - pstr);
                    break;
                case 2:
                    strncpy(pconf->acs_username, pstr, pstr_bk - pstr);
                    break;
                case 3:
                    strncpy(pconf->acs_password, pstr, pstr_bk - pstr);
                    break;
                case 4:
                    strncpy(tmp, pstr, pstr_bk - pstr);
                    pconf->periodic_inform_enable = atoi(tmp);
                    break;
                case 5:
                    strncpy(tmp, pstr, pstr_bk - pstr);
                    pconf->periodic_inform_interval = atoi(tmp);
                    break;
                case 6:
                    strncpy(pconf->periodic_inform_time, pstr, pstr_bk - pstr);
                    break;
                case 7:
                    strncpy(pconf->parameterkey, pstr, pstr_bk - pstr);
                    break;
                case 8:
                    strncpy(tmp, pstr, pstr_bk - pstr);
                    pconf->conn_req_ser_port = atoi(tmp);
                    break;
                case 9:
                    strncpy(pconf->conn_req_ser_username, pstr, pstr_bk - pstr);
                    break;
                case 10:
                    strncpy(pconf->conn_req_ser_password, pstr, pstr_bk - pstr);
                    break;
                case 11:
                    strncpy(tmp, pstr, pstr_bk - pstr);
                    pconf->retry_times = atoi(tmp);
		    break;
                case 12:
                    strncpy(pconf->command_key, pstr, pstr_bk - pstr);
                    break;
                case 13:
                    strncpy(tmp, pstr, pstr_bk - pstr);
                    pconf->flag_reboot = atoi(tmp);
                    break;
                default:
                    printf("ERROR\n");
                    return AGENT_CONF_FAILED;
            }
            pstr = pstr_bk + 1;
        } else {
            pstr = pstr_bk;
        }
    }

    return AGENT_CONF_SUCCESS;
}

/*
 ************************************************************************
 * Function name:set_agent_conf()
 * Description: set the agnet basic configure information to file
 * Parameter: agent_conf *pconf -- pointer to the agent configure struct
 * Return value:
 *     Success return AGENT_CONF_SUCCESS
 *     Failed return  AGENT_CONF_FAILED
 ************************************************************************
 */

int set_agent_conf(agent_conf *pconf)
{
    FILE *fp = NULL;

    /*if (access(CONF_PATH, F_OK) == -1) {
        printf("File %s not exist\n", CONF_PATH);
        return AGENT_CONF_FAILED;
    }*/

    fp = fopen(agent_conf_path, "w");
    if (fp == NULL) {
        printf( "Can't open file %s\n", agent_conf_path);
        return AGENT_CONF_FAILED;
    }


    fprintf(fp, "%s\n%s\n%s\n%d\n%d\n%s\n%s\n%d\n%s\n%s\n%d\n%s\n%d\n",
                 pconf->acs_url, pconf->acs_username, pconf->acs_password,
                 pconf->periodic_inform_enable, pconf->periodic_inform_interval,
                 pconf->periodic_inform_time, pconf->parameterkey,
                 pconf->conn_req_ser_port, pconf->conn_req_ser_username,
                 pconf->conn_req_ser_password, pconf->retry_times, 
                 pconf->command_key, pconf->flag_reboot);

    fclose(fp);

    return AGENT_CONF_SUCCESS;
}

/*
 *********************************************************************
 * Function name: init_agent_conf()
 * Description: init the agent configure file
 * Parameter: void
 * Retrun value:
 *     SUccess retrun AGENT_CONF_SUCCESS
 *     Failed  return AGENT_CONF_FAILED
 *********************************************************************
 */

int init_agent_conf()
{
    agent_conf a_conf;

    if (access(agent_conf_path, F_OK) == -1) {
        printf( "File %s is not exist\n", agent_conf_path);
        //Use default value to write configure file
        memset(&a_conf, 0, sizeof(a_conf));

        strcpy(a_conf.acs_url, ACS_URL);
        strcpy(a_conf.acs_username, ACS_USERNAME);
        strcpy(a_conf.acs_password, ACS_PASSWORD);

        a_conf.periodic_inform_enable = PERIODIC_INFORM_ENABLE;
        a_conf.periodic_inform_interval = PERIODIC_INFORM_INTERVAL;
        strcpy(a_conf.periodic_inform_time, PERIODIC_INFORM_TIME);

        strcpy(a_conf.parameterkey, PARAMETERKEY);

        a_conf.conn_req_ser_port = CONN_REQ_SER_PORT;
        strcpy(a_conf.conn_req_ser_username, CONN_REQ_SER_USERNAME);
        strcpy(a_conf.conn_req_ser_password, CONN_REQ_SER_PASSWORD);
   
        a_conf.retry_times = RETRY_TIMES;
        strcpy(a_conf.command_key, COMMAND_KEY);
        a_conf.flag_reboot = FLAG_REBOOT;

        /*write to configure file*/
        if (set_agent_conf(&a_conf) == AGENT_CONF_FAILED) {
            printf("Set agent info error\n");
            return AGENT_CONF_FAILED;
        }
    }
  
    return AGENT_CONF_SUCCESS;
}

