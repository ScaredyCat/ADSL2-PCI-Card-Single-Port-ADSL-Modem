/* 
 * Copyright (C) 2000-2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 *
 * 
 * ua_cfg.c
 *
 * $Id: ua_cfg.c,v 1.14 2003/11/10 06:01:20 tyhuang Exp $
 */
#include "ua_cfg.h"
#include "ua_cm.h"

struct uaCfgObj{
	BOOL	bproxy;		/* using proxy or not */
	char*	proxy;		/* proxy name/ip */
	int	proxyport;	/* proxy port, default 5060 */
	TXPTYPE proxytxp;	/* proxy transport type */
	BOOL	bregistrar;	/* using registrar or not. add by tyhuang */
	char*	registrar;	/* registrar server */
	int	registrarport;	/* registrar's port */
	TXPTYPE	registrartxp;	/* registrar transport type */
	int	listenport;	/* listen port number */
	/* char*	realm;		 Realm, domain name. modified by tyhuang*/

	/* support separated SIMPLE server, add by sam */
	BOOL	bUseDifferenceProxyForSIMPLE;	
	char*	SIMPLEProxy;
	int		SIMPLEProxyPort;
	TXPTYPE	SIMPLEProxyTxp;
};

/*create a new uaCfg objcect */
CCLAPI
UaCfg uaCfgNew(void)
{
	UaCfg _this;

	_this=(UaCfg)calloc(1,sizeof(struct uaCfgObj));
	if(_this != NULL){
		_this->bproxy=CFG_DEFALUT_USINGPROXY;
		_this->proxy=NULL;
		_this->proxyport=CFG_DEFAULT_PROXYPORT;
		_this->proxytxp=UNKNOWN_TXP;
		_this->bregistrar=CFG_DEFALUT_USINGREGISTRAR;
		_this->registrar=NULL;
		_this->registrarport=CFG_DEFAULT_REGISTRARPORT;
		_this->registrartxp=UNKNOWN_TXP;
		_this->listenport=CFG_DEFAULT_LISTENPORT;

		/* support separated SIMPLE server, add by sam */
		_this->bUseDifferenceProxyForSIMPLE = FALSE;
		_this->SIMPLEProxy = NULL;
		_this->SIMPLEProxyPort = CFG_DEFAULT_PROXYPORT;
		_this->SIMPLEProxyTxp = UNKNOWN_TXP;

		return _this;
	}else
		return NULL;
}

/*destory a uaCfg object*/
CCLAPI
RCODE uaCfgFree(IN UaCfg _this)
{
	RCODE rCode = RC_OK;

	if(_this != NULL){
		_this->bproxy=CFG_DEFALUT_USINGPROXY;
		if(_this->proxy != NULL){
			free(_this->proxy);
			_this->proxy=NULL;
		}
		_this->proxyport=CFG_DEFAULT_PROXYPORT;
		_this->bregistrar=CFG_DEFALUT_USINGREGISTRAR;
		if(_this->registrar != NULL){
			free(_this->registrar);
			_this->registrar= NULL;
		}
		_this->proxytxp=UNKNOWN_TXP;
		_this->registrartxp=UNKNOWN_TXP;
		_this->registrarport=CFG_DEFAULT_REGISTRARPORT;
		_this->listenport=CFG_DEFAULT_LISTENPORT;

		/* support separated SIMPLE server, add by sam */
		_this->bUseDifferenceProxyForSIMPLE = FALSE;
		if ( _this->SIMPLEProxy != NULL)
		{
			free(_this->SIMPLEProxy);
			_this->SIMPLEProxy = NULL;
		}
		_this->SIMPLEProxyPort = CFG_DEFAULT_PROXYPORT;
		_this->SIMPLEProxyTxp = UNKNOWN_TXP;

		free(_this);
	}else
		rCode = UACFG_NULL;

	return rCode;
}

/*duplicate a UaCfg object*/
CCLAPI 
UaCfg uaCfgDup(IN UaCfg _src)
{
	UaCfg dest=NULL;

	if(_src != NULL){
		dest=uaCfgNew();
		dest->bproxy=_src->bproxy;
		dest->listenport=_src->listenport;
		dest->proxy=strDup(_src->proxy);
		dest->proxyport=_src->proxyport;
		dest->proxytxp=_src->proxytxp;
		dest->bregistrar=_src->bregistrar;
		dest->registrar=strDup(_src->registrar);
		dest->registrarport=_src->registrarport;
		dest->registrartxp=_src->registrartxp;

		/* support separated SIMPLE server, add by sam */
		dest->bUseDifferenceProxyForSIMPLE = _src->bUseDifferenceProxyForSIMPLE;
		dest->SIMPLEProxy = strDup(_src->SIMPLEProxy);
		dest->SIMPLEProxyPort = _src->SIMPLEProxyPort;
		dest->SIMPLEProxyTxp = _src->SIMPLEProxyTxp;
	}

	return dest;
}


