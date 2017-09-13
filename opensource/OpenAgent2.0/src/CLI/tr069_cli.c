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

/***********************************************************************
*
*	include file
*
***********************************************************************/

#include "tr069_cli.h"

/***********************************************************************
*
*	function prototype
*
***********************************************************************/

static inline unsigned long ewt_mktime(struct tm *tm);
 
extern char *strptime (__const char *__restrict __s,
                       __const char *__restrict __fmt, struct tm *__tp)
     __THROW;
static int get_agent_conf(agent_conf *pconf);
static int set_agent_conf(agent_conf *pconf);
static int init_agent_conf();
/************************************************************************
*
*	global var define
*
************************************************************************/
char agent_conf_path[256];
/*
 ***********************************************************************
 * Function name: get_agent_conf
 * Description: get the basic configure infoemation of agent
 * Parameter: agent_conf *pconf -- pointer to agent configure struct
 * Return Value: success fail
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

    if (access(agent_conf_path, F_OK) == -1) {
        printf( "File %s not exist\n", agent_conf_path);
        return FAIL;
    }

    fp = fopen(agent_conf_path, "r");
    if (fp == NULL) {
        printf( "Can't open file %s\n", agent_conf_path);
        return FAIL;
    }

    if (fread(info_buf, sizeof(char), sizeof(info_buf), fp) <= 0) {
        printf( "Read file %s error\n", agent_conf_path);
        fclose(fp);
        return FAIL;
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
                    return FAIL;
            }
            pstr = pstr_bk + 1;
        } else {
            pstr = pstr_bk;
        }
    }

    return SUCCESS;
}

/*
 ************************************************************************
 * Function name:set_agent_conf()
 * Description: set the agnet basic configure information to file
 * Parameter: agent_conf *pconf -- pointer to the agent configure struct
 * Return value: Success Fail
 ************************************************************************
 */

int set_agent_conf(agent_conf *pconf)
{
    FILE *fp = NULL;

    fp = fopen(agent_conf_path, "w");
    if (fp == NULL) {
        printf( "Can't open file %s\n", agent_conf_path);
        return FAIL;
    }

    fprintf(fp, "%s\n%s\n%s\n%d\n%d\n%s\n%s\n%d\n%s\n%s\n%d\n%s\n%d\n",
                 pconf->acs_url, pconf->acs_username, pconf->acs_password,
                 pconf->periodic_inform_enable, pconf->periodic_inform_interval,
                 pconf->periodic_inform_time, pconf->parameterkey,
                 pconf->conn_req_ser_port, pconf->conn_req_ser_username,
                 pconf->conn_req_ser_password, pconf->retry_times, 
                 pconf->command_key, pconf->flag_reboot);

    fclose(fp);

    return SUCCESS;
}
/*
 *********************************************************************
 * Function name: init_agent_conf()
 * Description: init the agent configure file
 * Parameter: void
 * Retrun value:  SUCCESS  FAIL
 *********************************************************************
 */

int init_agent_conf()
{
    agent_conf a_conf;

    if (access(agent_conf_path, F_OK) == -1) {
        printf( "File %s is not exist\n", agent_conf_path);
        /*Use default value to write configure file*/
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
        if (set_agent_conf(&a_conf) == FAIL) {
            printf("Set agent info error\n");
            return FAIL;
        }
    }
    
    //printf( "Init agent conf Success\n");

    return SUCCESS;
}

/*
 ********************************************************************
 * Function name: ewt_mktime();
 * Description: time transition
 * Parameter: tm
 * Return value: time
 *********************************************************************
 */
static inline unsigned long ewt_mktime(struct tm *tm)
{
    tm->tm_year += 1900;
    tm->tm_mon += 1;

    if (0 >= (int) (tm->tm_mon -= 2)) {  //1..12 -> 11,12,1..10
        tm->tm_mon += 12;                // Puts Feb last since it has leap day
        tm->tm_year -= 1;
    }

    return ((( (unsigned long) (tm->tm_year/4 - tm->tm_year/100 + tm->tm_year/400 + 367*tm->tm_mon/12 + tm->tm_mday) + tm->tm_year*365 - 719499 )*24 + tm->tm_hour)*60 + tm->tm_min)*60 + tm->tm_sec;

}
/*
 ********************************************************************
 * Function name: strtimetosec();
 * Description: time transition
 * Parameter: strtm
 * Return value: time
 *********************************************************************
 */
