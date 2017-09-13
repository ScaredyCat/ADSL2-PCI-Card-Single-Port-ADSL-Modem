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
;    File    : wpa_cfg.h
;    Abstract: 
;
;
;Modification History:
;       By              Date     Ver.   Modification Description
;       --------------- -------- -----  --------------------------------------
;
;*****************************************************************************/
#if __cplusplus
extern "C" {
#endif

#ifndef _WPA_CFG_H_
#define _WPA_CFG_H_

#include "ifx_wdrv_types.h"
#include "radDef.h"

#define DOT1X_NAS_ID_LEN    LENOFNASID
#define DOT1X_RAD_SECRET_MAX_LEN        32

enum WPA_CFG_STATUS
{
    WPA_CFG_ENABLED =1,        /* Enable  ReAuthEnabled */    
    WPA_CFG_DISABLED,          /* Disable ReAuthEnabled */
};

typedef struct WPA_CFG
{
    UINT32 ip;
    UINT16 port;
    char   secret[DOT1X_RAD_SECRET_MAX_LEN+1];  
} WPA_CFG_T;

typedef struct WPA_CFG_DATA
{
    INT32     enable;	
    INT32     period; 
    INT32     num;
    WPA_CFG_T *pDefault;
} WPA_CFG_DATA_T;

#define NUMOFSERVER             4   //RadDef.h
#define RADIUS_SECRET_LEN       32
#define PSK_LEN                 32
#define PSK_PASSPHRASE_MAX_LEN  64

typedef struct _RADIUS_CONF
{
    UINT8	ip[4];
    UINT8	secret[RADIUS_SECRET_LEN];
} RADIUS_CONF;

typedef struct _DOT1X_CONF
{
	UINT8	    reAuthEnabled;			// 1 true, 2 false(default) REAUTHENABLED_TRUE, REAUTHENABLED_FALSE
	UINT8	    updateCount;            //not used
	UINT16	    reAuthPeriod;		    //seconds (default: 3600)
	RADIUS_CONF radiusConf[NUMOFSERVER];
	UINT8       PSK[PSK_LEN];                //Pre-Share Key.
	UINT8       beSupportedSta;
	UINT8       unicastCipher;
	UINT16      updGroupKeyInterval;    //default: 3600 seconds
} DOT1X_CONF;

// used in DOT1X_CONF.beSupportedSta
#define BIT_STA_WPA             0x01        //default
#define BIT_STA_1X              0x02
#define BIT_STA_WEP             0x04

// used in DOT1X_CONF.unicastCipher
#define WPACFG_CIPHER_TKIP      0x01        //default
#define WPACFG_CIPHER_AES       0x02

// used in InputType of Dot1xCfg_SetPSK()
#define PSK_BY_ASCII_PASS       1
#define PSK_BY_HEX_CHAR         2

#define DEF_UPD_GROUP_KEY_INTERVAL      3600

void  WpaCfg_Init(WPA_CFG_DATA_T *map);
void WpaCfg_unInit(void);

INT32 Dot1xCfg_ReAuthEnabled(UINT8 *pValue);                 // *pEn -- 1: true, 2: false
INT32 Dot1xCfg_SetReAuthEnabled(UINT8 value);
INT32 Dot1xCfg_ReAuthPeriod(UINT16 *pTime);                  // pTime: in sec
INT32 Dot1xCfg_SetReAuthPeriod(UINT16 pTime);
INT32 Dot1xCfg_RadiusIp(UINT8 Index, UINT8 *pIpAddr);        // Index: 0~3, pIpAddr: pointer to 4 bytes IP address, return 0 if success
INT32 Dot1xCfg_SetRadiusIp(UINT8 Index, UINT8 *pIpAddr);
INT32 Dot1xCfg_RadiusSecret(UINT8 Index, UINT8 *pSecret);    // Index: 0~3, pSecret: pointer to 32 bytes secret
INT32 Dot1xCfg_SetRadiusSecret(UINT8 Index, UINT8 *pSecret);

INT32 Dot1xCfg_BeSupportedSta(UINT8 *pValue);
INT32 Dot1xCfg_SetBeSupportedSta(UINT8 value);
INT32 Dot1xCfg_UnicastCipher(UINT8 *pValue);
INT32 Dot1xCfg_SetUnicastCipher(UINT8 value);
INT32 Dot1xCfg_PSK(UINT8 *PSKArray);
INT32 Dot1xCfg_SetPSK(UINT8 InputType, UINT8 *PSKArray, UINT8 *output);
INT32 Dot1xCfg_UpdGroupKeyInterval(UINT16 *pValue);
INT32 Dot1xCfg_SetUpdGroupKeyInterval(UINT16 value);
void WpaCfg_Init_default(void);
void Dot1xCfg_SetPSK_key(UINT8 *input);

#endif /* _WPA_CFG_H_ */

#if __cplusplus
}
#endif

