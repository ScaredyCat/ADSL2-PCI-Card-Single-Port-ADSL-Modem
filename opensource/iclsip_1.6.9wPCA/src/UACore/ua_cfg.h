/* 
 * Copyright (C) 2000-2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * ua_cfg.h
 *
 * $Id: ua_cfg.h,v 1.17 2004/09/24 05:45:10 tyhuang Exp $
 */

#ifndef UA_CFG_H
#define UA_CFG_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "ua_cm.h"
#include <sipTx/sipTx.h>

#define CFG_DEFALUT_USINGPROXY		FALSE
#define CFG_DEFALUT_USINGREGISTRAR	FALSE
#define	CFG_DEFAULT_PROXYPORT		5060
#define	CFG_DEFAULT_REGISTRARPORT	5060
#define	CFG_DEFAULT_LISTENPORT		5060

/*typedef struct uaCfgObj* UaCfg;*/

/*create a new uaCfg objcect */
CCLAPI UaCfg uaCfgNew(void);
/*destory a uaCfg object*/
CCLAPI RCODE uaCfgFree(IN UaCfg);
/*duplicate a UaCfg object*/
CCLAPI UaCfg uaCfgDup(IN UaCfg);

/*check using proxy or not */
CCLAPI BOOL  uaCfgUsingProxy(IN UaCfg);		
/*using proxy or not, TRUE using, FALSE not using*/
CCLAPI RCODE uaCfgSetProxy(IN UaCfg,IN BOOL);	

/*get using proxy address, maybe NULL */
CCLAPI char* uaCfgGetProxyAddr(IN UaCfg);	
/*set proxy address */
CCLAPI RCODE uaCfgSetProxyAddr(IN UaCfg,IN const char*);
/*get proxy listen port */
CCLAPI int   uaCfgGetProxyPort(IN UaCfg);	
/*set proxy listen port*/
CCLAPI RCODE uaCfgSetProxyPort(IN UaCfg,IN const int);	
/*set proxy transport type*/
CCLAPI RCODE uaCfgSetProxyTXP(IN UaCfg,IN TXPTYPE);
/*get proxy transport type*/
CCLAPI TXPTYPE uaCfgGetProxyTXP(IN UaCfg);

/*check using Registrar or not */
CCLAPI BOOL  uaCfgUsingRegistrar(IN UaCfg);		
/*using Registrar or not, TRUE using, FALSE not using*/
CCLAPI RCODE uaCfgSetRegistrar(IN UaCfg,IN BOOL);

/*get registrar address */
CCLAPI char* uaCfgGetRegistrarAddr(IN UaCfg);	
/*set registrar address */
CCLAPI RCODE uaCfgSetRegistrarAddr(IN UaCfg,IN const char*);	
/*get registrar port */
CCLAPI int   uaCfgGetRegistrarPort(IN UaCfg);	
/*set registrar port */
CCLAPI RCODE uaCfgSetRegistrarPort(IN UaCfg,IN const int);
/*set registrar transport type*/
CCLAPI RCODE uaCfgSetRegistrarTXP(IN UaCfg,IN TXPTYPE);
/*get registrar transport type*/
CCLAPI TXPTYPE uaCfgGetRegistrarTXP(IN UaCfg);	

/*get listen port */
CCLAPI int   uaCfgGetListenPort(IN UaCfg);	
/*set listen port */
CCLAPI RCODE uaCfgSetListenPort(IN UaCfg, IN const int); 


/* support separated SIMPLE server, add by sam */
/*check using proxy or not */
CCLAPI BOOL  uaCfgUsingSIMPLEProxy(IN UaCfg);		
/*using proxy or not, TRUE using, FALSE not using*/
CCLAPI RCODE uaCfgSetSIMPLEProxy(IN UaCfg,IN BOOL);	

/*get using proxy address, maybe NULL */
CCLAPI char* uaCfgGetSIMPLEProxyAddr(IN UaCfg);	
/*set proxy address */
CCLAPI RCODE uaCfgSetSIMPLEProxyAddr(IN UaCfg,IN const char*);
/*get proxy listen port */
CCLAPI int   uaCfgGetSIMPLEProxyPort(IN UaCfg);	
/*set proxy listen port*/
CCLAPI RCODE uaCfgSetSIMPLEProxyPort(IN UaCfg,IN const int);	
/*set proxy transport type*/
CCLAPI RCODE uaCfgSetSIMPLEProxyTXP(IN UaCfg,IN TXPTYPE);
/*get proxy transport type*/
CCLAPI TXPTYPE uaCfgGetSIMPLEProxyTXP(IN UaCfg);



/*get domain name, equal to SIP Realm 
CCLAPI char* uaCfgGetRealmName(IN UaCfg);	
set domain name, equla to SIP Realm 
CCLAPI RCODE uaCfgSetRealmName(IN UaCfg,IN const char*);*/

#ifdef  __cplusplus
}
#endif

#endif /* UA_CFG_H */
