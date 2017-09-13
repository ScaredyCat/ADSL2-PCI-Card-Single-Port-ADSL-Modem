/*****************************************************************************
;
;   (C) Unpublished Work of ADMtek Incorporated.  All Rights Reserved.
;
;       THIS WORK IS AN UNPUBLISHED WORK AND CONTAINS CONFIDENTIAL,
;       PROPRIETARY AND TRADESECRET INFORMATION OF ADMTEK INCORPORATED.
;       ACCESS TO THIS WORK IS RESTRICTED TO (I) ADMTEK EMPLOYEES WHO HAVE A
;       NEED TO KNOW TO PERFORM TASKS WITHIN THE SCOPE OF THEIR ASSIGNMENTS
;       AND (II) ENTITIES OTHER THAN ADMTEK WHO HAVE ENTERED INTO APPROPRIATE
;       LICENSE AGREEMENTS.  NO PART OF THIS WORK MAY BE USED, PRACTICED,
;       PERFORMED, COPIED, DISTRIBUTED, REVISED, MODIFIED, TRANSLATED,
;       ABBRIDGED, CONDENSED, EXPANDED, COLLECTED, COMPILED, LINKED, RECAST,
;       TRANSFORMED OR ADAPTED WITHOUT THE PRIOR WRITTEN CONSENT OF ADMTEK.
;       ANY USE OR EXPLOITATION OF THIS WORK WITHOUT AUTHORIZATION COULD
;       SUBJECT THE PERPERTRATOR TO CRIMINAL AND CIVIL LIABILITY.
;
;------------------------------------------------------------------------------
;
;    Project : WPA
;    Creator : 
;    File    : wpa_cfg.c
;    Abstract: 
;
;    This module contains the following routines:  
;
;Modification History:
;       By              Date     Ver.   Modification Description
;       --------------- -------- -----  --------------------------------------
;
;*****************************************************************************/
#ifdef WLAN_HOSTOS_LINUX
#include <asm/byteorder.h>
#endif //WLAN_HOSTOS_LINUX

#define INCLUDE_OS_DEP
#include "wlan_include.h"

#include "ifx_wdrv_types.h"
#include "wpa_cfg.h"
#include "ifx_wdrv_debug.h"
#include "if_wlan.h"
#include "ifxadm_wdrv_init.h"
#include "radMain.h"
#include "inet_addr.h"

#include "sibapi.h"

#include "1xdefs.h"
#include "1xproto.h"

static  DOT1X_CONF      *pWpaCfg_Config = NULL;

#define UAP_NO_ERROR    OK
#define UAP_UNINITIALIZED NOK
#define UAP_NULL_BUFFER NOK
#define UAP_BAD_VALUE NOK


void WpaCfg_Init_default(void)
{
    WPA_CFG_DATA_T cfg;
    WPA_CFG_T   def[NUMOFSERVER];
    int i;
    
    memset(&def, 0, sizeof(WPA_CFG_T)*NUMOFSERVER);
     
    cfg.enable = REAUTHENABLED_FALSE;
    cfg.period = DEF_REAUTHPERIOD;
    cfg.pDefault = def;
    cfg.num =  0;

#if 0
{ //bj_temp
   struct in_addr *ina= (struct in_addr *)&def[0].ip;
   PRINTTODOM("set RADIUS server info to static, remove this...");
    cfg.num++;
    def[0].ip = 0x0a0101f8;    // 10.1.1.248
    memcpy(def[0].secret, "123456", 6);
    WDEBUG("Radius server: %s\n", inet_ntoa(*ina));
    printMem(def[0].secret, RADIUS_SECRET_LEN, "Radius server secret: ");
}
#else

    if (SIBCfg_isDot1xEnable()){
        int len;
        struct in_addr *ina;
        
        for (i=0; i < NUMOFSERVER ; i++){
            if (!SIBCfg_radServIpAddr(i, &(def[i].ip)) &&
                !SIBCfg_radServPort(i, &(def[i].port)) &&
                !SIBCfg_radServSecret(i, def[i].secret, &len) ){
        
                cfg.num++;
                
                ina= (struct in_addr *)&def[i].ip;
                WDEBUG("index:%d, Radius server: %s, port:%d\n", i, inet_ntoa(*ina), def[i].port);
                printMem(def[i].secret, RADIUS_SECRET_LEN, "Radius server secret: ");

            }
        }
    }
#endif

    WpaCfg_Init(&cfg);
}   

