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
 * $Author $
 * $Date: 2007-06-08 02:14:47 $
 */
                                                                                                                             
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
typedef struct
{
    char param_path[7];
    int used_num;
}TR_multi_obj;

TR_multi_obj multi_obj_buf[50];
//HANK2007/12/5 05:44¤U¤È
//char obj_conf_path[] = "/root/multi.conf";
char obj_conf_path[] = "/etc/conf/multi.conf";


//WANDevice
int dev_WAN_WANnum(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
//InternetGatewayDevice.WANDevice.WAN-CommonInterfaceConfig
int dev_WANCommon_If_EnabledForInternet(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANCommon_If_WANAccessType(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANCommon_If_Layer1UpstreamMaxBitRate(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANCommon_If_Layer1DownstreamMaxBitRate(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANCommon_If_PhysicalLinkStatus(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANCommon_If_WANAccessProvider(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANCommon_If_TotalBytesSent(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANCommon_If_TotalBytesReceived(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANCommon_If_TotalPacketsSent(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANCommon_If_TotalPacketsReceived(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANCommon_If_MaximumActiveConnections(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANCommon_If_NumberOfActiveConnections(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
//InternetGatewayDevice.WANDevice.{i}.WAN-CommonInterfaceConfig.Connection
int dev_WANCommon_If_DeviceContainer(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
        return 0;
}
int dev_WANCommon_If_ServiceID(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}

//InternetGatewayDevice.WANDevice.WANDSLInterfaceConfig
int dev_WANDSL_If_Enable(int opt_flag, void *mthdval_struct, int locate[])
{
    int wandsl_if_enable;
    
    if (!opt_flag) // get function
    {
        //Call native function to get wandsl_if_enable -- TO DO
        wandsl_if_enable = 0;
        *((int *)mthdval_struct) = wandsl_if_enable;
        printf("wandsl_if_enable is %d.\n", wandsl_if_enable);
        return 0;
    }
    else           // set function
    {  
        //Call native function to set wandsl_if_enable -- TO DO
        int i = 1;
        switch (i) {
            case 0 :
                printf("Set wandsl_if_enable parameter value Successfully.\n");
                return 0;
            case 1 :
                printf("Set wandsl_if_enable parameter value Successfully and need reboot\n");
                return 1;
            case -1 :
                printf("Set wandsl_if_enable parameter value invalid\n");
                return -1;
            case -2 :
                printf("Set wandsl_if_enable parameter value fail\n");
                return -2;
        }

    }
}
int dev_WANDSL_If_Status(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        //Call native function to get WANDSL_If_Status -- TO DO
        strcpy((char *)mthdval_struct, "Up");
        printf("WANDSL_If_Status is %s.\n", (char *)mthdval_struct);
        return 0;
    }
    else           // set function
    {
        printf("WANDSL_If_Status is unwritable.\n");
        return -1;
    }
}
int dev_WANDSL_If_ModulationType(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        //Call native function to get WANDSL_If_ModulationType -- TO DO
        strcpy((char *)mthdval_struct, "ADSL_G.dmt");
        printf("WANDSL_If_ModulationType is %s.\n", (char *)mthdval_struct);
        return 0;
    }
    else           // set function
    {
        printf("WANDSL_If_ModulationType is unwritable.\n");
        return -1;
    }
}
int dev_WANDSL_If_LineEncoding(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_If_DataPath(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        //Call native function to get WANDSL_If_DataPath -- TO DO
        strcpy((char *)mthdval_struct, "Interleaved");
        printf("WANDSL_If_DataPath is %s.\n", (char *)mthdval_struct);
        return 0;
    }
    else           // set function
    {
        printf("WANDSL_If_DataPath is unwritable.\n");
        return -1;
    }
}
int dev_WANDSL_If_InterleaveDepth(int opt_flag, void *mthdval_struct, int locate[])
{
    unsigned int wandsl_if_interleavedepth;
    
    if (!opt_flag) // get function
    {
        //Call native function to get WANDSL_If_InterleaveDepth -- TO DO
        wandsl_if_interleavedepth = 2;
        *((unsigned int *)mthdval_struct) = wandsl_if_interleavedepth;
        printf("wandsl_if_interleavedepth is %d.\n", wandsl_if_interleavedepth);
        return 0;
    }
    else           // set function
    {
        printf("wandsl_if_interleavedepth is unwritable.\n");
        return -1;
    }
}
int dev_WANDSL_If_LineNumber(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_If_UpstreamCurrRate(int opt_flag, void *mthdval_struct, int locate[])
{
    unsigned int wandsl_if_upstreamcurrate;
    
    if (!opt_flag) // get function
    {
        //Call native function to get wandsl_if_upstreamcurrate -- TO DO
        wandsl_if_upstreamcurrate = 100;
        *((unsigned int *)mthdval_struct) = wandsl_if_upstreamcurrate;
        printf("wandsl_if_upstreamcurrate is %d.\n", wandsl_if_upstreamcurrate);
        return 0;
    }
    else           // set function
    {
        printf("wandsl_if_upstreamcurrate is unwritable.\n");
        return -1;
    }
}
int dev_WANDSL_If_DownstreamCurrRate(int opt_flag, void *mthdval_struct, int locate[])
{
    unsigned int wandsl_if_downstreamcurrate;
    
    if (!opt_flag) // get function
    {
        //Call native function to get wandsl_if_downstreamcurrate -- TO DO
        wandsl_if_downstreamcurrate = 10;
        *((unsigned int *)mthdval_struct) = wandsl_if_downstreamcurrate;
        printf("wandsl_if_downstreamcurrate is %d.\n", wandsl_if_downstreamcurrate);
        return 0;
    }
    else           // set function
    {
        printf("wandsl_if_downstreamcurrate is unwritable.\n");
        return -1;
    }
}
int dev_WANDSL_If_UpstreamMaxRate(int opt_flag, void *mthdval_struct, int locate[])
{
    unsigned int wandsl_if_upstreammaxrate;
    
    if (!opt_flag) // get function
    {
        //Call native function to get wandsl_if_upstreammaxrate -- TO DO
        wandsl_if_upstreammaxrate = 1000;
        *((unsigned int *)mthdval_struct) = wandsl_if_upstreammaxrate;
        printf("wandsl_if_upstreammaxrate is %d.\n", wandsl_if_upstreammaxrate);
        return 0;
    }
    else           // set function
    {
        printf("wandsl_if_upstreammaxrate is unwritable.\n");
        return -1;
    }
}
int dev_WANDSL_If_DownstreamMaxRate(int opt_flag, void *mthdval_struct, int locate[])
{
    unsigned int wandsl_if_downstreammaxrate;
    
    if (!opt_flag) // get function
    {
        //Call native function to get wandsl_if_downstreammaxrate -- TO DO
        wandsl_if_downstreammaxrate = 20;
        *((unsigned int *)mthdval_struct) = wandsl_if_downstreammaxrate;
        printf("wandsl_if_downstreammaxrate is %d.\n", wandsl_if_downstreammaxrate);
        return 0;
    }
    else           // set function
    {
        printf("wandsl_if_downstreammaxrate is unwritable.\n");
        return -1;
    }
}
int dev_WANDSL_If_UpstreamNoiseMargin(int opt_flag, void *mthdval_struct, int locate[])
{
    int wandsl_if_upstreamnoisemargin;
    
    if (!opt_flag) // get function
    {
        //Call native function to get wandsl_if_upstreamnoisemargin -- TO DO
        wandsl_if_upstreamnoisemargin = 2000;
        *((int *)mthdval_struct) = wandsl_if_upstreamnoisemargin;
        printf("wandsl_if_upstreamnoisemargin is %d.\n", wandsl_if_upstreamnoisemargin);
        return 0;
    }
    else           // set function
    {
        printf("wandsl_if_upstreamnoisemargin is unwritable.\n");
        return -1;
    }
}
int dev_WANDSL_If_DownstreamNoiseMargin(int opt_flag, void *mthdval_struct, int locate[])
{
    int wandsl_if_downstreamnoisemargin;
    
    if (!opt_flag) // get function
    {
        //Call native function to get wandsl_if_upstreamnoisemargin -- TO DO
        wandsl_if_downstreamnoisemargin = 200;
        *((int *)mthdval_struct) = wandsl_if_downstreamnoisemargin;
        printf("wandsl_if_downstreamnoisemargin is %d.\n", wandsl_if_downstreamnoisemargin);
        return 0;
    }
    else           // set function
    {
        printf("wandsl_if_downstreamnoisemargin is unwritable.\n");
        return -1;
    }
}
int dev_WANDSL_If_UpstreamAttenuation(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_If_DownstreamAttenuation(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_If_UpstreamPower(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_If_DownstreamPower(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_If_ATURVendor(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_If_ATURCountry(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_If_ATURANSIStd(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_If_ATURANSIRev(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_If_ATUCVendor(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_If_ATUCCountry(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_If_ATUCANSIStd(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_If_ATUCANSIRev(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_If_TotalStart(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_If_ShowtimeStart(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_If_LastShowtimeStart(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_If_CurrentDayStart(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_If_QuarterHourStart(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}

//InternetGatewayDevice.WANDevice.WANDSLInterfaceConfig.stats.Total
int dev_WANDSL_Stats_Total_ReceiveBlocks(int opt_flag, void *mthdval_struct, int locate[])
{
    unsigned int wandsl_stats_total_receiveblocks;
    
    if (!opt_flag) // get function
    {
        //Call native function to get eth_config_maccontrolenabled -- TO DO
        wandsl_stats_total_receiveblocks = 10;
        *((unsigned int *)mthdval_struct) = wandsl_stats_total_receiveblocks;
        printf("wandsl_stats_total_receiveblocks is %d.\n", wandsl_stats_total_receiveblocks);
        return 0;
    }
    else           // set function
    {
        printf("wandsl_stats_total_receiveblocks is unwritable.\n");
        return -1;
    }
}
int dev_WANDSL_Stats_Total_TransmitBlocks(int opt_flag, void *mthdval_struct, int locate[])
{
    unsigned int wandsl_stats_total_transmitblocks;
    
    if (!opt_flag) // get function
    {
        //Call native function to get eth_config_maccontrolenabled -- TO DO
        wandsl_stats_total_transmitblocks = 10;
        *((unsigned int *)mthdval_struct) = wandsl_stats_total_transmitblocks;
        printf("wandsl_stats_total_transmitblocks is %d.\n", wandsl_stats_total_transmitblocks);
        return 0;
    }
    else           // set function
    {
        printf("wandsl_stats_total_transmitblocks is unwritable.\n");
        return -1;
    }
}
int dev_WANDSL_Stats_Total_CellDelin(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_Total_LinkRetrain(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_Total_InitErrors(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_Total_InitTimeouts(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_Total_LossOfFraming(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_Total_ErroredSecs(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_Total_SeverelyErroredSecs(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_Total_FECErrors(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_Total_ATUCFECErrors(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_Total_HECErrors(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_Total_ATUCHECErrors(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_Total_CRCErrors(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_Total_ATUCCRCErrors(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}

//InternetGatewayDevice.WANDevice.WANDSLInterfaceConfig.stats.LastShowtime
int dev_WANDSL_Stats_LastShowtime_ReceiveBlocks(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_LastShowtime_TransmitBlocks(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_LastShowtime_CellDelin(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_LastShowtime_LinkRetrain(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_LastShowtime_InitErrors(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_LastShowtime_InitTimeouts(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_LastShowtime_LossOfFraming(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_LastShowtime_ErroredSecs(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_LastShowtime_SeverelyErroredSecs(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_LastShowtime_FECErrors(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_LastShowtime_ATUCFECErrors(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_LastShowtime_HECErrors(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_LastShowtime_ATUCHECErrors(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_LastShowtime_CRCErrors(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_LastShowtime_ATUCCRCErrors(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
//InternetGatewayDevice.WANDevice.WANDSLInterfaceConfig.stats.Showtime
int dev_WANDSL_Stats_Showtime_ReceiveBlocks(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_Showtime_TransmitBlocks(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_Showtime_CellDelin(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_Showtime_LinkRetrain(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_Showtime_InitErrors(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_Showtime_InitTimeouts(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_Showtime_LossOfFraming(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_Showtime_ErroredSecs(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_Showtime_SeverelyErroredSecs(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_Showtime_FECErrors(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_Showtime_ATUCFECErrors(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_Showtime_HECErrors(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_Showtime_ATUCHECErrors(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_Showtime_CRCErrors(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_Showtime_ATUCCRCErrors(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
//InternetGatewayDevice.WANDevice.WANDSLInterfaceConfig.stats.CurrentDay.
int dev_WANDSL_Stats_CurrentDay_ReceiveBlocks(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_CurrentDay_TransmitBlocks(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_CurrentDay_CellDelin(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_CurrentDay_LinkRetrain(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_CurrentDay_InitErrors(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_CurrentDay_InitTimeouts(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_CurrentDay_LossOfFraming(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_CurrentDay_ErroredSecs(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_CurrentDay_SeverelyErroredSecs(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_CurrentDay_FECErrors(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_CurrentDay_ATUCFECErrors(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_CurrentDay_HECErrors(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_CurrentDay_ATUCHECErrors(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_CurrentDay_CRCErrors(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_CurrentDay_ATUCCRCErrors(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
//InternetGatewayDevice.WANDevice.WANDSLInterfaceConfig.stats.QuarterHour
int dev_WANDSL_Stats_QuarterHour_ReceiveBlocks(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_QuarterHour_TransmitBlocks(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_QuarterHour_CellDelin(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_QuarterHour_LinkRetrain(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_QuarterHour_InitErrors(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_QuarterHour_InitTimeouts(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_QuarterHour_LossOfFraming(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_QuarterHour_ErroredSecs(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_QuarterHour_SeverelyErroredSecs(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_QuarterHour_FECErrors(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_QuarterHour_ATUCFECErrors(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_QuarterHour_HECErrors(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_QuarterHour_ATUCHECErrors(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_QuarterHour_CRCErrors(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Stats_QuarterHour_ATUCCRCErrors(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}

//InternetGatewayDevice.WANDevice.WANEthernetInterfaceConfig.
int dev_WANEth_If_Enable(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANEth_If_Status(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANEth_If_MACAddress(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANEth_If_MaxBitRate(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANEth_If_DuplexMode(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}


//InternetGatewayDevice.WANDevice.WANEthernetInterfaceConfig.Stats
int dev_WAN_Ethstats_BytesSent(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WAN_Ethstats_BytesReceived(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WAN_Ethstats_PacketsSent(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WAN_Ethstats_PacketsReceived(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}

//InternetGatewayDevice.WANDevice.WANDSLConnectionManagement
int dev_WANDSL_Manage_ServiceNumber(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}

//InternetGatewayDevice.WANDevice.WANDSLConnectionManagement.Connection-Service
int dev_WANDSL_Manage_ConnectionServ_WANConnectionDevice(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Manage_ConnectionServ_WANConnectionService(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Manage_ConnectionServ_DestinationAddress(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Manage_ConnectionServ_LinkType(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Manage_ConnectionServ_ConnectionType(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Manage_ConnectionServ_Name(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}

//InternetGatewayDevice.WANDevice.WAN-DSLDiagnostics.
int dev_WANDSL_Diag_LoopDiagnosticsState(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Diag_ACTPSDds(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Diag_ACTPSDus(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Diag_ACTATPds(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Diag_ACTATPus(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Diag_HLINSCds(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Diag_HLINpsds(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Diag_QLNpsds(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Diag_BITSpsds(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Diag_SNRpsds(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANDSL_Diag_GAINSpsds(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}

//InternetGatewayDevice.WANDevice.WANConnectionDevice

int dev_WANConnectionDev_IPConnectionNumber(int opt_flag, void *mthdval_struct, int locate[])
{
    unsigned int wan_ip_conn_num;

    if (!opt_flag) // get function
    {
        wan_ip_conn_num = 1;
        *((unsigned int *)mthdval_struct) = wan_ip_conn_num;
        printf("Get WANCon IPCon_Number is %d.\n",  *((unsigned int *)mthdval_struct));
        return 0;
    }
    else           // set function
    {
        printf("WANIPConnectionDeviceNumberOfEntries is unwritable\n");
        return -1;
    }

}
int dev_WANConnectionDev_PPPConnectionNumber(int opt_flag, void *mthdval_struct, int locate[])
{
    unsigned int wan_ppp_conn_num;
                                                                                
    if (!opt_flag) // get function
    {
        wan_ppp_conn_num = 1;
        *((unsigned int *)mthdval_struct) = wan_ppp_conn_num;
        printf("Get WANCon PPPCon_Number is %d.\n", *((unsigned int *)mthdval_struct)); 
        return 0;
    }
    else           // set function
    {
        printf("WANPPPConnectionDeviceNumberOfEntries is unwritable\n");
        return -1;
    }
}

//InternetGatewayDevice.WANDevice.WANConnectionDevice.WANDSLLinkConfig.

int dev_WANConnectionDev_LinkConfig_Enable(int opt_flag, void *mthdval_struct, int locate[])
{
    int wan_dslconf_enable;
    if (!opt_flag) // get function
    {
        wan_dslconf_enable = 1;
        *((int *)mthdval_struct) = wan_dslconf_enable; 
        printf("Get WANCon LinkCon_Enable is %d.\n", *((int *)mthdval_struct));
        return 0;
    }
    else           // set function
    {
        printf("WANDSLLinkConfig enable is writable\n");
        return 0;
    }
}
int dev_WANConnectionDev_LinkConfig_LinkStatus(int opt_flag, void *mthdval_struct, int locate[])
{
    char wan_dsllink_stats[12];
    if (!opt_flag) // get function
    {
        strcpy(wan_dsllink_stats, "Up");
        strcpy((char *)mthdval_struct, wan_dsllink_stats);
        printf("Get WANCon LinkCon_LinkStatus is %s.\n", (char *)mthdval_struct);
        return 0;
    }
    else           // set function
    {
	printf("WANDSLLinkConfig LinkStatus is unwritable\n");
        return -1;
    }
}
int dev_WANConnectionDev_LinkConfig_LinkType(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        strcpy((char*)mthdval_struct, "Unconfigured");
        printf("Get WANCon LinkCOn_LinkType %s.\n", (char*)mthdval_struct);
        return 0;
    }
    else           // set function
    {
	printf("Set WANDSLLinkConfig LinkType is writable\n");
        return 0;
    }
}
int dev_WANConnectionDev_LinkConfig_AutoConfig(int opt_flag, void *mthdval_struct, int locate[])
{
    int wan_dsllink_autoconf;
    if (!opt_flag) // get function
    {
        wan_dsllink_autoconf = 0;
        *((int *)mthdval_struct) = wan_dsllink_autoconf;
        printf("Get WANCon LinkConfig_AutoConfig is %d.\n", *((int*)mthdval_struct));
        return 0;
    }
    else           // set function
    {
	printf("WANDSLLinkConfig AutoConfig is unwritable\n");
        return -1;
    }
}
int dev_WANConnectionDev_LinkConfig_ModulationType(int opt_flag, void *mthdval_struct, int locate[])
{
    char wan_dsllink_modutype[17];
    if (!opt_flag) // get function
    {
        strcpy(wan_dsllink_modutype, "SDSL");
        strcpy((char *)mthdval_struct, wan_dsllink_modutype);
        printf("Get WANCon LinkConfig_ModuType is %s.\n ", (char*)mthdval_struct); 
        return 0;
    }
    else           // set function
    {
	printf("WANDSLLinkConfig ModulationType is unwritable\n");
        return -1;
    }
}
int dev_WANConnectionDev_LinkConfig_DestinationAddress(int opt_flag, void *mthdval_struct, int locate[])
{
    char wan_dsllink_destaddr[256];
    if (!opt_flag) // get function
    {
	strcpy(wan_dsllink_destaddr, "www.example.com");
        strcpy((char *)mthdval_struct, wan_dsllink_destaddr);
        printf("Get WANCon LinkConfig_DestinAddress is %s.\n", (char*)mthdval_struct);    
        return 0;
    }
    else           // set function
    {
	printf("WANDSLLinkConfig DestinationAddress is wirtable\n");
        return 0;
    }
}
int dev_WANConnectionDev_LinkConfig_ATMEncapsulation(int opt_flag, void *mthdval_struct, int locate[])
{
    char wan_dsllink_atmencap[10];
    if (!opt_flag) // get function
    {
        strcpy(wan_dsllink_atmencap, "VCMUX");
        strcpy((char *)mthdval_struct, wan_dsllink_atmencap);\
        printf("Get WANCon LinkConfig_ATMEncapsulation is %s.\n", (char*)mthdval_struct);
        return 0;
    }
    else           // set function
    {
        printf("WANDSLLinkConfig ATMEncapsulation is writable\n");
        return 0;
    }
}
int dev_WANConnectionDev_LinkConfig_FCSPreserved(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANConnectionDev_LinkConfig_VCSearchList(int opt_flag, void *mthdval_struct, int locate[])
{
    char wan_dsllink_vcSearch[20];
    if (!opt_flag) // get function
    {
        strcpy(wan_dsllink_vcSearch, "VPl1/VCl1, VPl2/VCl2");
        strcpy((char *)mthdval_struct, wan_dsllink_vcSearch);
        printf("Get WANCon LinkConfig_VCSearchList is %s.\n", (char*)mthdval_struct);
        return 0;
    }
    else           // set function
    {
        printf("WANDSLLinkConfig VCSearchList is writable\n");
        return 0;
    }
}
int dev_WANConnectionDev_LinkConfig_ATMAAL(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}

int dev_WANConnectionDev_LinkConfig_ATMTransmittedBlocks(int opt_flag, void *mthdval_struct, int locate[])
{
    unsigned int wan_dsl_atmtransblock;
    if (!opt_flag) // get function
    {
        wan_dsl_atmtransblock = 10;
        *((unsigned int *)mthdval_struct) = wan_dsl_atmtransblock;
        printf("Get WANCon LinkConfig_ATMTrans_Blocks is %d.\n", *((unsigned int *)mthdval_struct));
        return 0;
    }
    else           // set function
    {
        printf("DSLLinkConfig ATMTransmittedBlocks is unwritable\n");
        return -1;
    }
}
int dev_WANConnectionDev_LinkConfig_ATMReceivedBlocks(int opt_flag, void *mthdval_struct, int locate[])
{
    unsigned int wan_dsl_atmrecvblock;
    if (!opt_flag) // get function
    {
        wan_dsl_atmrecvblock = 10;
        *((unsigned int *)mthdval_struct) = wan_dsl_atmrecvblock;
        printf("Get WANCon LinkConfig_ATMRece_Blocks is %d.\n", *((unsigned int *)mthdval_struct));
        return 0;
    }
    else           // set function
    {
        printf("DSLLinkConfig ATMReceiveBlocks is unwritable\n");
        return -1;
    }

}
int dev_WANConnectionDev_LinkConfig_ATMQoS(int opt_flag, void *mthdval_struct, int locate[])
{
    char wan_dsllink_atmqos[8];
    if (!opt_flag) // get function
    {
        strcpy(wan_dsllink_atmqos, "CBR");
        strcpy((char *)mthdval_struct, wan_dsllink_atmqos);
        printf("Get WANCon LinkConfig_ATMQos is %s.\n", (char *)mthdval_struct);
        return 0;
    }
    else           // set function
    {
        printf("DSLLinkConfig ATMQoS is writable");
        return 0;
    }
}
int dev_WANConnectionDev_LinkConfig_ATMPeakCellRate(int opt_flag, void *mthdval_struct, int locate[])
{
    unsigned int wan_dsllink_atmpeakrate;
    if (!opt_flag) // get function
    {
        wan_dsllink_atmpeakrate = 1200;
        *((unsigned int *)mthdval_struct) = wan_dsllink_atmpeakrate;
        printf("Get WANCon LinkConfig_ATMPeakCellRate is %d.\n", *((unsigned int *)mthdval_struct));
        return 0;
    }
    else           // set function
    {
	printf("DSLLinkConfig ATMPeakCellRate is writable\n");
        return 0;
    }
}
int dev_WANConnectionDev_LinkConfig_ATMMaximumBurstSize(int opt_flag, void *mthdval_struct, int locate[])
{
    unsigned int wan_dsllink_atmmaxsize;
    if (!opt_flag) // get function
    {
        wan_dsllink_atmmaxsize = 5;
        *((unsigned int *)mthdval_struct) = wan_dsllink_atmmaxsize;
        printf("Get WANCon LinkConfig_ATMMaxBurstSize is %d.\n", *((unsigned int *)mthdval_struct));
        return 0;
    }
    else           // set function
    {
	printf("DSLLinkConfig ATMMaximumBurstSize is writable\n");
        return 0;
    }
}
int dev_WANConnectionDev_LinkConfig_ATMSustainableCellRate(int opt_flag, void *mthdval_struct, int locate[])
{
    unsigned int wan_dsllink_atmsustnrate;
    if (!opt_flag) // get function
    {
        wan_dsllink_atmsustnrate = 1400;
        *((unsigned int *)mthdval_struct) = wan_dsllink_atmsustnrate;
        printf("Get WANCon LinkConfig_ATMSustainableCellRate is %d.\n", *((unsigned int *)mthdval_struct));
        return 0;
    }
    else           // set function
    {
        printf("DSLLinkConfig ATMSustainableCellRate is writable\n");
        return 0;
    }
}
int dev_WANConnectionDev_LinkConfig_AAL5CRCErrors(int opt_flag, void *mthdval_struct, int locate[])
{
    unsigned int wan_dsllink_aalcrcerr;
    if (!opt_flag) // get function
    {
        wan_dsllink_aalcrcerr = 0;
        *((unsigned int *)mthdval_struct) = wan_dsllink_aalcrcerr;
        printf("Get WANCon LinkConfig_AAL5CRCErrors is %d.\n", *((unsigned int *)mthdval_struct));
        return 0;
    }
    else           // set function
    {
        printf("DSLLinkConfig AAL5CRCErrors is unwritable\n");
        return -1;
    }
}
int dev_WANConnectionDev_LinkConfig_ATMCRCErrors(int opt_flag, void *mthdval_struct, int locate[])
{
    unsigned int wan_dsllink_atmcrcerr;
    if (!opt_flag) // get function
    {
        wan_dsllink_atmcrcerr = 0;
        *((unsigned int *)mthdval_struct) = wan_dsllink_atmcrcerr;
        printf("Get WANCon LinkConfig_ATMCRCErrors is %d.\n", *((unsigned int *)mthdval_struct));
        return 0;
    }
    else           // set function
    {
        printf("DSLLinkConfig ATMCRCErrors is unwritable\n");
        return -1;
    }
}
int dev_WANConnectionDev_LinkConfig_ATMHECErrors(int opt_flag, void *mthdval_struct, int locate[])
{
    unsigned int wan_dsllink_atmhecerr;
    if (!opt_flag) // get function
    {
        wan_dsllink_atmhecerr = 0;
        *((unsigned int *)mthdval_struct) = wan_dsllink_atmhecerr;
        printf("Get WANCon LinkConfig_ATMHECErrors is %d.\n", *((unsigned int *)mthdval_struct));
        return 0;
    }
    else           // set function
    {
        printf("DSLLinkConfig ATMHECErrors is unwritable\n");
        return -1;
    }

}

//InternetGatewayDevice.WANDevice.WANConnectionDevice.WANATMF5Loopback-Diagnostics

int dev_WANConnectionDev_ATMF5_DiagnosticsState(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANConnectionDev_ATMF5_NumberOfRepetitions(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANConnectionDev_ATMF5_Timeout(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANConnectionDev_ATMF5_SuccessCount(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANConnectionDev_ATMF5_FailureCount(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANConnectionDev_ATMF5_AverageResponseTime(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANConnectionDev_ATMF5_MinimumResponseTime(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANConnectionDev_ATMF5_MaximumResponseTime(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}

//InternetGatewayDevice.WANDevice.{i}.WAN-ConnectionDevice.{i}.WANEthernetLink-Config.

int dev_WANEthConfig_EthernetLinkStatus(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}

//InternetGatewayDevice.WANDevice.{i}.WAN-ConnectionDevice.{i}.WANPOTSLinkConfig.

int dev_WANPOSTConfig_Enable(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANPOSTConfig_LinkStatus(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANPOSTConfig_ISPPhoneNumber(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANPOSTConfig_ISPInfo(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANPOSTConfig_LinkType(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANPOSTConfig_NumberOfRetries(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANPOSTConfig_DelayBetweenRetries(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANPOSTConfig_Fclass(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANPOSTConfig_DataModulationSupported(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANPOSTConfig_DataProtocol(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANPOSTConfig_DataCompression(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANPOSTConfig_PlusVTRCommandSupported(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}

//InternetGatewayDevice.WANDevice.{i}.WAN-ConnectionDevice.{i}.WANIPConnection.{i}.

int dev_WANIPConnection_Enable(int opt_flag, void *mthdval_struct, int locate[])
{
    int wan_ipconn_enbale;
    if (!opt_flag) // get function
    {
        wan_ipconn_enbale = 1;
        *((int *)mthdval_struct) = wan_ipconn_enbale;
        return 0;
    }
    else           // set function
    {
        printf("WANIPConnection enbale is writable\n");
        return 0;
    }
}
int dev_WANIPConnection_ConnectionStatus(int opt_flag, void *mthdval_struct, int locate[])
{
    char wan_ipconn_connstats[17];
    if (!opt_flag) // get function
    {
        strcpy(wan_ipconn_connstats, "Connected");
        strcpy((char *)mthdval_struct, wan_ipconn_connstats);
        return 0;
    }
    else           // set function
    {
        printf("WANIPConnection ConnectionStatus is unwritable\n");
        return -1;
    }
}
int dev_WANIPConnection_PossibleConnectionTypes(int opt_flag, void *mthdval_struct, int locate[])
{
    char wan_ipconn_posbl_conntype[13];
    if (!opt_flag) // get function
    {
        strcpy(wan_ipconn_posbl_conntype, "IP_Routed");
        strcpy((char *)mthdval_struct, wan_ipconn_posbl_conntype);
        return 0;
    }
    else           // set function
    {
        printf("WANIPConnection PossibleConnectionTypes is unwritable");
        return -1;
    }
}
int dev_WANIPConnection_ConnectionType(int opt_flag, void *mthdval_struct, int locate[])
{
    char wan_ipconn_conntype[13];
    if (!opt_flag) // get function
    {
        strcpy(wan_ipconn_conntype, "IP_Routed");
        strcpy((char *)mthdval_struct, wan_ipconn_conntype);
        return 0;
    }
    else           // set function
    {
        printf("WANIPConnection ConnectionTypes is writable");
        return 0;
    }

}
int dev_WANIPConnection_Name(int opt_flag, void *mthdval_struct, int locate[])
{
    char wan_ipconn_name[256];
    if (!opt_flag) // get function
    {
        strcpy(wan_ipconn_name, "ewt");
        strcpy((char *)mthdval_struct, wan_ipconn_name);
        return 0;
    }
    else           // set function
    {
        printf("WANIPConnection name is writable");
        return 0;
    }
}
int dev_WANIPConnection_Uptime(int opt_flag, void *mthdval_struct, int locate[])
{
    //unsigned int wan_ipconn_uptime;
    if (!opt_flag) // get function
    {
        strcpy((char *)mthdval_struct, "2000-00-00T00:00:00");
        return 0;
    }
    else           // set function
    {
        printf("WANIPConnection uptime is unwritable");
        return -1;
    }
}
int dev_WANIPConnection_LastConnectionError(int opt_flag, void *mthdval_struct, int locate[])
{
    char wan_ipconn_lastconnerr[31];  
    if (!opt_flag) // get function
    {
        strcpy(wan_ipconn_lastconnerr, "ERROR_NONE");
        strcpy((char *)mthdval_struct, wan_ipconn_lastconnerr);
        return 0;
    }
    else           // set function
    {
	printf("WANIPConnection LastConnectionError is unwritable");
        return -1;
    }
}
int dev_WANIPConnection_AutoDisconnectTime(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANIPConnection_IdleDisconnectTime(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANIPConnection_WarnDisconnectDelay(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANIPConnection_RSIPAvailable(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANIPConnection_NATEnabled(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        //res = BcmDb_getWanInfo(&wanid, &ipinfo);
            *((int *)mthdval_struct) = 1;
	    printf("Get WANIPConnection NATEnabled success!\n");
	    return 0;
    }
    else           // set function
    {
            printf("Set WANIPConnection NATEnabled  write success!\n");
            return 0;
    }
}
int dev_WANIPConnection_AddressingType(int opt_flag, void *mthdval_struct, int locate[])
{
    char wan_ipconn_addrtype[7];
    if (!opt_flag) // get function
    {
        strcpy(wan_ipconn_addrtype, "DHCP");
        strcpy((char *)mthdval_struct, wan_ipconn_addrtype);
        return 0;
    }
    else           // set function
    {
        printf("WANIPConnection AddressingType is writable\n");
        return 0;
    }
}
int dev_WANIPConnection_ExternalIPAddress(int opt_flag, void *mthdval_struct, int locate[])
{
    char wan_ipconn_extipaddr[20];
    if (!opt_flag) // get function
    {
        strcpy(wan_ipconn_extipaddr, "127.0.0.1");
        strcpy((char *)mthdval_struct, wan_ipconn_extipaddr);
        return 0;
    }
    else           // set function
    {
        printf("WANIPConnection ExternalIPAddress is writable\n");
        return 0;
    }

}
int dev_WANIPConnection_SubnetMask(int opt_flag, void *mthdval_struct, int locate[])
{
    char wan_ipconn_submask[20];
    if (!opt_flag) // get function
    {
        strcpy(wan_ipconn_submask, "255.255.1.0");
        strcpy((char *)mthdval_struct, wan_ipconn_submask);
        return 0;
    }
    else           // set function
    {
        printf("WANIPConnection SubnetMask is writable\n");
        return 0;
    }

}
int dev_WANIPConnection_DefaultGateway(int opt_flag, void *mthdval_struct, int locate[])
{
    char wan_ipconn_gateway[20];
    if (!opt_flag) // get function
    {
        strcpy(wan_ipconn_gateway, "192.168.0.1");
        strcpy((char *)mthdval_struct, wan_ipconn_gateway);
        return 0;
    }
    else           // set function
    {
        printf("WANIPConnection DefaultGateway is writable\n");
        return 0;
    }

}
int dev_WANIPConnection_DNSEnabled(int opt_flag, void *mthdval_struct, int locate[])
{
    int wan_ipconn_dnsenable;
    if (!opt_flag) // get function
    {
        wan_ipconn_dnsenable = 1;
        *((int *)mthdval_struct) = wan_ipconn_dnsenable;
        return 0;
    }
    else           // set function
    {
        printf("WANIPConnection DNSEnabled is writable\n");
        return 0;
    }
}
int dev_WANIPConnection_DNSOverrideAllowed(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANIPConnection_DNSServers(int opt_flag, void *mthdval_struct, int locate[])
{
    char wan_ipconn_dnsserver[20];
    if (!opt_flag) // get function
    {
        strcpy(wan_ipconn_dnsserver, "172.168.0.1");
        strcpy((char *)mthdval_struct, wan_ipconn_dnsserver);
        return 0;
    }
    else           // set function
    {
        printf("WANIPConnection DNSServers is writable\n");
        return 0;
    }

}
int dev_WANIPConnection_MaxMTUSize(int opt_flag, void *mthdval_struct, int locate[])
{
    unsigned int wan_ipconn_mtusize;
    if (!opt_flag) // get function
    {
        wan_ipconn_mtusize = 10;
        *((unsigned int *)mthdval_struct) = wan_ipconn_mtusize;
        return 0;
    }
    else           // set function
    {
        printf("WANIPConnection MaxMTUSize is writable\n");
        return 0;
    }

}
int dev_WANIPConnection_MACAddress(int opt_flag, void *mthdval_struct, int locate[])
{
    char wan_ipconn_mac[20];
    if (!opt_flag) // get function
    {
        strcpy(wan_ipconn_mac, "00:00:00:00:00:00");
        strcpy((char *)mthdval_struct, wan_ipconn_mac);
        return 0;
    }
    else           // set function
    {
        printf("WANIPConnection MACAddress is unwritable\n");
        return -1;
    }

}
int dev_WANIPConnection_MACAddressOverride(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANIPConnection_ConnectionTrigger(int opt_flag, void *mthdval_struct, int locate[])
{
    char wan_ipconn_trigger[20];
    if (!opt_flag) // get function
    {
        strcpy(wan_ipconn_trigger, "");
        strcpy((char *)mthdval_struct, wan_ipconn_trigger);
        return 0;
    }
    else           // set function
    {
        printf("WANIPConnection ConnectionTrigger is writable\n");
        return 0;
    }
                                                                                
}
int dev_WANIPConnection_RouteProtocolRx(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANIPConnection_PortMappingNumberOfEntries(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}



//InternetGatewayDevice.WANDevice.{i}.WAN-ConnectionDevice.{i}.WANIPConnection.{i}.-PortMapping.{i}.

int dev_WANIP_PortMap_PortMappingEnabled(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANIP_PortMap_PortMappingLeaseDuration(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANIP_PortMap_RemoteHost(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANIP_PortMap_ExternalPort(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANIP_PortMap_InternalPort(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANIP_PortMap_PortMappingProtocol(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANIP_PortMap_InternalClient(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANIP_PortMap_PortMappingDescription(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}

//InternetGatewayDevice.WANDevice.{i}.WAN-ConnectionDevice.{i}.WANIPConnection.{i}.-Stats.

int dev_WANIPConnectStats_EthernetBytesSent(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANIPConnectStats_EthernetBytesReceived(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANIPConnectStats_EthernetPacketsSent(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANIPConnectStats_EthernetPacketsReceived(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}


//InternetGatewayDevice.WANDevice.{i}.WAN-ConnectionDevice.{i}.WANPPPConnection.{i}

int dev_WANPPP_Enable(int opt_flag, void *mthdval_struct, int locate[])
{
    int wanppp_enable;
    
    if (!opt_flag) // get function
    {
        //Call native function to get wanppp_uptime -- TO DO
        wanppp_enable = 0;
        *((int *)mthdval_struct) = wanppp_enable;
        printf("wanppp_enable is %d.\n", wanppp_enable);
        return 0;
    }
    else           // set function
    {
        printf("Set wanppp_enable successfully.\n");
        return 0;
    }
}
int dev_WANPPP_ConnectionStatus(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        //Call native function to get WANPPP_ConnectionStatus -- TO DO
        strcpy((char *)mthdval_struct, "Connecting");
        //printf("WANPPP_ConnectionStatus is %s.\n", (char *)mthdval_struct);
        return 0;
    }
    else           // set function
    {
        printf("WANPPP_ConnectionStatus is unwritable.\n");
        return -1;
    }
}
int dev_WANPPP_PossibleConnectionTypes(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        //Call native function to get WANPPP_PossibleConnectionTypes -- TO DO
        strcpy((char *)mthdval_struct, "IP_Routed");
        //printf("WANPPP_PossibleConnectionTypes is %s.\n", (char *)mthdval_struct);
        return 0;
    }
    else           // set function
    {
        printf("WANPPP_PossibleConnectionTypes is unwritable.\n");
        return -1;
    }
}
int dev_WANPPP_ConnectionType(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        //Call native function to get WANPPP_ConnectionType -- TO DO
        strcpy((char *)mthdval_struct, "IP_Routed");
        //printf("WANPPP_ConnectionType is %s.\n", (char *)mthdval_struct);
        return 0;
    }
    else           // set function
    {
         //Call native function to get WANPPP_ConnectionType -- TO DO
        printf("Set WANPPP_ConnectionType successfully.\n");
        return 0;
    }
}
int dev_WANPPP_Name(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        //Call native function to get WANPPP_Name -- TO DO
        strcpy((char *)mthdval_struct, "AAAA");
        //printf("WANPPP_Name is %s.\n", (char *)mthdval_struct);
        return 0;
    }
    else           // set function
    {
        //Call native function to set WANPPP_Name -- TO DO
        printf("Set WANPPP_Name successfully.\n");
        return 0;
    }
}
int dev_WANPPP_Uptime(int opt_flag, void *mthdval_struct, int locate[])
{
    unsigned int wanppp_uptime;
    
    if (!opt_flag) // get function
    {
        //Call native function to get wanppp_uptime -- TO DO
        //wanppp_uptime = 1234;
        //*((unsigned int *)mthdval_struct) = wanppp_uptime;
        //printf("wanppp_uptime is %d.\n", wanppp_uptime);
        strcpy((char *)mthdval_struct, "2000-00-00T00:00:00");
        return 0;
    }
    else           // set function
    {
        printf("wanppp_uptime is unwritable.\n");
        return -1;
    }
}
int dev_WANPPP_LastConnectionError(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        //Call native function to get WANPPP_LastConnectionError -- TO DO
        strcpy((char *)mthdval_struct, "ERROR_ISP_TIME_OUT");
        //printf("WANPPP_LastConnectionError is %s.\n", (char *)mthdval_struct);
        return 0;
    }
    else           // set function
    {
        printf("WANPPP_LastConnectionError is unwritable.\n");
        return -1;
    }
}
int dev_WANPPP_AutoDisconnectTime(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANPPP_IdleDisconnectTime(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANPPP_WarnDisconnectDelay(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANPPP_RSIPAvailable(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANPPP_NATEnabled(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        //res = BcmDb_getWanInfo(&wanid, &waninfo);
            *((int *)mthdval_struct) = 0;
            printf("Get WANIPConnection NATEnabled success!\n");
            return -1;
                                                                                
    }
    else           // set function
    {
            printf("Set WANIPConnection NATEnabled  write success!\n");
            return 0;
    }
}
    
int dev_WANPPP_Username(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        //Call native function to get WANPPP_Username -- TO DO
            strcpy((char *)mthdval_struct, "worksys");
            printf("Get ppp Username information success\n");
            return 0;
    }
    else           // set function
    {
        //Call native function to set WANPPP_Username -- TO DO
            printf("set and get ppp information success\n");
            return 0;
    }
}
int dev_WANPPP_Password(int opt_flag, void *mthdval_struct, int locate[])
{

    if (!opt_flag) // get function
    {
            strcpy((char *)mthdval_struct, "");
            return 0;
    }
    else           // set function
    {
            printf("set and get ppp information success!\n");
            return 0;
    }
}
int dev_WANPPP_PPPEncryptionProtocol(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANPPP_PPPCompressionProtocol(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANPPP_PPPAuthenticationProtocol(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANPPP_ExternalIPAddress(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        //Call native function to get WANPPP_ExternalIPAddress -- TO DO
        strcpy((char *)mthdval_struct, "202.125.201.12");
        printf("WANPPP_ExternalIPAddress is %s.\n", (char *)mthdval_struct);
        return 0;
    }
    else           // set function
    {
        printf("WANPPP_ExternalIPAddress is unwritable.\n");
        return -1;
    }
}
int dev_WANPPP_RemoteIPAddress(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        //Call native function to get WANPPP_RemoteIPAddress -- TO DO
        strcpy((char *)mthdval_struct, "192.69.154.25");
        printf("WANPPP_RemoteIPAddress is %s.\n", (char *)mthdval_struct);
        return 0;
    }
    else           // set function
    {
        printf("WANPPP_RemoteIPAddress is unwritable.\n");
        return -1;
    }
}
int dev_WANPPP_MaxMRUSize(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANPPP_CurrentMRUSize(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANPPP_DNSEnabled(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANPPP_DNSOverrideAllowed(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANPPP_DNSServers(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        //Call native function to get WANPPP_DNSServers -- TO DO
        strcpy((char *)mthdval_struct, "192.65.48.95");
        printf("WANPPP_DNSServers is %s.\n", (char *)mthdval_struct);
        return 0;
    }
    else           // set function
    {
        //Call native function to set WANPPP_DNSServers -- TO DO
        printf("Set WANPPP_DNSServers successfully.\n");
        return 0;
    }
}
int dev_WANPPP_MACAddress(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANPPP_MACAddressOverride(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANPPP_TransportType(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANPPP_PPPoEACName(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        //Call native function to get WANPPP_PPPoEACName -- TO DO
        strcpy((char *)mthdval_struct, "Admin");
        printf("WANPPP_PPPoEACName is %s.\n", (char *)mthdval_struct);
        return 0;
    }
    else           // set function
    {
        //Call native function to set WANPPP_PPPoEACName -- TO DO
        printf("Set WANPPP_PPPoEACName successfully.\n");
        return 0;
    }
}
int dev_WANPPP_PPPoEServiceName(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        //Call native function to get WANPPP_PPPoEServiceName -- TO DO
        strcpy((char *)mthdval_struct, "ASDF");
        printf("WANPPP_PPPoEServiceName is %s.\n", (char *)mthdval_struct);
        return 0;
    }
    else           // set function
    {
        //Call native function to set WANPPP_PPPoEServiceName -- TO DO
        printf("Set WANPPP_PPPoEServiceName successfully.\n");
        return 0;
    }
}
int dev_WANPPP_ConnectionTrigger(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        //Call native function to get WANPPP_ConnectionTrigger -- TO DO
        strcpy((char *)mthdval_struct, "OnDemand");
        printf("WANPPP_ConnectionTrigger is %s.\n", (char *)mthdval_struct);
        return 0;
    }
    else           // set function
    {
        //Call native function to set WANPPP_ConnectionTrigger -- TO DO
        printf("Set WANPPP_ConnectionTrigger successfully.\n");
        return 0;
    }
}
int dev_WANPPP_RouteProtocolRx(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANPPP_PPPLCPEcho(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANPPP_PPPLCPEchoRetry(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANPPP_PortMappingNumberOfEntries(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}

//InternetGatewayDevice.WANDevice.{i}.WAN-ConnectionDevice.{i}.WANPPPConnection.-{i}.PortMapping.{i}.
int dev_WANPPP_PortMap_PortMappingEnabled(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANPPP_PortMap_PortMappingLeaseDuration(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANPPP_PortMap_RemoteHost(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANPPP_PortMap_ExternalPort(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANPPP_PortMap_InternalPort(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANPPP_PortMap_PortMappingProtocol(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANPPP_PortMap_InternalClient(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANPPP_PortMap_PortMappingDescription(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}

////InternetGatewayDevice.WANDevice.{i}.WAN-ConnectionDevice.{i}.WANPPPConnection.-{i}.Stats
int dev_WANPPP_Stats_EthernetBytesSent(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANPPP_Stats_EthernetBytesReceived(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANPPP_Stats_EthernetPacketsSent(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WANPPP_Stats_EthernetPacketsReceived(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}


//add/delete object
int dev_obj_forward(int opt_flag, int* instance_num, int locate[])
{
    if (!opt_flag) // add obj  function
    {
        int res = 0;
        char obj_name[2];
        sprintf(&obj_name[0], "%s", "F");
        obj_name[1] = '\0';
        res = get_instance_num(obj_name, instance_num);
        if (res != 0) {
            printf("get instance number fail !\n");
            return -1;
        }

        printf("New add instance number is %d\n", *instance_num);
        printf("add wanconnection device success!\n");
        return 1;

    }
    else           // del obj function
    {
        int res = 0;
        char obj_name[3];
        sprintf(&obj_name[0], "%s", "F");
        sprintf(&obj_name[1], "%c", (locate[0]+48));
        obj_name[2] = '\0';

        res = del_obj_conf(obj_name, instance_num);
        if (res != 0 ) {
            printf("delete object %d fail !\n", *instance_num);
            return -1;
        }
        printf("Del instance number %d success!\n", *instance_num);
        return 0;
    }
}



int dev_obj_IPInterface(int opt_flag, int* instance_num, int locate[])
{
    if (!opt_flag) // add obj  function
    {
        int res = 0;
        char obj_name[3];
        sprintf(&obj_name[0], "%c", (locate[0]+48));
        sprintf(&obj_name[1], "%s", "L");
        obj_name[2] = '\0';
        res = get_instance_num(obj_name, instance_num);
        if (res != 0) {
            printf("get instance number fail !\n");
            return -1;
        }

        printf("New add instance number is %d\n", *instance_num);
        printf("add wanconnection device success!\n");
        return 1;

    }
    else           // del obj function
    {
        int res = 0;
        char obj_name[4];
        sprintf(&obj_name[0], "%c", (locate[0]+48));
        sprintf(&obj_name[1], "%s", "L");
        sprintf(&obj_name[2], "%c", (locate[1]+48));
        obj_name[3] = '\0';

        res = del_obj_conf(obj_name, instance_num);
        if (res != 0 ) {
            printf("delete ip interface object %d fail !\n", *instance_num);
            return -1;
        }
        printf("Del instance number %d success!\n", *instance_num);
        return 0;
    }
}



int dev_obj_wanconndev(int opt_flag, int* instance_num, int locate[])
{
    if (!opt_flag) // add obj  function
    {
        int res = 0; 
        char obj_name[3];
        sprintf(&obj_name[0], "%c", (locate[0]+48));
        sprintf(&obj_name[1], "%s", "W");
        obj_name[2] = '\0';
        res = get_instance_num(obj_name, instance_num);
        if (res != 0) {
            printf("get instance number fail !\n");
            return -1;
        }
        
        printf("New add instance number is %d\n", *instance_num);
        printf("add wanconnection device success!\n");
        return 1;
    }
    else           // del obj function
    {   
        int res = 0;
        char obj_name[4];
        sprintf(&obj_name[0], "%c", (locate[0]+48));
        sprintf(&obj_name[1], "%s", "W");
        sprintf(&obj_name[2], "%c", (locate[1]+48));
        obj_name[3] = '\0';

        res = del_obj_conf(obj_name, instance_num);
        if (res != 0 ) {
            printf("delete wanconnection object %d fail !\n", *instance_num);
            return -1;
        }
        printf("Del instance number %d success!\n", *instance_num);
        return 0;
    }
}

int dev_obj_wanipconn(int opt_flag, int* instance_num, int locate[])
{
    if (!opt_flag) // add obj  function
    {   
        int res = 0;
        char obj_name[5];
        sprintf(&obj_name[0], "%c", (locate[0]+48));
        sprintf(&obj_name[1], "%s", "W");
        sprintf(&obj_name[2], "%c", (locate[1]+48));
        sprintf(&obj_name[3], "%s", "I");
        obj_name[4] = '\0';

        res = get_instance_num(obj_name, instance_num);
        if (res != 0) {
            printf("get instance number fail !\n");
            return -1;
        }
        printf("New add instance number is %d\n", *instance_num);
        printf("add wanconnection ip_connection success!\n");
        return 0;
    }
    else           // del obj function
    {   
        int i=0;
        for (i =0; i<4; i++)
        printf("locate[%d] = %d\n", i , locate[i]);
        int res = 0;
        char obj_name[6];
        sprintf(&obj_name[0], "%c", (locate[0]+48));
        sprintf(&obj_name[1], "%s", "W");
        sprintf(&obj_name[2], "%c", (locate[1]+48));
        sprintf(&obj_name[3], "%s", "I");
        sprintf(&obj_name[4], "%c", (locate[2]+48));
        obj_name[5] = '\0';
        
        res = del_obj_conf(obj_name, instance_num);
        if (res != 0 ) {
            printf("delete wanconnection ip_connection object %d fail !\n", *instance_num);
            return -1;
        }
        printf("Del instance number %d success!\n", *instance_num);         
        return 0;
    }
}

int dev_obj_wanpppconn(int opt_flag, int *instance_num, int locate[])
{
    if (!opt_flag) // add obj  function
    {
        int res;
        char obj_name[5];
        sprintf(&obj_name[0], "%c", (locate[0]+48));
        sprintf(&obj_name[1], "%s", "W");
        sprintf(&obj_name[2], "%c", (locate[1]+48));
        sprintf(&obj_name[3], "%s", "N");
        obj_name[4] = '\0';

        res = get_instance_num(obj_name, instance_num);
        if (res != 0) {
            printf("get instance number fail !\n");
            return -1;
        }
        printf("add wanconnection ppp_connection success!\n");
        printf("New add instance number is %d\n", *instance_num);
        return 0;
    }
    else           // del obj function
    {
        int res;
        char obj_name[6];
        sprintf(&obj_name[0], "%c", (locate[0]+48));
        sprintf(&obj_name[1], "%s", "W");
        sprintf(&obj_name[2], "%c", (locate[1]+48));
        sprintf(&obj_name[3], "%s", "N");
        sprintf(&obj_name[4], "%c", (locate[2]+48));
        obj_name[5] = '\0';

        res = del_obj_conf(obj_name, instance_num);
        if (res != 0 ) {
            printf("delete wanconnection ppp_connection object %d fail !\n", *instance_num);
            return -1;
        }
        printf("Del instance number %d success!\n", *instance_num);
        return 0;
    }
    
}

int dev_obj_ipportmap(int opt_flag, int* instance_num, int locate[])
{
    if (!opt_flag) // add obj  function
    {
        int res;
        char obj_name[6];
        sprintf(&obj_name[0], "%c", (locate[0]+48));
        sprintf(&obj_name[1], "%s", "W");
        sprintf(&obj_name[2], "%c", (locate[1]+48));
        sprintf(&obj_name[3], "%s", "I");
        sprintf(&obj_name[4], "%c", (locate[2]+48));
        obj_name[5] = '\0';

        res = get_instance_num(obj_name, instance_num);
        if (res != 0) {
            printf("get instance number fail !\n");
            return -1;
        }
        printf("New add instance number is %d\n", *instance_num);
        printf("add wanconnection ip_portmap success!\n");
        return 0;
    }
    else           // del obj function
    {
        int res;
        char obj_name[7];
        sprintf(&obj_name[0], "%c", (locate[0]+48));
        sprintf(&obj_name[1], "%s", "W");
        sprintf(&obj_name[2], "%c", (locate[1]+48));
        sprintf(&obj_name[3], "%s", "I");
        sprintf(&obj_name[4], "%c", (locate[2]+48));
        sprintf(&obj_name[5], "%c", (locate[3]+48));
        obj_name[6] = '\0';

        res = del_obj_conf(obj_name, instance_num);
        if (res != 0 ) {
            printf("delete wanconnection ip_portmap object %d fail !\n", *instance_num);
            return -1;
        }
        printf("Del instance number %d success!\n", *instance_num);
        return 0;
    }
}

int dev_obj_pppportmap(int opt_flag, int* instance_num, int locate[])
{
    if (!opt_flag) // add obj  function
    {
        int res;
        char obj_name[6];
        sprintf(&obj_name[0], "%c", (locate[0]+48));
        sprintf(&obj_name[1], "%s", "W");
        sprintf(&obj_name[2], "%c", (locate[1]+48));
        sprintf(&obj_name[3], "%s", "N");
        sprintf(&obj_name[4], "%c", (locate[2]+48));
        obj_name[5] = '\0';

        res = get_instance_num(obj_name, instance_num);
        if (res != 0) {
            printf("get instance number fail !\n");
            return -1;
        }
        printf("New add instance number is %d\n", *instance_num);
        printf("add wanconnection ppp_portmap success!\n");
        return 0;
    }
    else           // del obj function
    {
        int res;
        char obj_name[7];
        sprintf(&obj_name[0], "%c", (locate[0]+48));
        sprintf(&obj_name[1], "%s", "W");
        sprintf(&obj_name[2], "%c", (locate[1]+48));
        sprintf(&obj_name[3], "%s", "N");
        sprintf(&obj_name[4], "%c", (locate[2]+48));
        sprintf(&obj_name[5], "%c", (locate[3]+48));
        obj_name[6] = '\0';

        res = del_obj_conf(obj_name, instance_num);
        if (res != 0 ) {
            printf("delete wanconnection ppp_portmap object %d fail !\n", *instance_num);
            return -1;
        }
        printf("Del instance number %d success!\n", *instance_num);
        return 0;
    }
}




//get multi object instance number
int get_wanconndev_instance(int* instance_name, int locate[])
{
     int i, res;
     char obj_name[3];

     sprintf(&obj_name[0], "%c", (locate[0]+48));
     sprintf(&obj_name[1], "%s", "W");
     obj_name[2] = '\0';

     res = get_init_instance_num(obj_name, instance_name);
     if (res != 0) {
         printf("get init instance num fail !\n");
         return -1;
     }
     for (i = 0; i < 9; i++) {
          printf("instance[%d] = %d\n", i, instance_name[i]);
     }
     return 0;
}


int get_wanipconn_instance(int* instance_name, int locate[])
{
     int i, res;
     char obj_name[5];
     sprintf(&obj_name[0], "%c", (locate[0]+48));
     sprintf(&obj_name[1], "%s", "W");
     sprintf(&obj_name[2], "%c", (locate[1]+48));
     sprintf(&obj_name[3], "%s", "I");
     obj_name[4] = '\0';
     res = get_init_instance_num(obj_name, instance_name);
     if (res != 0) {
         printf("get init instance num fail !\n");
         return -1;
     }
     for (i = 0; i < 9; i++) {
          printf("instance[%d] = %d\n", i, instance_name[i]);
     }
     return 0;
}


int get_wanpppconn_instance(int* instance_name, int locate[])
{  
     int i, res;
     char obj_name[5];
     sprintf(&obj_name[0], "%c", (locate[0]+48));
     sprintf(&obj_name[1], "%s", "W");
     sprintf(&obj_name[2], "%c", (locate[1]+48));
     sprintf(&obj_name[3], "%s", "N");
     obj_name[4] = '\0';

     res = get_init_instance_num(obj_name, instance_name);
     if (res != 0) {
         printf("get init instance num fail !\n");
         return -1;
     }
     for (i = 0; i < 9; i++) {
          printf("instance[%d] = %d\n", i, instance_name[i]);
     } 
     return 0;
}


int get_ipportmap_instance(int* instance_name, int locate[])
{
     int i, res;
     char obj_name[6];
     sprintf(&obj_name[0], "%c", (locate[0]+48));
     sprintf(&obj_name[1], "%s", "W");
     sprintf(&obj_name[2], "%c", (locate[1]+48));
     sprintf(&obj_name[3], "%s", "I");
     sprintf(&obj_name[4], "%c", (locate[2]+48));
     obj_name[5] = '\0';

     res = get_init_instance_num(obj_name, instance_name);
     if (res != 0) {
         printf("get init instance num fail !\n");
         return -1;
     }
     for (i = 0; i < 9; i++) {
          printf("instance[%d] = %d\n", i, instance_name[i]);
     }
     return 0;
}


int get_pppportmap_instance(int *instance_name, int locate[])
{
     int i, res;
     char obj_name[6];
     sprintf(&obj_name[0], "%c", (locate[0]+48));
     sprintf(&obj_name[1], "%s", "W");
     sprintf(&obj_name[2], "%c", (locate[1]+48));
     sprintf(&obj_name[3], "%s", "N");
     sprintf(&obj_name[4], "%c", (locate[2]+48));
     obj_name[5] = '\0';

     res = get_init_instance_num(obj_name, instance_name);
     if (res != 0) {
         printf("get init instance num fail !\n");
         return -1;
     }
     for (i = 0; i < 9; i++) {
          printf("instance[%d] = %d\n", i, instance_name[i]);
     }
     return 0;
}


int get_forwarding_instance(int *instance_name,  int locate[])
{
     int i, res;
     char obj_name[2];
    
     res = init_multi_conf();
     if (res != 0) {
         printf("init multi conf file fail!\n");
         return -1;
     }

     sprintf(&obj_name[0], "%s", "F");
     obj_name[1] = '\0';

     res = get_init_instance_num(obj_name, instance_name);
     if (res != 0) {
         printf("get init instance num fail !\n");
         return -1;
     }
     for (i = 0; i < 9; i++) {
          printf("instance[%d] = %d\n", i, instance_name[i]);
     }
     return 0;
}


int get_ipinterface_instance(int *instance_name, int locate[])
{
     int i, res;
     char obj_name[3];
     sprintf(&obj_name[0], "%c", (locate[0]+48));
     sprintf(&obj_name[1], "%s", "L");
     obj_name[2] = '\0';

     res = get_init_instance_num(obj_name, instance_name);
     if (res != 0) {
         printf("get init instance num fail !\n");
         return -1;
     }
     for (i = 0; i < 9; i++) {
          printf("instance[%d] = %d\n", i, instance_name[i]);
     }
     return 0;
}




//get instance number
int get_instance_num(char *obj_name, int *instance_num)
{   
    int i = 0, j = 0, record_num; 
    int res, num = 1;
    res = read_obj_conf(&record_num);
    if (res != 0){
        printf("read obj conf fail!\n");
        return -1;
    }
    for (i = 0; i < record_num; i++) {
        if (strcmp (multi_obj_buf[i].param_path, obj_name) == 0) {
            break;
        }
    }
   
    if (i == record_num) {
        *instance_num = 1;
        strcpy (multi_obj_buf[i].param_path, obj_name);
        multi_obj_buf[i].used_num = 1;
        record_num++;
    } else {
          for (j = 0; j < 9; j++) {
              if ((multi_obj_buf[i].used_num & num) == 0) {
                  *instance_num = j+1;
                  printf("New instance number is %d\n", *instance_num);
                  break;
              } else {
                  num *= 2;         
              }
          }
          if (j == 9) {
              printf("object instance number must <= 9 !\n");
              return -1;
          }
          multi_obj_buf[i].used_num += num;
    }
        
    //write to obj conf
    res = write_obj_conf (record_num);
    if (res != 0) {
        return -1;
    }
    return 0;    
}


//get init instance number
int get_init_instance_num(char *obj_name, int *instance_num)
{
    int i = 0, j = 0, k = 0,record_num = 0, res = 0, num = 1;
    res = read_obj_conf(&record_num);
    if (res != 0){
        printf("read obj conf fail!\n");
        return -1;
    }
    printf("object path locate string: %s\n", obj_name);
 
    for (i = 0; i < record_num; i++) {
        if (strcmp (multi_obj_buf[i].param_path, obj_name) == 0)
            break;
    }
    for (j = 0; j < 9; j++) {
         if ((multi_obj_buf[i].used_num & num) != 0) {
             instance_num[k] = j+1;
             k++;
             num *= 2;
         } else 
             num *= 2;
    }
    return 0;
}


int read_obj_conf(int *record_num)
{
    int i = 0;
    FILE *fp = NULL;
     
    //Open obj conf
    fp = fopen (obj_conf_path, "rb+");
    if (fp == NULL) {
        printf("Can't open obj.conf file\n");
        return -1;
    }
   
    //read number of element from obj.conf
    if (fread (record_num, sizeof(int), 1, fp) != 1) {
        printf("Can't read \"num\" from %s\n", obj_conf_path);
        fclose(fp);
        return (-1);
    }
    printf("record number is %d\n", *record_num);
    //read elememts from obj.conf file
    for (i = 0; i < *record_num; i++) {
        if (fread (&multi_obj_buf[i], sizeof(TR_multi_obj), 1 ,fp) != 1) {
            printf("Can't read elements form obj.conf file\n");
            fclose(fp);
            return -1;
        }
    }
    for (i = 0; i < *record_num; i++) {
        printf("obj_name=%s  used_num=%d\n", multi_obj_buf[i].param_path, multi_obj_buf[i].used_num);
    }
    fclose(fp);

    return 0;
}


int write_obj_conf(int record_num)
{
    int i = 0;
    FILE *fp = NULL;
   
    //Open config file
    fp = fopen (obj_conf_path, "wb+");
    if (fp == NULL) {
        printf("Can't open obj.conf file\n");
        return -1;
    }

    if (fwrite (&record_num, sizeof(int), 1, fp) != 1) {
        printf("Can't write num into obj.conf file\n");
        fclose(fp);
        return(-1);
    }

    for (i = 0; i < record_num; i++) {
        if (fwrite (&multi_obj_buf[i], sizeof(TR_multi_obj), 1 ,fp) != 1) {
            printf("Can't write elements  into obj.conf\n");
            fclose(fp);
            return(-1);
        }
    }

    for (i = 0; i < record_num; i++) {
        printf("obj_name=%s  used_num=%d\n", multi_obj_buf[i].param_path, multi_obj_buf[i].used_num);
    }


    fclose(fp);
    
    return 0;
}


int del_obj_conf(char *obj_name, int *instance_num)
{
    int res = 0, i = 0, j = 0 ,k = 0;
    int num = 1, record_num = 0;
    char *locat = NULL;
    char pre_obj_name[6];

    //read obj conf
    res = read_obj_conf (&record_num);
    if (res != 0) {
        return -1;
    }
    
    for (i = 0; i < 7; i++){
        if (obj_name[i] != '\0')
            pre_obj_name[i] = obj_name[i];
        else 
            break;
    }
    pre_obj_name[--i] = '\0';      

    printf("pre_obj_name : %s\n", pre_obj_name);
    for (i = 0; i < record_num; i ++) {
        if (strcmp (multi_obj_buf[i].param_path, pre_obj_name) == 0) {
            break;
        }
    }
    if (i == record_num) {
        printf("search object in config file fail !\n");
        return -1;
    } else {
          for (k = 0; k < (*instance_num-1); k++) {
              num *= 2;
          }
          multi_obj_buf[i].used_num -= num;
    }
    printf("multi_obj_buf[%d].used_num = %d\n", i,  multi_obj_buf[i].used_num);
    
    printf("loop i=%d record_num=%d\n", i, record_num);
    
    //delete obj path from config file
    if (i < record_num) {
        j = i;
        //delete record which used num is 0
        if (multi_obj_buf[i].used_num == 0) {
            for (; i < record_num - 1; i ++) {
                strcpy (multi_obj_buf[i].param_path, multi_obj_buf[i+1].param_path);
                multi_obj_buf[i].used_num = multi_obj_buf[i+1].used_num;
            }
            record_num--;
        }

        for (j = 0; j < record_num; j++) {
            locat = strstr (multi_obj_buf[j].param_path, obj_name);
            if (locat != NULL) {
                //check the path is need to be deleted
                if (strcmp (locat, multi_obj_buf[j].param_path) == 0) {
                    for (; j < record_num - 1; j++) {
                        strcpy(multi_obj_buf[j].param_path, multi_obj_buf[j+1].param_path);
                        multi_obj_buf[j].used_num = multi_obj_buf[j+1].used_num;
                    }
                    record_num--;
                }
            }
        }
    }
    
    //rewrite to obj conf
    res = write_obj_conf (record_num);
    if (res != 0) {
        return -1;
    }
    return 0;
}


int init_multi_conf()
{  
   FILE *fp;
   int i = 0, init_size = 3;
   strcpy(multi_obj_buf[0].param_path, "1W");
   multi_obj_buf[0].used_num = 1;
   
   strcpy(multi_obj_buf[1].param_path, "1W1I");
   multi_obj_buf[1].used_num = 1;
   
   strcpy(multi_obj_buf[2].param_path, "1W1N");
   multi_obj_buf[2].used_num = 1;

   if (access(obj_conf_path, F_OK) == -1) {
       printf("The configure file %s doesn't exit, create it\n", obj_conf_path);
        if ((fp = fopen(obj_conf_path, "wb+")) == NULL){
            printf("Can't initialize multi.conf file\n");
            return -1;
        }
        if (fwrite(&init_size, sizeof(int), 1, fp) != 1){
            printf("Can't write initsize into multi.conf file\n");
            fclose(fp);
            return -1;
        }
        for (i = 0; i < init_size; i++) {
            if (fwrite(&multi_obj_buf[i], sizeof(TR_multi_obj), 1 ,fp) != 1) {
                printf("Can't write initial multi_obj into multi.conf\n");
                fclose(fp);
                return -1;
            }
        }
        fclose(fp);
  }
  return 0;
}