/*check using proxy or not */
CCLAPI 
BOOL  uaCfgUsingProxy(IN UaCfg _this)
{
	if(_this != NULL)
		return _this->bproxy;
	else
		return CFG_DEFALUT_USINGPROXY;
}

/*using proxy or not, TRUE using, FALSE not using*/
CCLAPI 
RCODE uaCfgSetProxy(IN UaCfg _this, IN BOOL bUsing)
{
	RCODE rCode = RC_OK;
	if(_this != NULL)
		_this->bproxy=bUsing;
	else
		rCode =UACFG_NULL;

	return rCode;
}

/*get using proxy address, maybe NULL */
CCLAPI 
char* uaCfgGetProxyAddr(IN UaCfg _this)
{
	if(_this != NULL)
		return _this->proxy;
	else
		return NULL;
}
	
/*set proxy address */
CCLAPI 
RCODE uaCfgSetProxyAddr(IN UaCfg _this, IN const char* proxyaddr)
{
	RCODE rCode=RC_OK;

	if(_this!= NULL){
		if(_this->proxy != NULL){
			free(_this->proxy);
			_this->proxy=NULL;
		}
		_this->proxy=strDup(proxyaddr);
	}else
		rCode=UACFG_NULL;
	
	return rCode;
}

/*get proxy listen port */
CCLAPI 
int uaCfgGetProxyPort(IN UaCfg _this)
{
	if(_this != NULL)
		return _this->proxyport;
	else
		return -1;

}
/*set proxy listen port*/
CCLAPI 
RCODE uaCfgSetProxyPort(IN UaCfg _this,IN const int port)
{
	RCODE rCode=RC_OK;
	if(_this != NULL){
		_this->proxyport=port;
	}else
		rCode=UACFG_NULL;

	return rCode;
}

/*set proxy transport type*/
CCLAPI 
RCODE uaCfgSetProxyTXP(IN UaCfg _this,IN TXPTYPE txtype)
{
	RCODE rCode=RC_OK;
	if(_this == NULL){
		return UACFG_NULL;
	}else
		_this->proxytxp=txtype;

	return rCode;
}

/*get proxy transport type*/
CCLAPI TXPTYPE uaCfgGetProxyTXP(IN UaCfg _this)
{
	TXPTYPE txp=UNKNOWN_TXP;

	if(_this != NULL){
		txp=_this->proxytxp;
	}

	return txp;
}

/*check using registrar or not */
/* add by tyhuang */
CCLAPI 
BOOL  uaCfgUsingRegistrar(IN UaCfg _this)
{
	if(_this != NULL)
		return _this->bregistrar;
	else
		return CFG_DEFALUT_USINGREGISTRAR;
}

/*set using registrar or not, TRUE using, FALSE not using*/
/* add by tyhuang */
CCLAPI 
RCODE uaCfgSetRegistrar(IN UaCfg _this, IN BOOL bUsing)
{
	RCODE rCode = RC_OK;
	if(_this != NULL)
		_this->bregistrar=bUsing;
	else
		rCode =UACFG_NULL;

	return rCode;
}

/*get registrar address */
CCLAPI 
char* uaCfgGetRegistrarAddr(IN UaCfg _this)
{
	if(_this != NULL)
		return _this->registrar;
	else
		return NULL;
}

/*set registrar address */
CCLAPI 
RCODE uaCfgSetRegistrarAddr(IN UaCfg _this,IN const char* addr)
{
	RCODE rCode=RC_OK;

	if(_this != NULL){
		if(_this->registrar != NULL){
			free(_this->registrar);
			_this->registrar = NULL;
		}
		_this->registrar=strDup(addr);
	}else
		rCode=UACFG_NULL;

	return rCode;

}

/*get registrar port */
CCLAPI 
int uaCfgGetRegistrarPort(IN UaCfg _this)	
{
	if(_this != NULL)
		return _this->registrarport;
	else
		return -1;
}

/*set registrar port */
CCLAPI 
RCODE uaCfgSetRegistrarPort(IN UaCfg _this,IN const int port)
{
	RCODE rCode=RC_OK;

	if(_this != NULL){
		_this->registrarport=port;
	}else
		rCode=UACFG_NULL;

	return rCode;
}