void WpaCfg_unInit(void)
{
    if (pWpaCfg_Config){
        _1xStop();

        RadCli_UnInit();
    
        os_freeMemory(pWpaCfg_Config);
    }
}
/*--------------------------------------------------------------
* ROUTINE NAME - WpaCfg_Init
*---------------------------------------------------------------
* FUNCTION: 
* INPUT:     
* OUTPUT:   
* RETURN:   
* NOTE:     
*---------------------------------------------------------------*/
void WpaCfg_Init(WPA_CFG_DATA_T *map)
{
    UINT32          ip_tmp;
    int             i, j = map->num;


        pWpaCfg_Config = (DOT1X_CONF*)os_getMemory(sizeof(DOT1X_CONF));
        if (!pWpaCfg_Config)
            return;
        
        memset(pWpaCfg_Config, 0, sizeof(DOT1X_CONF));

        
        //convert WPA_CFG_DATA_T to DOT1X_CONF
        //default value from FDK
        pWpaCfg_Config->reAuthEnabled = map->enable;
        pWpaCfg_Config->reAuthPeriod = map->period;
        pWpaCfg_Config->updateCount = map->num; //not used
        
        if (j > NUMOFSERVER)
            j = NUMOFSERVER;
        if (j < 0)
            j = 0;

        memset(pWpaCfg_Config->radiusConf, 0, sizeof(RADIUS_CONF) * NUMOFSERVER);
        for (i=0; i<j; i++)
        {
            ip_tmp = htonl(map->pDefault[i].ip);
            memcpy(pWpaCfg_Config->radiusConf[i].ip, &ip_tmp, 4);
            memcpy(pWpaCfg_Config->radiusConf[i].secret, map->pDefault[i].secret, 
                    RADIUS_SECRET_LEN);
        }
        //default value in here
        memset(pWpaCfg_Config->PSK, 0x00, PSK_LEN);
        pWpaCfg_Config->beSupportedSta = BIT_STA_WPA;
        pWpaCfg_Config->unicastCipher = WPACFG_CIPHER_TKIP;
        pWpaCfg_Config->updGroupKeyInterval = DEF_UPD_GROUP_KEY_INTERVAL;

#if 0
PRINTTODOM(_msg)("set PSK KEY to static \"aaaaaaaa\", remove this...");
memset(pWpaCfg_Config->PSK, 0xaa, 32);
    printMem(pWpaCfg_Config->PSK, 8, "PSK KEY");
#else
    SIBCfg_PSK_key(pWpaCfg_Config->PSK);
    printMem(pWpaCfg_Config->PSK, PSK_LEN, "pWpaCfg_Config->PSK");
    pWpaCfg_Config->unicastCipher  = SIBCfg_wpaPairwise();
    pWpaCfg_Config->updGroupKeyInterval = SIBCfg_wpaGroupRekey();
#endif

    UpdateRadiusServerInfo(pWpaCfg_Config->radiusConf);
    _1xStart();
    
    return;
}



/*--------------------------------------------------------------
* ROUTINE NAME - Dot1xCfg_ReAuthEnabled
*---------------------------------------------------------------
* FUNCTION: 
* INPUT: pValue: WPA_CFG_ENABLED or WPA_CFG_DISABLED,
*                default value is WPA_CFG_DISABLED      
* OUTPUT:   
* RETURN:   
* NOTE:     
*---------------------------------------------------------------*/
INT32 Dot1xCfg_ReAuthEnabled(UINT8 *pValue)
{

    *pValue = pWpaCfg_Config->reAuthEnabled;
    
    return OK;
}

