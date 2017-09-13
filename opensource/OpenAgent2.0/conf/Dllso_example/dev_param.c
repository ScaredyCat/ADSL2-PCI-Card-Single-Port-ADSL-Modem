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


/**********************************************************************
*
*	dev_param.c - The parameters and methods of device
*
*	$Author: andyy $
*
*	history: $Date: 2007-06-08 05:46:21 $, initial version by simonl
*
***********************************************************************/

/***********************************************************************
*
*	include file
*
***********************************************************************/
#include <stdio.h>

#define DOWNLOAD_IMAGE_PATH "/root/download/image"
#define DOWNLOAD_WEB_PATH "/root/download/web"
#define DOWNLOAD_CONFIG_PATH "/root/download/config"

int dev_get_wan_para(char *wan_para)
{
    strcpy(wan_para, "InternetGatewayDevice.WANDevice.1.WANConnectionDevice.1.WANPPPConnection.1.ExternalIPAddress");
    printf("dev_get_wan_para have call\n");
    return 0;
}

int dev_first_install()
{
   printf("This is not first install.\n");
   return -1;
}

int dev_reboot_cmdkey(char *rebood_cmdkey)
{
   strcpy(rebood_cmdkey, "12345");
   printf("dev_reboot_cmdkey have call!\n");
   return 0;
}

//download device function
int get_flash_free_size()
{
    printf("Call get flash free size function!\n");
    return 50000000;
}


void kill_all_apps()
{
    printf("Call killallapps function!\n"); 
}


int parse_data(char *image_start_ptr, int bufsize, int dl_type)
{
    printf("Call parse data function!\nThe size is %d\n", bufsize);

    int result = 0;

    if(dl_type == 0)
    { 
        result = 1;
    }
    else if (dl_type == 1)
    {
        result = 2;
    }
    else if (dl_type == 2)
    {
        result = 3;
    }

    return result;
}


//tasklist device  function
int write_config_file(char *filestream, int len)
{
    printf("Call write config file function!\n");

    FILE *fp = NULL;

    fp = fopen(DOWNLOAD_CONFIG_PATH, "w+");
    if(fp == NULL)
    {
        printf("write text failed\n");
        return -1;
    }
    fwrite(filestream, 1, len, fp);
    fclose(fp);
    printf("write text successful\n");

    return 0;
}

int write_web(char *filestream, int len)
{
    printf("Call write_web function!\n");
 
    FILE *fp = NULL;
 
    fp = fopen(DOWNLOAD_WEB_PATH, "w+");
    if(fp == NULL)
    {
        printf("write web failed\n");
        return -1;
    }
    fwrite(filestream, 1, len, fp);
    fclose(fp);
    printf("write web successful\n");
 
    return 0;
} 

int flash_image(char *imageptr, int imagelen) 
{
    printf("Call flashimage function!\n");

    FILE *fp = NULL;
 
    fp = fopen(DOWNLOAD_IMAGE_PATH, "w+");
    if(fp == NULL)
    {
        printf("write image failed\n");
        return -1;
    }
    fwrite(imageptr, 1, imagelen, fp);
    fclose(fp);
    printf("flash image successful\n");

    return 0;
}

int reboot_dev()
{
    printf("Call reboot function!\n");
    return 0;
}

/*
 * Function name: TR_sys_cmd()
 * Description:
 * Parameter:
 * Return Value:
 *     success return 0 , else return -1
 */

int TR_sys_cmd(char *cmd)
{
    if (system(cmd) == -1) {
        return -1;
    }

    return 0;
}

