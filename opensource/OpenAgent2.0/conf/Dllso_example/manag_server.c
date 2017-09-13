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
 * $Author: joinsonj $
 * $Date: 2007-06-08 02:59:21 $
 */

#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#define agent_conf_path "/etc/conf/agent.conf"
#define AGENT_CONF_SUCCESS 0
#define AGENT_CONF_FAILED  -1

//Define struct
typedef struct {
    unsigned char acs_url[256];
    unsigned char acs_username[256];
    unsigned char acs_password[256];
    unsigned short periodic_inform_enable;
    unsigned int periodic_inform_interval;
    unsigned char periodic_inform_time[24];
    unsigned char parameterkey[32];
    unsigned int conn_req_ser_port;             // 0 ~ 65535
    unsigned char conn_req_ser_username[256];
    unsigned char conn_req_ser_password[256];
    unsigned int retry_times;
    unsigned char command_key[33];
    unsigned int flag_reboot;
}agent_conf;


static int set_agent_conf(agent_conf *pconf);
static int get_agent_conf(agent_conf *pconf);
static int dev_get_wan_ip(char *ip_addr);

//manageserver
int dev_ManageServer_URL(int opt_flag, void *mthdval_struct, int locate[])
{
    agent_conf a_conf;
    int ret = -1;
    
    if (get_agent_conf(&a_conf) == AGENT_CONF_FAILED) {
        return ret;
    }
    if (!opt_flag) {
        strcpy((char *)mthdval_struct, a_conf.acs_url);
        ret = 0;
    } else {
        strcpy(a_conf.acs_url, (char *)mthdval_struct);
        if (set_agent_conf(&a_conf) == AGENT_CONF_SUCCESS) {
            ret = 0;
        }
    }
    
    return ret;
}
int dev_ManageServer_Username(int opt_flag, void *mthdval_struct, int locate[])
{
    agent_conf a_conf;
    int ret = -1;

    if (!opt_flag) {
        printf("ManageServer username is Unreadable\n");
    } else {
        if (get_agent_conf(&a_conf) == AGENT_CONF_SUCCESS) {
            strcpy(a_conf.acs_username, (char *)mthdval_struct);
            if (set_agent_conf(&a_conf) == AGENT_CONF_SUCCESS) {
                ret = 0;
            }
        }
    }

    return ret;
}
int dev_ManageServer_Password(int opt_flag, void *mthdval_struct, int locate[])
{
    agent_conf a_conf;
    int ret = -1;

    if (!opt_flag) {
        strcpy((char *)mthdval_struct, "");
        ret = 0;
    } else {
        if (get_agent_conf(&a_conf) == AGENT_CONF_SUCCESS) {
            strcpy(a_conf.acs_password, (char *)mthdval_struct);
            if (set_agent_conf(&a_conf) == AGENT_CONF_SUCCESS) {
                ret = 0;
            }
        }
    }

    return ret;
}
int dev_ManageServer_PeriodicInformEnable(int opt_flag, void *mthdval_struct, int locate[])
{
    agent_conf a_conf;
    int ret = -1;

    if (get_agent_conf(&a_conf) == AGENT_CONF_FAILED) {
        return ret;
    }
    if (!opt_flag) {
        *(int *)mthdval_struct = a_conf.periodic_inform_enable;
        ret = 0;
    } else {
        a_conf.periodic_inform_enable = *(unsigned short *)mthdval_struct;
        if (set_agent_conf(&a_conf) == AGENT_CONF_SUCCESS) {
            ret = 1;
        }
    }

    return ret;
}
int dev_ManageServer_PeriodicInformInterval(int opt_flag, void *mthdval_struct, int locate[])
{
    agent_conf a_conf;
    int ret = -1;

    if (get_agent_conf(&a_conf) == AGENT_CONF_FAILED) {
        return ret;
    }
    if (!opt_flag) {
        *(unsigned int *)mthdval_struct = a_conf.periodic_inform_interval;
        ret = 0;
    } else {
        a_conf.periodic_inform_interval = *(unsigned int *)mthdval_struct;
        if (set_agent_conf(&a_conf) == AGENT_CONF_SUCCESS) {
            ret = 1;
        }
    }

    return ret;
}
int dev_ManageServer_PeriodicInformTime(int opt_flag, void *mthdval_struct, int locate[])
{
    agent_conf a_conf;
    //time_t temp;
    int ret = -1;

    if (get_agent_conf(&a_conf) == AGENT_CONF_FAILED) {
        return ret;
    }
    if (!opt_flag) {
        strcpy((char *)mthdval_struct, a_conf.periodic_inform_time);
        ret = 0;
    } else {
        strcpy(a_conf.periodic_inform_time, (char *)mthdval_struct);
        if (set_agent_conf(&a_conf) == AGENT_CONF_SUCCESS) {
            /*temp = strtimetosec((char *)mthdval_struct);
            if (temp != -1) {
                set_period_time(temp);
                ret = 1;
            } else {
                printf("Transfer time to seconds error\n");
            }*/
            ret = 1;
        }
    }

    return ret;
}
int dev_ManageServer_ParameterKey(int opt_flag, void *mthdval_struct, int locate[])
{
    agent_conf a_conf;
    int ret = -1;

    if (get_agent_conf(&a_conf) == AGENT_CONF_FAILED) {
        return ret;
    }
    if (!opt_flag) {
        strcpy((char *)mthdval_struct, a_conf.parameterkey);
        ret = 0;
    } else {
        strcpy(a_conf.parameterkey, (char *)mthdval_struct);
        if (set_agent_conf(&a_conf) == AGENT_CONF_SUCCESS) {
            ret = 0;
        }
    }

    return ret;
}
int dev_ManageServer_ConnectionRequestURL(int opt_flag, void *mthdval_struct, int locate[])
{
    agent_conf a_conf;
    int ret = -1;
    char wan_ipaddr[32];
    
    memset(wan_ipaddr, 0, sizeof(wan_ipaddr));
    
    if (!opt_flag) {
        if (get_agent_conf(&a_conf) == AGENT_CONF_SUCCESS) {
            if (dev_get_wan_ip(wan_ipaddr) == 0) {
                sprintf((char *)mthdval_struct, "http://%s:%d/%d", wan_ipaddr, a_conf.conn_req_ser_port, 0);
            }
            ret = 0;
        }
    } else {
        printf("Connection Request URL is Unwriteable\n");
    }

    return ret;
}
int dev_ManageServer_ConnectionRequestUsername(int opt_flag, void *mthdval_struct, int locate[])
{
    agent_conf a_conf;
    int ret = -1;

    if (get_agent_conf(&a_conf) == AGENT_CONF_FAILED) {
        return ret;
    }
    if (!opt_flag) {
        strcpy((char *)mthdval_struct, a_conf.conn_req_ser_username);
        ret = 0;
    } else {
        strcpy(a_conf.conn_req_ser_username, (char *)mthdval_struct);
        if (set_agent_conf(&a_conf) == AGENT_CONF_SUCCESS) {
            ret = 0;
        }
    }

    return ret;
}
int dev_ManageServer_ConnectionRequestPassword(int opt_flag, void *mthdval_struct, int locate[])
{
    agent_conf a_conf;
    int ret = -1;

    if (!opt_flag) {
        strcpy((char *)mthdval_struct, "");
        ret = 0;
    } else {
        if (get_agent_conf(&a_conf) == AGENT_CONF_SUCCESS) {
            strcpy(a_conf.conn_req_ser_password, (char *)mthdval_struct);
            if (set_agent_conf(&a_conf) == AGENT_CONF_SUCCESS) {
                ret = 0;
            }
        }
    }

    return ret;
}
int dev_ManageServer_UpgradesManaged(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_ManageServer_KickURL(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_ManageServer_DownloadProgressURL(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}


static int dev_get_wan_ip(char *ip_addr)
{
    int s;
    struct ifconf conf;
    struct ifreq *ifr;
    char buff[2048];
    int num;
    int i;

    s = socket(PF_INET, SOCK_DGRAM, 0);
    conf.ifc_len = 2048;
    conf.ifc_buf = buff;

    ioctl(s, SIOCGIFCONF, &conf);
    num = conf.ifc_len / sizeof(struct ifreq);
    ifr = conf.ifc_req;

    for(i=0;i < num;i++)
    {
        struct sockaddr_in *sin = (struct sockaddr_in *)(&ifr->ifr_addr);

        ioctl(s, SIOCGIFFLAGS, ifr);
        if(((ifr->ifr_flags & IFF_LOOPBACK) == 0) && (ifr->ifr_flags & IFF_UP))
        {
            sprintf(ip_addr, "%s", inet_ntoa(sin->sin_addr));
            close(s);
            return 0;
        }
        ifr++;
    }
    close(s);
    return 0;
}


static int get_agent_conf(agent_conf *pconf)
{
    FILE *fp = NULL;
    char info_buf[4096];
    char *pstr = NULL, *pstr_bk = NULL;
    char tmp[32];
    int i = 0;

    //init variable
    memset(info_buf, 0, sizeof(info_buf));
    memset(pconf, 0, sizeof(agent_conf));

    if (access(agent_conf_path, F_OK) == -1) {
        printf( "File %s not exist\n", agent_conf_path);
        return AGENT_CONF_FAILED;
    }


    fp = fopen(agent_conf_path, "r");
    if (fp == NULL) {
        printf( "Can't open file %s\n", agent_conf_path);
        return AGENT_CONF_FAILED;
    }
    printf("open agent.conf success\n");

    if (fread(info_buf, sizeof(char), sizeof(info_buf), fp) <= 0) {
        printf( "Read file %s error\n", agent_conf_path);
        fclose(fp);
        return AGENT_CONF_FAILED;
    }
    printf("read success\n");

    if (fclose(fp) != 0) {
        printf("ERROR: close file agent.conf failed\n");
        return AGENT_CONF_FAILED;
    }
    printf("close success\n");

    //get parameter value
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

static int set_agent_conf(agent_conf *pconf)
{
    FILE *fp = NULL;

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

    if (fclose(fp) != 0) {
        printf("ERROR: close agent.conf failed\n");
        return AGENT_CONF_FAILED;
    }

    return AGENT_CONF_SUCCESS;
}