/*--------------------------------------------------------------
* ROUTINE NAME - Dot1xCfg_SetReAuthEnabled
*---------------------------------------------------------------
* FUNCTION: 
* INPUT: pValue: WPA_CFG_ENABLED or WPA_CFG_DISABLED,
*                default value is WPA_CFG_DISABLED    
* OUTPUT:   
* RETURN:   
* NOTE:     
*---------------------------------------------------------------*/
INT32 Dot1xCfg_SetReAuthEnabled(UINT8 value)
{
    
    if (value < WPA_CFG_ENABLED || value > WPA_CFG_DISABLED)
        return NOK;    
        
    
    pWpaCfg_Config->reAuthEnabled = value;
    
    Notify1xCfg_ReAuthEnabled(value);
                        
    return OK;
}

/*--------------------------------------------------------------
* ROUTINE NAME - Dot1xCfg_ReAuthPeriod
*---------------------------------------------------------------
* FUNCTION: 
* INPUT: pValue: default value is 3600 seconds    
* OUTPUT:   
* RETURN:   
* NOTE:     
*---------------------------------------------------------------*/
INT32 Dot1xCfg_ReAuthPeriod(UINT16 *pValue)
{
    
    if (pValue == NULL)
        return NOK;
    
    *pValue = pWpaCfg_Config->reAuthPeriod;
    
    
    return UAP_NO_ERROR;   
}

/*--------------------------------------------------------------
* ROUTINE NAME - Dot1xCfg_SetReAuthPeriod
*---------------------------------------------------------------
* FUNCTION: 
* INPUT:     
* OUTPUT:   
* RETURN:   
* NOTE:     
*---------------------------------------------------------------*/
INT32 Dot1xCfg_SetReAuthPeriod(UINT16 value)
{
    
    pWpaCfg_Config->reAuthPeriod = value;
    
    Notify1xCfg_ReAuthPeriod(value);
    
    return UAP_NO_ERROR;
}

/*--------------------------------------------------------------
* ROUTINE NAME - Dot1xCfg_RadiusIp
*---------------------------------------------------------------
* FUNCTION: 
* INPUT: Index: 0~3
*        pIpAddr: IP address will be pIpAddr[0].pIpAddr[1].pIpAddr[2].pIpAddr[3]   
* OUTPUT:   
* RETURN:   
* NOTE:     
*---------------------------------------------------------------*/
INT32 Dot1xCfg_RadiusIp(UINT8 Index, UINT8 *pIpAddr)
{
    
    if (pIpAddr == NULL)
        return UAP_NULL_BUFFER;

    if (Index > 3)
        return UAP_BAD_VALUE;
    
    memcpy(pIpAddr, pWpaCfg_Config->radiusConf[Index].ip, 4);    
    
    return UAP_NO_ERROR;
}

/*--------------------------------------------------------------
* ROUTINE NAME - Dot1xCfg_SetRadiusIp
*---------------------------------------------------------------
* FUNCTION: 
* INPUT:     
* OUTPUT:   
* RETURN:   
* NOTE:     
*---------------------------------------------------------------*/
INT32 Dot1xCfg_SetRadiusIp(UINT8 Index, UINT8 *pIpAddr)
{
    
    if (pIpAddr == NULL)
        return UAP_NULL_BUFFER;
    
    if (Index > 3)
        return UAP_BAD_VALUE;
        
    
    memcpy(pWpaCfg_Config->radiusConf[Index].ip, pIpAddr, 4);
           
    UpdateRadiusServerInfo(pWpaCfg_Config->radiusConf);    
    
    return UAP_NO_ERROR;
}

/*--------------------------------------------------------------
* ROUTINE NAME - Dot1xCfg_RadiusSecret
*---------------------------------------------------------------
* FUNCTION: 
* INPUT: Index: 0~3
*        pSecret: The secret length <= 32 bytes    
* OUTPUT:   
* RETURN:   
* NOTE:     
*---------------------------------------------------------------*/
INT32 Dot1xCfg_RadiusSecret(UINT8 Index, UINT8 *pSecret)
{
    if (pSecret == NULL)
        return UAP_NULL_BUFFER;

    if (Index > 3)
        return UAP_BAD_VALUE;
    
    memcpy(pSecret, pWpaCfg_Config->radiusConf[Index].secret, RADIUS_SECRET_LEN);    
    
    return UAP_NO_ERROR;
}

