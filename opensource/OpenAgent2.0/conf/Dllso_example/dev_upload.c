/*
 * Iopyright(c) 2006-2007, Works Systems, Inc.
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
 * Function: dev_getconfigfile() This function will get the configure file of ADSL
 * Param: char *buf the buffer which will be save the configure file content
 * Return Value:
 *     Return 0 is successful, -1 is failed
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

int dev_getconfigfile(char *buf, int length)
{
    int readCnt = 0;
    FILE *fp;
    int fd;
    struct stat theStat;
    fp = fopen("/OpenAgent2.0/conf/Dllso_example/upload/upconf.txt", "r");
    if (fp == NULL) {
        printf("file open fail!\n");
        return -1;
    }
    fd = fileno(fp);
    fstat(fd, &theStat);
    if (theStat.st_size > length) {
        printf("file is too big!\n");
        fclose(fp);
        return -1;
    }

    readCnt = fread(buf, 1, theStat.st_size, fp);
    if (readCnt < theStat.st_size ){
        printf("lose some.%d %d\n", readCnt, theStat.st_size);
        fclose(fp);
        return -1;
    }
    fclose(fp);
    return 0;
}               

/*
 * Function: dev_getsyslog() This Function will get the system log of ADSL
 * Param: char *buf  will be used to save the log data
 * Return value:
 *      return 0 is successful
 */

int dev_getsyslog(char *buf, int length)
{ 
    int fd;
    int readCnt = 0;
    FILE *fp;
    struct stat theStat;
   
    fp = fopen("/OpenAgent2.0/conf/Dllso_example/upload/upsyslog.txt", "r");
    if (fp == NULL) {
        printf("open file fail!\n");
        return -1;
    }
    fd = fileno(fp);
    fstat(fd, &theStat);
    if (theStat.st_size > length) {
        printf("file is too big!\n");
        fclose(fp);
        return -1;
    }

    readCnt = fread(buf, 1, theStat.st_size, fp);
    if (readCnt < theStat.st_size) {
        printf("lose some:%d %d", readCnt, theStat.st_size);
        fclose(fp);
        return -1;
    }
    fclose(fp);
    return 0;
}


