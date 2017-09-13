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

#include "dl_conf.h"
#include "logger.h"
#include "../handler/methods/download.h"
#include "../handler/methods/methods.h"

/*
 * Description: Read information about Download or TransferComplete from configure file
 * Param:
 *      filename:  The configure file name
 *      flag:      store flag value read from configure file
 *      param_ptr: Pointer that point to the  address of parameter structure   
 * Return Value: 0 success, -1 unsucessfully 
 * 
 */ 
int read_dl_conf(char *filename, void *param_ptr)
{    
    FILE *fp;
    int  flag;
    
    if((fp = fopen(filename, "rb+")) == NULL){
        LOG(m_tools, ERROR, "Can't open dl configure file\n");
        return (-1);
    }
    
    /* Judge whether the configure file is empty */
    fseek(fp, 0, SEEK_END);
    if(ftell(fp) == 0){
        LOG(m_tools, ERROR, "dl confgiure file is empty\n");
        fclose(fp);
        return (-1);
    }
    
    /* Move to the beginning of the configure file */
    fseek(fp, 0, SEEK_SET);
    ftell(fp);
    
    if(fread(&flag, sizeof(int), 1,fp) != 1){
        LOG(m_tools, ERROR, "Can't read flag from dl configure file\n");
        fclose(fp);
        return (-1);
    }
     
    if(flag == 0)
    { 
        if(fread((TR_download *)param_ptr, sizeof(TR_download), 1, fp) != 1){
            LOG(m_tools, ERROR, "Can't read Download parameters from dl configure file\n");
            fclose(fp);
            return (-1);
        }
    }
    
    if(flag == 1)
    {
        if(fread((TR_tran_comp *)param_ptr, sizeof(TR_tran_comp), 1, fp) != 1){
            LOG(m_tools, ERROR, "Can't read TransferComplete parameters from dl configure file\n");
            fclose(fp);
            return (-1);
        }
    }     
    
    /* Clear the file content */ 
    if(ftruncate(fileno(fp), 0) != 0){
       LOG(m_tools, ERROR, "Change dl configure file size to 0 failure\n");
       fclose(fp);
       return (-1);
    }
    
    LOG(m_tools, DEBUG, "Change dl configure file size to 0 sucessfully\n");
    
    fclose(fp); 
    return (0);
}

/*
 * Description: Read information about Download or TransferComplete from configure file
 * Param:
 *       filename:  The configure file name
 *       flag:      "0" Download information       "1" TransferComplete information
 *       param_ptr: Pointer that point to the  address of parameter structure 
 * Return Value: 0 success  -1 failure
 */
int write_dl_conf(char *filename, int flag, void *param_ptr)
{
    FILE   *fp = NULL;  
    
    if(flag == 0)
    {
        if((fp = fopen(filename, "wb+")) == NULL){
            LOG(m_tools, ERROR, "Can't open or create dl configure file\n");
            return (-1);
        }
        
        if(fwrite(&flag, sizeof(int), 1, fp) != 1){
            LOG(m_tools, ERROR, "Can't write flag 0 into dl configure file\n");
            fclose(fp);
            return(-1);
        }
        
        LOG(m_tools, DEBUG, "Write flag 0 into dl configure file\n");
        
        if(fwrite(param_ptr, sizeof(TR_download), 1, fp) != 1){
            LOG(m_tools, ERROR, "Can't write Download parameters into dl configure file\n");
            fclose(fp);
            return (-1);
        }
        
        LOG(m_tools, DEBUG, "Write Download parameters into dl configure file\n");
    }
    if(flag == 1)
    {
        if((fp = fopen(filename, "wb+")) ==NULL){
             LOG(m_tools, ERROR, "Can't open dl configure file\n");
             return (-1);
        }
        
        if(fwrite(&flag, sizeof(int), 1, fp) != 1){
            LOG(m_tools, ERROR, "Can't write flag 1 into dl configure file\n");
            fclose(fp);
            return (-1);
        }
        
        LOG(m_tools, DEBUG, "Write flag 1 into dl configure file\n");
        
        if(fwrite(param_ptr, sizeof(TR_tran_comp), 1, fp) != 1){
            LOG(m_tools, ERROR, "Can't write TransferComplete parameters into dl configure file\n");
            fclose(fp);
            return (-1);
        }
        
        LOG(m_tools, DEBUG, "Write TransferComplete parameters into dl configure file\n");
    }
   
    if((flag != 0) && (flag != 1)){
    	LOG(m_tools, ERROR, "The flag write to dl configure file is not 0 or 1\n");
    	return(-1);
    }
    
    fclose(fp);
    return (0);
}

/*
 * Description: Read flag from configure file
 * Param:
 *       filename:  The configure file name               
 * Return Value: 0 means flag = 0, 1 means flag = 1
 *               -1 unsucessfully  -2 the configure file is empty
 */
int flag_dl_conf(char *filename)
{
    FILE *fp;
    int  flag;
    
    if((fp = fopen(filename, "rb")) == NULL){
        LOG(m_tools, ERROR, "Can't open dl configure file\n");
        return (-1);
    }
    
    /* Judge whether the configure file is empty */
    fseek(fp, 0, SEEK_END);
    if(ftell(fp) == 0){
        LOG(m_tools, DEBUG, "dl confgiure file is empty\n");
        fclose(fp);
        return (-2);
    }
    
    /* Move to the beginning of the configure file */
    fseek(fp, 0, SEEK_SET);
    ftell(fp);

    if(fread(&flag, sizeof(int), 1,fp) != 1){
        LOG(m_tools, ERROR, "Can't read flag from dl configure file\n");
        fclose(fp);
        return (-1);
    }
    
    fclose(fp);
        
    LOG(m_tools, DEBUG, "The flag read from dl configure file is %d\n", flag);
    return flag;
}