/*--------------------------------------------------------------
* ROUTINE NAME - Dot1xCfg_SetRadiusSecret
*---------------------------------------------------------------
* FUNCTION: 
* INPUT:     
* OUTPUT:   
* RETURN:   
* NOTE:     
*---------------------------------------------------------------*/
INT32 Dot1xCfg_SetRadiusSecret(UINT8 Index, UINT8 *pSecret)
{
    if (pSecret == NULL)
        return UAP_NULL_BUFFER;
    
    if (Index > 3)
        return UAP_BAD_VALUE;
        
    
    memcpy(pWpaCfg_Config->radiusConf[Index].secret, pSecret, RADIUS_SECRET_LEN);
    
    UpdateRadiusServerInfo(pWpaCfg_Config->radiusConf);
    
    return UAP_NO_ERROR;
}

/*-----------------------------------------------------------------------------
* ROUTINE NAME - Dot1xCfg_PSK
*------------------------------------------------------------------------------
* FUNCTION: The configuration options an AP should support are:
*           3. Pre-shared key for WPA which can be an ASCII passphrase or a 256
*              bit key
*
* INPUT   : PSKArray: a pointer point to 32 bytes array
* OUTPUT  : 
* RETURN  : 
* NOTE:
*-----------------------------------------------------------------------------*/
INT32 Dot1xCfg_PSK(UINT8 *PSKArray)
{
    
    if (PSKArray == NULL)
        return UAP_NULL_BUFFER;

    memcpy(PSKArray, pWpaCfg_Config->PSK, PSK_LEN);
    
    return UAP_NO_ERROR;
}

void Dot1xCfg_SetPSK_key(UINT8 *input)
{
    memcpy(pWpaCfg_Config->PSK, input, PSK_LEN);
}
/*-----------------------------------------------------------------------------
* ROUTINE NAME - Dot1xCfg_SetPSK
*------------------------------------------------------------------------------
* FUNCTION: 
*
* INPUT   : InputType: PSK_BY_ASCII_PASS or PSK_BY_HEX_CHAR
*           PSKArray: 64 hex characters when InputType = PSK_BY_HEX_CHAR, or
*                     ASCII string which length <= 32 bytes when InputType = PSK_BY_ASCII_PASS
* OUTPUT  : 
* RETURN  : 
* NOTE:
*-----------------------------------------------------------------------------*/
INT32 Dot1xCfg_SetPSK(UINT8 InputType, UINT8 *PSKArray, UINT8 *output)
{
    UINT8 psk[PSK_LEN + 8];
    UINT8 AP8628SSID[33];
    
    if (PSKArray == NULL)
        return UAP_NULL_BUFFER;

    if (InputType < PSK_BY_ASCII_PASS || InputType > PSK_BY_HEX_CHAR)
        return UAP_BAD_VALUE;   
                
    if (InputType == PSK_BY_HEX_CHAR)
    {
        TransAsciiToHex(psk, PSKArray, PSK_LEN);
        memcpy(output, psk, PSK_LEN);        
    }
    else
    {
        memset(AP8628SSID, 0x00, 33);
        SIBCfg_ServiceArea(AP8628SSID);
        WPA_PasswordHash(PSKArray, AP8628SSID + 1, (strlen(AP8628SSID) - 1), psk);
        memcpy(output, psk, PSK_LEN);
    }
    
    //Drop all station when reset PSK.
//    WPA_DropAllStation();

	return UAP_NO_ERROR;
}