time_t strtimetosec(char *strtm)
{
    char *format = "%Y-%m-%dT%T";
    struct tm tm;
    time_t temp;

    if (strptime(strtm, format, &tm) == NULL){
        printf("strptime error\n");
        return -1;
    }

    temp = ewt_mktime(&tm);

    return temp;
}
/*
 ********************************************************************
 * Function name: Creat_msg();
 * Description: Creat message queue.
 * Parameter: 
 * Return value: 0:SUCCESS  -1:FAIL
 *********************************************************************
 */
int creat_msgq()
{
    key_t mqkey;
    int oflag,mqid;

    char filenm[] = "agent_mq";
    oflag = IPC_CREAT;

    mqkey = ftok(filenm,PROJID);
    mqid = msgget(mqkey, 0);
    if (mqid == -1){
        printf("msgget error.\n");
        return -1;
    }
    return mqid;
}
/*
 ********************************************************************
 * Function name: send_msg();
 * Description: Send message queue.
 * Parameter: int mqid:id of mq; long type: type of send mq; char *text:text
 * Return value: 0:SUCCESS  -1:FAIL
 *********************************************************************
 */
int send_msg(int mqid, long type, char *text)
{
    int res = 0;

    struct msgbuf {
        long mtype;
        char mtext[MAX_SEND_SIZE];
    }msg;
    msg.mtype = 2;
    strcpy(msg.mtext, text);
    res = msgsnd(mqid, &msg, strlen(msg.mtext) + 1, 0);
    if(res == -1) {
        perror("msgsnd:.\n");
        return -1;
    }
    return 0;
}
/*
 ********************************************************************
 * Function name: isvalidvalue();
 * Description: check value is valid.
 * Parameter: value
 * Return value: 0:SUCCESS  -1:FAIL
 *********************************************************************
 */
int isvalidvalue(const char *value)
{
    int i = 0, size =0;
    int status = -1;
    
    size = strlen(value);
    for(i = 0; i < size; i++){
        if(!isalnum(value[i]))
            break;
    }
    if(size > 0 && i == size)
        status = 0;
    return status;
}
/*
 ********************************************************************
 * Function name: verifyLength();
 * Description: check length of value is valid.
 * Parameter: value, max: max lenth
 * Return value: 0:SUCCESS  -1:FAIL
 *********************************************************************
 */
int verifyLength(const char *value, unsigned int max)
{
    int status = -1;
    if(strlen(value) < max)
        status = 0;
    return status;
}
/*
 ********************************************************************
 * Function name: verifyNumber();
 * Description: check value is number.
 * Parameter: value
 * Return value: 0:SUCCESS  -1:FAIL
 *********************************************************************
 */
int verifyNumber(const char *value)
{
    int i = 0, size =0;
    int status = -1;
    
    size = strlen(value);
    for(i = 0; i < size; i++){
        if(!isdigit(value[i]))
            break;
    }
    if(size > 0 && i == size)
        status = 0;
    return status;
}
/*
 ********************************************************************
 * Function name: check_agent_status();
 * Description: check status of agent(running or not running).
 * Parameter:  none
 * Return value: 0:SUCCESS  -1:FAIL
 *********************************************************************
 */
int check_agent_status()
{
    int res;
    int sockfd;

    struct sockaddr_in cli_addr;

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        return -1;
    }
    printf("socket ok.\n");
    cli_addr.sin_family = AF_INET;
    cli_addr.sin_port = htons(CLI_PORT);
    cli_addr.sin_addr.s_addr = INADDR_ANY;
   
    bzero(&(cli_addr.sin_zero), 8);
  
    res = connect(sockfd, (struct sockaddr *)&cli_addr, sizeof(struct sockaddr));
    if (res == -1) {
        perror("connected");
        printf("Agent is not running.\n");
        return -1;
    } else{
        printf("connect ok.\n");
    }
    return 0;
}
/*
 ********************************************************************
 * Function name: cli_set_tr069();
 * Description: send to message queue
 * Parameter:  param
 * Return value: 0:SUCCESS  -1:FAIL
 *********************************************************************
 */