/*set registrar transport type*/
/* add by tyhuang */
CCLAPI 
RCODE uaCfgSetRegistrarTXP(IN UaCfg _this,IN TXPTYPE txtype)
{
	RCODE rCode=RC_OK;
	if(_this == NULL){
		return UACFG_NULL;
	}else
		_this->registrartxp=txtype;

	return rCode;
}

/*get registrar transport type*/
/* add by tyhuang */
CCLAPI TXPTYPE uaCfgGetRegistrarTXP(IN UaCfg _this)
{
	TXPTYPE txp=UNKNOWN_TXP;

	if(_this != NULL){
		txp=_this->registrartxp;
	}

	return txp;
}

/*get listen port */
CCLAPI 
int uaCfgGetListenPort(IN UaCfg _this)
{
	if(_this!=NULL)
		return _this->listenport;
	else
		return -1;
}

/*set listen port */
CCLAPI 
RCODE uaCfgSetListenPort(IN UaCfg _this, IN const int port)
{
	RCODE rCode=RC_OK;

	if(_this!= NULL){
		_this->listenport=port;
	}else
		rCode=UACFG_NULL;

	return rCode;
}

/*modified by tyhuang-get domain name, equal to SIP Realm 
CCLAPI 
char* uaCfgGetRealmName(IN UaCfg _this)
{
	if(_this != NULL)
		return _this->realm;
	else
		return NULL;
}

set domain name, equla to SIP Realm 
CCLAPI 
RCODE uaCfgSetRealmName(IN UaCfg _this,IN const char* domain)
{
	RCODE rCode=RC_OK;

	if(_this != NULL){
		if(_this->realm != NULL){
			free(_this->realm);
			_this->realm=NULL;
		}
		_this->realm=strDup(domain);
	}else
		rCode=UACFG_NULL;

	return rCode;
}*/


/*check using proxy or not */
CCLAPI 
BOOL  uaCfgUsingSIMPLEProxy(IN UaCfg _this)
{
	if(_this != NULL)
		return _this->bUseDifferenceProxyForSIMPLE;
	else
		return FALSE;
}

/*using proxy or not, TRUE using, FALSE not using*/
CCLAPI 
RCODE uaCfgSetSIMPLEProxy(IN UaCfg _this, IN BOOL bUsing)
{
	RCODE rCode = RC_OK;
	if(_this != NULL)
		_this->bUseDifferenceProxyForSIMPLE=bUsing;
	else
		rCode =UACFG_NULL;

	return rCode;
}

/*get using proxy address, maybe NULL */
CCLAPI 
char* uaCfgGetSIMPLEProxyAddr(IN UaCfg _this)
{
	if(_this != NULL)
		return _this->SIMPLEProxy;
	else
		return NULL;
}
	
/*set proxy address */
CCLAPI 
RCODE uaCfgSetSIMPLEProxyAddr(IN UaCfg _this, IN const char* proxyaddr)
{
	RCODE rCode=RC_OK;

	if(_this!= NULL){
		if(_this->proxy != NULL){
			free(_this->SIMPLEProxy);
			_this->SIMPLEProxy=NULL;
		}
		_this->SIMPLEProxy=strDup(proxyaddr);
	}else
		rCode=UACFG_NULL;
	
	return rCode;
}

/*get proxy listen port */
CCLAPI 
int uaCfgGetSIMPLEProxyPort(IN UaCfg _this)
{
	if(_this != NULL)
		return _this->SIMPLEProxyPort;
	else
		return -1;

}
/*set proxy listen port*/
CCLAPI 
RCODE uaCfgSetSIMPLEProxyPort(IN UaCfg _this,IN const int port)
{
	RCODE rCode=RC_OK;
	if(_this != NULL){
		_this->SIMPLEProxyPort=port;
	}else
		rCode=UACFG_NULL;

	return rCode;
}

/*set proxy transport type*/
CCLAPI 
RCODE uaCfgSetSIMPLEProxyTXP(IN UaCfg _this,IN TXPTYPE txtype)
{
	RCODE rCode=RC_OK;
	if(_this == NULL){
		return UACFG_NULL;
	}else
		_this->SIMPLEProxyTxp=txtype;

	return rCode;
}

/*get proxy transport type*/
CCLAPI TXPTYPE uaCfgGetSIMPLEProxyTXP(IN UaCfg _this)
{
	TXPTYPE txp=UNKNOWN_TXP;

	if(_this != NULL){
		txp=_this->SIMPLEProxyTxp;
	}

	return txp;
}