/*--------------------------------------------------------------
* ROUTINE NAME - Dot1xCfg_BeSupportedSta
*---------------------------------------------------------------
* FUNCTION: The configuration options an AP should support are:
*           1. Select one or more station configurations to associate to the AP
*              a. WPA
*              b. WEP
*              c. WEP rekeying using the existing 802.1X EAPOL-key message.
*
* INPUT: pValue: 1~7, default value is BIT_STA_WPA  
* OUTPUT:   
* RETURN:   
* NOTE:     
*---------------------------------------------------------------*/
INT32 Dot1xCfg_BeSupportedSta(UINT8 *pValue)
{
    
    if (pValue == NULL)
        return UAP_NULL_BUFFER;
    
    *pValue = pWpaCfg_Config->beSupportedSta;
    
    return UAP_NO_ERROR;
}

/*--------------------------------------------------------------
* ROUTINE NAME - Dot1xCfg_SetBeSupportedSta
*---------------------------------------------------------------
* FUNCTION: 
* INPUT: value: 1~7   
* OUTPUT:   
* RETURN:   
* NOTE:     
*---------------------------------------------------------------*/
INT32 Dot1xCfg_SetBeSupportedSta(UINT8 value)
{
    
    if (value < BIT_STA_WPA || value > (BIT_STA_WPA|BIT_STA_1X|BIT_STA_WEP))
        return UAP_BAD_VALUE;    
        
    pWpaCfg_Config->beSupportedSta = value;
    
    return UAP_NO_ERROR;
}

/*--------------------------------------------------------------
* ROUTINE NAME - Dot1xCfg_UnicastCipher
*---------------------------------------------------------------
* FUNCTION: The configuration options an AP should support are:
*           2. For WPA select the list of available ciphers for unicast
*              a. TKIP
*              b. AES
*
* INPUT: pValue: 1~3, default value is WPACFG_CIPHER_TKIP  
* OUTPUT:   
* RETURN:   
* NOTE:     
*---------------------------------------------------------------*/
INT32 Dot1xCfg_UnicastCipher(UINT8 *pValue)
{
    if (pValue == NULL)
        return UAP_NULL_BUFFER;
    
    *pValue = pWpaCfg_Config->unicastCipher;
    
    return UAP_NO_ERROR;
}

/*--------------------------------------------------------------
* ROUTINE NAME - Dot1xCfg_SetUnicastCipher
*---------------------------------------------------------------
* FUNCTION: 
* INPUT: value: 1~3  
* OUTPUT:   
* RETURN:   
* NOTE:     
*---------------------------------------------------------------*/
INT32 Dot1xCfg_SetUnicastCipher(UINT8 value)
{
    if (value < WPACFG_CIPHER_TKIP || value > (WPACFG_CIPHER_TKIP|WPACFG_CIPHER_AES))
        return UAP_BAD_VALUE;    
        
    pWpaCfg_Config->unicastCipher = value;
    
    return UAP_NO_ERROR;
}

/*--------------------------------------------------------------
* ROUTINE NAME - Dot1xCfg_UpdGroupKeyInterval
*---------------------------------------------------------------
* FUNCTION: The AP control panel will also need to provide configuration for the
*           interval at which 802.1X should update the group key. 
*
* INPUT: pValue: default value is 3600 seconds   
* OUTPUT:   
* RETURN:   
* NOTE:     
*---------------------------------------------------------------*/
INT32 Dot1xCfg_UpdGroupKeyInterval(UINT16 *pValue)
{
    if (pValue == NULL)
        return UAP_NULL_BUFFER;
    
    *pValue = pWpaCfg_Config->updGroupKeyInterval;
    
    return UAP_NO_ERROR;   
}

/*--------------------------------------------------------------
* ROUTINE NAME - Dot1xCfg_SetUpdGroupKeyInterval
*---------------------------------------------------------------
* FUNCTION: 
* INPUT:     
* OUTPUT:   
* RETURN:   
* NOTE:     
*---------------------------------------------------------------*/
INT32 Dot1xCfg_SetUpdGroupKeyInterval(UINT16 value)
{
    pWpaCfg_Config->updGroupKeyInterval = value;
    
    return UAP_NO_ERROR;
}

