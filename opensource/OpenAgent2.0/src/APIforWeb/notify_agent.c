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
#include "notify_agent.h"
/**********************************************************************
*
*	function prototype
*
***********************************************************************/
static int creat_msgq();
static int send_msg(int mqid, long type, char *text);
static int read_msg(int mqid, long type);

/*
 ********************************************************************
 * Function name: Creat_msg();
 * Description: Creat message queue.
 * Parameter: None
 * Return value: 0:SUCCESS  -1:FAIL
 *********************************************************************
 */
int creat_msgq()
{
    key_t mqkey;
    int oflag,mqid;

    char filenm[] = "shared-file";
    oflag = IPC_CREAT;

    mqkey = ftok(filenm,0xFF);
    mqid = msgget(mqkey, oflag);
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
        char mtext[MAX_READ_SIZE];
    }msg;
    msg.mtype = 1;
    strcpy(msg.mtext, text);
    printf("%s\n", msg.mtext);
    res = msgsnd(mqid, &msg, strlen(msg.mtext), 0);
    if(res == -1) {
        perror("msgsnd error:");
        return -1;
    }
    return 0;
}
/*
 ********************************************************************
 * Function name: read_msg();
 * Description: Read message queue.
 * Parameter: int mqid:id of mq; long type: type of recv mq; char *text:text
 * Return value: 0:SUCCESS  -1:FAIL
 *********************************************************************
 */
int read_msg(int mqid, long type)
{
    int res = 0;
    int flag = IPC_NOWAIT;
    static int j = 1;
    struct msgbuf {
        long mtype;
        char mtext[MAX_READ_SIZE];
    }msg;    
    memset(msg.mtext, '\0', sizeof(msg.mtext));
    msg.mtype = type;
    res = msgrcv(mqid, &msg, MAX_READ_SIZE, type, flag); 
    if (res == -1) {
        perror("recv error:");
       // return -1;
    }
    else printf("In p_msgq now! Rcv text ok%d.\n", j++);
    return 0;
}
/*
 ********************************************************************
 * Function name: notify_agent();
 * Description: Notify agent when set parameter by web.
 * Parameter: char *text:text
 * Return value: 0:SUCCESS  -1:FAIL
 *********************************************************************
 */
int notify_agent(char  *text)
{
    int mqid = 0;
    long type = 0;
    int i, res = 0;
    int msg_qnum;
    struct msqid_ds myqueue_ds;
    mqid = creat_msgq();
    if(mqid == -1) {
        printf("error.\n");
        return -1;
    }
    msg_qnum = 0;

    res = msgctl(mqid, IPC_STAT, &myqueue_ds);
    if(res == -1) {
        printf("msgctl error.\n");
        return -1;
    }
    msg_qnum = myqueue_ds.msg_qnum;
    printf("msg_qnum = %d\n", msg_qnum);
    if(msg_qnum > 10) {    
        for (i = 0; i < msg_qnum; i++) {
            res = read_msg (mqid, type);
            if(res == -1) {
            	 perror("read error:");
            	// return -1;
            }
        } 
    }   
    res = send_msg(mqid, type, text);
    if (res == -1) {
        perror("send error:");
        return -1;
    }
    return 0;
}