int cli_set_tr069(char *param)
{
    int res = 0;
    long type = SNDMSG;
    int mqid;

    res = creat_msgq();
    if(res == -1) {
        printf("error.\n");
        return -1;
    }
    mqid = res;
    res = send_msg(mqid, type, param);
    if (res == -1) {
        printf("error.\n");
        return -1;
    }
    printf("Set to agent successful.\n");
    return 0;
}
/*
 ********************************************************************
 * Function name: main();
 * Description: recv command from command line and send to mq
 * Parameter:  argc, argv[]
 * Return value: 0:SUCCESS  -1:FAIL
 *********************************************************************
 */
int main(int argc, char *argv[])
{
    int res = 0;
    time_t timesec;
    char buf[128];
    agent_conf a_conf;
    int f;       //flag of whether agent is running.0:runnning, 1: not running

    f = check_agent_status();
    
    char help_msg[2048] = "Usage: tr069 [tr069_command] [Parameter Value]\n\n";

    strcat(help_msg, "Configure TR-069 client parameters, including ACS URL, ACS User Name, ACS Password,Inform Interval, Connection Requeset Port, Connection Request User name, Connection Request Password, Periodic Enable, Periodic Interval, Periodic Time, Retry times\n\n");
    strcat(help_msg, "tr069 command summary\n");
    strcat(help_msg, "\tttr069 acsurl\t\tConfigure the url for the CPE to connect to the ACS\n");
    strcat(help_msg, "\ttr069 acsuser\t\tConfigure username used for HTTP-based authentication\n\t\t\t\tof the CPE\n");
    strcat(help_msg, "\ttr069 acspwd\t\tConfigure password used for HTTP-based authentication\n\t\t\t\tof the CPE\n");
    strcat(help_msg, "\ttr069 interval\t\tConfigure the duration in seconds of the interval for\n\t\t\t\tthe CPE MUST attempt to connect with the ACS\n");
    strcat(help_msg, "\ttr069 enable\t\tConfigure the status(enable or disable) of the interval\n\t\t\t\tfor the CPE MUST attempt to connect with the ACS\n");
    strcat(help_msg, "\ttr069 time\t\tConfigure the time of the interval for\n\t\t\t\tthe CPE MUST attempt to connect with the ACS\n");
    strcat(help_msg, "\ttr069 reqport\t\tConfigure the port for an ACS to make a Connection\n\t\t\t\tRequest Notification to the CPE\n");
    strcat(help_msg, "\ttr069 requser\t\tConfigure the username used to authenticate an ACS\n\t\t\t\tmaking a connection request to the CPE\n");
    strcat(help_msg, "\ttr069 reqpwd\t\tConfigure the password used to authenticate an ACS\n\t\t\t\tmaking a connectino request to the CPE\n");
    strcat(help_msg, "\ttr069 retrytimes\tConfigure the retry times used by communication\n\t\t\t\tmodule of agent for retry machenism\n");
    strcat(help_msg, "\ttr069 info\t\tDisplay TR-069 parameters information\n");
    strcat(help_msg, "\ttr069 help\t\tDisplay the help information\n\n");
    
    char acsurl_buf[64] =      "ACS URL:                      ";
    char acsuser_buf[64] =     "ACS User Name:                ";
    char acspwd_buf[64] =      "ACS Password:                 ";
    char informiv_buf[64] =     "Inform Interval:              ";
    char informenable_buf[64] =  "Inform Enable:               ";
    char informtime_buf[64] =   "Inform Time:               ";
    char connreqport_buf[64] = "Connection Request Port:      ";
    char connrequser_buf[64] = "Connection Request User Name: ";
    char connreqpwd_buf[64] =  "Connection Request Password:  ";
    char retrytimes_buf[64] = "Retry Times :        ";
 
    memset(&a_conf, 0, sizeof(a_conf));
    
    strcpy(agent_conf_path, AGENT_CONF_PATH);
    if (init_agent_conf() == FAIL) {
        printf("init agent conf failed");
        return -1;
    }
    if (get_agent_conf(&a_conf) == FAIL) {
        return -1;
    }

    if(argc == 1 || argc > 3) {
        printf("%s", help_msg);
        
        return 0;
    }
    else if(argc == 2) {
        if(strcasecmp(argv[1], "info") == 0) {
             printf("\t%s%s\n\t%s%s\n\t%s%s\n\t%s%d\n\t%s%d\n\t%s%s\n\t%s%d\n\t%s%s\n\t%s%s\n\t%s%d\n\n", 
                        acsurl_buf,a_conf.acs_url,
                        acsuser_buf, a_conf.acs_username, 
                        acspwd_buf, a_conf.acs_password, 
                        informenable_buf, a_conf.periodic_inform_enable,
                        informiv_buf, a_conf.periodic_inform_interval,
                        informtime_buf, a_conf.periodic_inform_time,
                        connreqport_buf, a_conf.conn_req_ser_port,
                        connrequser_buf, a_conf.conn_req_ser_username,
                        connreqpwd_buf, a_conf.conn_req_ser_password,
                        retrytimes_buf, a_conf.retry_times);
            return 0;
        } else {
            printf("%s", help_msg);
            return 0;
        }
    } else {
        if(strcasecmp(argv[1], "acsurl") == 0) {
            if(verifyLength(argv[2], 257) != 0){
                printf("The length of acsurl is too long.\n");
                
                return -1;
            }
            if (strcmp(a_conf.acs_url, argv[2]) == 0){
                printf("New acsurl is equal with old one.\n");
		
                return 0;
            }
            sprintf(buf, "1 %s", argv[2]);
            strcpy(a_conf.acs_url, argv[2]);
        }
        else if(strcasecmp(argv[1], "acsuser") == 0)
        {
            if(verifyLength(argv[2], 257) != 0){
                printf("The length of acsuser is too long.\n");
		
                return -1;
            }
            if(isvalidvalue(argv[2]) != 0){
                printf("The ACS User Name must be composed of number or character\n");
		
                return -1;
            }
            if (strcmp(a_conf.acs_username, argv[2]) == 0) {
                printf("New acsUser is same with old one.\n");
		
		return 0;
            }
                sprintf(buf, "2 %s", ACSUSER);                      
                strcpy(a_conf.acs_username, argv[2]);
        }
        else if(strcasecmp(argv[1], "acspwd") == 0)
        {
            if(verifyLength(argv[2], 257) != 0){
                printf("The length of acspwd is too long.\n");
		
                return -1;
            }
            if(isvalidvalue(argv[2]) != 0){
                printf("The ACS password must be composed of number or character\n");
		
                return -1;
            }
            if (strcmp(a_conf.acs_password, argv[2]) == 0) {
                printf("New acsPwd is same with old one.\n");
		
                return 0;
            }
            sprintf(buf, "2 %s", ACSPWD);                      
            strcpy(a_conf.acs_password, argv[2]); 
        }
        else if(strcasecmp(argv[1], "interval") == 0)
        {
            if(verifyNumber(argv[2]) != 0){
                printf("The informinterval must be a number.\n");
		
                return -1;
            }
            if(atoi(argv[2]) < 30  ){
                printf("The interval mustnot less than 30 seconds.\n");
		
                return -1;
            }
            if (a_conf.periodic_inform_interval == atoi(argv[2])) {
                printf("New informInterval is equal with old one.\n");
		
		return 0;
            } 
            sprintf(buf, "5 %s", argv[2]);                      
            a_conf.periodic_inform_interval = atoi(argv[2]);
        }
        else if(strcasecmp(argv[1], "enable") == 0)
        {
            if(verifyNumber(argv[2]) != 0){
                printf("The informenable must be a number.\n");
		
                return -1;
            }
            if(atoi(argv[2]) != 0 && atoi(argv[2]) != 1){
                printf("The inform enable is must be 0 or 1.\n");
		
                return -1;
            }
            if (a_conf.periodic_inform_enable == atoi(argv[2])) {
                printf("New informenable is equal with old one.\n");
		
		return 0;
            } 
            sprintf(buf, "8 %s", argv[2]);                      
            a_conf.periodic_inform_enable = atoi(argv[2]);
            
        }
        else if(strcasecmp(argv[1], "time") == 0)
        {
            if (strcmp(a_conf.periodic_inform_time, argv[2]) == 0) {
                printf("New time is equal with old one.\n");
		
		return 0;
            }
            timesec = strtimetosec(argv[2]);
            if(timesec == -1) {
                printf("The time format must be like that:1970-01-01T00:00:00\n");
                
                return -1;
            }
            sprintf(buf, "9 %ld", timesec);                      
            strcpy(a_conf.periodic_inform_time, argv[2]);
        }
        else if(strcasecmp(argv[1], "requser") == 0)
        {
            if(verifyLength(argv[2], 257) != 0){
                printf("The length of connrequser is too long.\n");
                
                return -1;
            }
            if(isvalidvalue(argv[2]) != 0){
                printf("The Connection Request User Name must be composed of number or character\n");
                
                return -1;
            }
            if (strcmp(a_conf.conn_req_ser_username, argv[2]) == 0) {
                printf("New requser is same with old one.\n");
                
		return 0;
            }
            sprintf(buf, "2 %s", REQUSER);                      
            strcpy(a_conf.conn_req_ser_username, argv[2]);
        }
        else if(strcasecmp(argv[1], "reqport") == 0)
        {
            if(verifyNumber(argv[2]) != 0){
                printf("The connreqport must be a number.\n");
                
                return -1;
            }
            if(atoi(argv[2]) < 1024 || atoi(argv[2]) > 65535){
                printf("The value of connreqport must be 1024 - 65535\n");
                
                return -1;
            }
            if (a_conf.conn_req_ser_port == atoi(argv[2])) {
                printf("New reqport is same with old one.\n");
                
		return 0;
            }
            sprintf(buf, "2 %s", REQURL);                      
            a_conf.conn_req_ser_port = atoi(argv[2]);
            printf("Need reboot agent now.\n");
        }
        else if(strcasecmp(argv[1], "reqpwd") == 0)
        {
            if(verifyLength(argv[2], 257) != 0){
                printf("The length of connreqpwd is too long.\n");
                
                return -1;
            }
            if(isvalidvalue(argv[2]) != 0){
                printf("The Connection Request Password must be composed of number or character\n");
                
                return -1;
            }
            if (strcmp(a_conf.conn_req_ser_password, argv[2]) == 0){
                printf("New reqpwd is equal with old one.\n");
                
		return 0;
            }
            sprintf(buf, "2 %s", REQPWD);                      
            strcpy(a_conf.conn_req_ser_password, argv[2]);
        }
        else if(strcasecmp(argv[1], "retrytimes") == 0)
        {
            if(verifyNumber(argv[2]) != 0){
                printf("The retrytimes must be a number.\n");
                
                return -1;
            }
            if(atoi(argv[2]) < 0 ||atoi(argv[2]) > 99 ){
                printf("The value of retry times must be 1 - 99.\n");
                
                return -1;
            }
            if (a_conf.retry_times == atoi(argv[2])) {
                printf("New retrytimes is equal with old one.\n");
                
		return 0;
            }
            sprintf(buf, "4 %s", argv[2]);                      
            a_conf.retry_times = atoi(argv[2]);
            
        }
        else
        {
            printf("%s", help_msg);
            
            return 0;
        }
        if (set_agent_conf(&a_conf) == FAIL) {
            return -1;
        }
        printf("Set agent.conf success\n");
        if(!f){
            res = cli_set_tr069(buf);
            if(res != 0) {
                printf("Set parameter failed.\n");
                return -1;
            }
        }
    }
    
    return 0; 
}



