/* 
 * Copyright (C) 2000-2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/* 
 * ua_user.c
 *
 * $Id: ua_user.c,v 1.43 2004/09/24 05:46:13 tyhuang Exp $
 */

#include "ua_cm.h"
#include "ua_user.h"
#include <common/cm_utl.h>
#include <adt/dx_lst.h>

struct uaUserObj{
	char*		displayname; /* displayname */
	char*		nameid;		/*main user name*/
	char*		laddr;		/*local IP address*/
	int		lport;		/*local port number */
	char*		lhostname;	/*local host name*/
	DxLst		contactLst;	/*list of contact address*/
	int		expires;	/*msec*/
	int		maxforwards;	/*max-forward hub number*/
	DxLst		authzLst;	/*user authorization information*/
	DigestCB	digestFunc;	/*add by tyhuang 2003.9.30 */
	char*		authData;	/*add by tyhuang 2003.9.30 */
	unsigned short	holdstyle;	/*hold of c=0.0.0.0 or sendonly */
};

struct uaAuthzObj{
	char*	domain;		/*specify realm name*/
	char*	username;	/*specify user name for above realm*/
	char*	passwd;		/*specify user password for above realm*/
};

CCLAPI 
UaUser uaUserNew(void)
{
	UaUser _this=NULL;

	_this=(UaUser)calloc(1,sizeof(struct uaUserObj));
	if(_this != NULL){
		_this->displayname=NULL;
		_this->nameid=NULL;
		_this->laddr=NULL;
		_this->lport=5060;
		_this->lhostname=NULL;
		_this->contactLst=NULL;
		_this->expires=EXPIRE_DEFAULT;
		_this->maxforwards=MAX_FORWARDS;
		_this->authzLst=NULL;
		_this->authData=NULL;
		_this->digestFunc=NULL;
		_this->holdstyle=HOLD_VERISON;
	}

	return _this;
}

CCLAPI 
RCODE uaUserFree(IN UaUser _this)
{
	RCODE rCode=RC_OK;
	
	if(_this!=NULL){
		if(_this->displayname){
			free(_this->displayname);
			_this->displayname=NULL;
		}
		if(_this->nameid != NULL){
			free(_this->nameid);
			_this->nameid = NULL;
		}

		if(_this->laddr != NULL){
			free(_this->laddr);
			_this->laddr = NULL;
		}
		if(_this->lhostname != NULL){
			free(_this->lhostname);
			_this->lhostname=NULL;
		}
		if(_this->contactLst){
			TCRPrint(TRACE_LEVEL_API,"*** [uaUserFree] contact list size = %d\n",dxLstGetSize(_this->contactLst));
			dxLstFree(_this->contactLst,(void (*)(void *))free);
			_this->contactLst=NULL;
		}
		if(_this->authzLst != NULL){
			dxLstFree(_this->authzLst,(void (*)(void *))uaAuthzFree);
			_this->authzLst=NULL;
		}
		if(_this->authData != NULL){
			free(_this->authData);
			_this->authData=NULL;
		}
		free(_this);
		_this=NULL;
	}else
		rCode =UAUSER_NULL;

	return rCode;
}


CCLAPI 
UaUser	uaUserDup(IN UaUser src)
{
	UaUser dest=NULL;

	if(src != NULL){ 
		dest=uaUserNew();
		if(dest != NULL){
			dest->displayname=strDup(src->displayname);
			dest->laddr=strDup(src->laddr);
			dest->lport=src->lport;
			dest->expires=src->expires;
			dest->nameid=strDup(src->nameid);
			dest->maxforwards=src->maxforwards;
			dest->contactLst=dxLstDup(src->contactLst,NULL);
			dest->lhostname=strDup(src->lhostname);
			dest->authzLst=dxLstDup(src->authzLst,(void *(*)(void *))uaAuthzDup);
			dest->authData=strDup(src->authData);
			dest->digestFunc=src->digestFunc;
			dest->holdstyle=src->holdstyle;
		}
	}

	return dest;
}

CCLAPI
const char* uaUserGetName(IN UaUser _this)
{
	if(_this!= NULL)
		return _this->nameid;
	else
		return NULL;
}

/*set user name */
CCLAPI 
RCODE uaUserSetName(IN UaUser _this, IN const char* name)
{
	RCODE rCode=RC_OK;
	if(_this!=NULL){
		if(_this->nameid!=NULL){
			free(_this->nameid);
			_this->nameid=NULL;
		}
		_this->nameid=strDup(name);
	}else
		rCode=UAUSER_NULL;
	return rCode;
}


CCLAPI 
const char* uaUserGetDisplayName(IN UaUser _this)
{
	if(_this!=NULL)
		return _this->displayname;
	else
		return NULL;
}

CCLAPI 
RCODE uaUserSetDisplayName(IN UaUser _this, const char* dname)
{
	RCODE rCode=RC_OK;

	if(_this==NULL)
		return UAUSER_NULL;

	if(_this->displayname != NULL)
		free(_this->displayname);
		
	if(dname)
		_this->displayname=strDup(dname);
	else
		_this->displayname=NULL;

	return rCode;
}

CCLAPI 
const char* uaUserGetLocalAddr(IN UaUser _this)
{
	if(_this != NULL)
		return _this->laddr;
	else
		return NULL;
}

CCLAPI 
RCODE uaUserSetLocalAddr(IN UaUser _this,IN const char* laddr)
{
	RCODE rCode=RC_OK;
	if(_this!=NULL){
		if(_this->laddr != NULL){
			free(_this->laddr);
			_this->laddr=NULL;
		}
		_this->laddr=strDup(laddr);
	}else
		rCode=UAUSER_NULL;
	
	return rCode;
}

/*get local port */
CCLAPI 
int uaUserGetLocalPort(IN UaUser _this)
{
	if(_this!=NULL)
		return _this->lport;
	else
		return -1;
}

/*set local port */
CCLAPI 
RCODE uaUserSetLocalPort(IN UaUser _this, IN const int port)
{
	RCODE rCode=RC_OK;

	if(_this!= NULL){
		if(port > 0)
			_this->lport=port;
	}else
		rCode=UAUSER_NULL;

	return rCode;
}

CCLAPI 
const char* uaUserGetLocalHost(IN UaUser _this)
{
	if(_this != NULL){
		return _this->lhostname;
	}else
		return NULL;
}

CCLAPI 
RCODE uaUserSetLocalHost(IN UaUser _this,IN const char* hostname)
{
	RCODE rCode=RC_OK;
	if(_this!=NULL){
		if(_this->lhostname!=NULL){
			free(_this->lhostname);
			_this->lhostname=NULL;
		}
		_this->lhostname=strDup(hostname);
	}else
		rCode=UAUSER_HOST_NULL;

	return rCode;
}

/*if user not specified the contact address, it will generate from other parameters*/
CCLAPI 
const char* uaUserGetContactAddr(IN UaUser _this,IN int _pos)
{
	if(_this)
		if(_this->contactLst)
			return dxLstPeek(_this->contactLst,_pos);
	return NULL;
}
 
const char* uaUserGetSIPSContact(IN UaUser _this)
{
	int pos,size;
	DxLst clist;
	char *semi,*contacturl,*target=NULL,*freeptr;

	if(!_this)
		return NULL;
	
	clist=_this->contactLst;
	if(!clist)
		return NULL;

	size=dxLstGetSize(clist);
	for(pos=0;pos<size;pos++){
		contacturl=strDup(dxLstPeek(clist,pos));
		freeptr=contacturl;
		semi=strchr(contacturl,':');
		if(semi)
			*semi='\0';
		semi=strchr(contacturl,'<');
		if(semi)
			contacturl=semi+1;
		if(strcmp(contacturl,"sips")==0){
			free(freeptr);
			target=dxLstPeek(clist,pos);
			break;
		}
		free(freeptr);
	}

	return target;
}

CCLAPI 
int uaUserGetNumOfContactAddr(IN UaUser _this)
{
	if(_this)
		if(_this->contactLst)
			return dxLstGetSize(_this->contactLst);
	
	return 0;
}

CCLAPI 
RCODE uaUserSetContactAddr(IN UaUser _this, IN const char* _addr)
{
	RCODE rCode=RC_OK;
	if(_this != NULL){
		if(_addr!=NULL){
			/*if(_this->contactaddr !=NULL){
				free(_this->contactaddr);
				_this->contactaddr=NULL;
			}
			_this->contactaddr=strDup(_addr);*/
			if(_this->contactLst==NULL)
				_this->contactLst=dxLstNew(DX_LST_STRING);
			if(_this->contactLst)
				dxLstPutTail(_this->contactLst,(char*)_addr);
		}else
			rCode=UAUSER_CONTACT_NULL;

	}else
		rCode=UAUSER_NULL;

	return rCode;
}

CCLAPI 
RCODE	uaUserDelContactAddr(IN UaUser _this,IN const char* _addr)
{
	RCODE rCode=RC_OK;
	int pos,size;
	char *contactaddr=NULL;

	if(_this != NULL){
		if(_this->contactLst && _addr){
			size=dxLstGetSize(_this->contactLst);
			for(pos=0;pos<size;pos++){
				contactaddr=dxLstPeek(_this->contactLst,pos);
				if(strICmp(_addr,contactaddr)==0){
					contactaddr=dxLstGetAt(_this->contactLst,pos);
					if(contactaddr){
						free(contactaddr);
						contactaddr=NULL;
					}
					break;
				}
			}
		}
	}else
		rCode=UAUSER_NULL;

	return rCode;
}


CCLAPI 
int uaUserGetExpires(IN UaUser _this)
{
	if(_this!=NULL)
		return _this->expires;
	else
		return (int)EXPIRE_DEFAULT;
}

CCLAPI 
RCODE uaUserSetExpires(IN UaUser _this,IN const int msec)
{
	RCODE rCode=RC_OK;

	if(_this !=NULL){
		_this->expires=msec;
	}else
		rCode=UAUSER_NULL;

	return rCode;
}

CCLAPI 
int uaUserGetMaxForwards(IN UaUser _this)
{
	if(_this != NULL){
		return _this->maxforwards;
	}else
		return -1;
}

CCLAPI 
RCODE uaUserSetMaxForwards(IN UaUser _this, IN const int _hub)
{
	RCODE rCode=RC_OK;

	if(_hub <=0)
		return UAUSER_MAXFORWARD_TOO_SMALL;
	
	if(_this !=NULL){
		_this->maxforwards=_hub;
	}else
		rCode=UAUSER_NULL;

	return rCode;
}

CCLAPI 
unsigned short	uaUserGetHoldVersion(IN UaUser _this)
{
	if(_this == NULL)
		return HOLD_VERISON;
	else
		return _this->holdstyle;
}
CCLAPI 
RCODE uaUserSetHoldVersion(IN UaUser _this, IN unsigned short holdversion)
{
	if(_this == NULL)
		return UAUSER_NULL;
	if(holdversion >=1)
		_this->holdstyle=1;
	else
		_this->holdstyle=0;

	return RC_OK;
}


/*add a new UaAuthz Object into user setting */
CCLAPI 
RCODE uaUserAddAuthz(IN UaUser _user,IN UaAuthz _authz)
{
	RCODE rCode=RC_OK;

	if(_user == NULL)
		return UAUSER_NULL;
	if(_authz == NULL)
		return UAUSER_AUTHZ_NULL;

	/*authzLst is NULL, never using it */
	if(_user->authzLst==NULL){
		_user->authzLst=dxLstNew(DX_LST_POINTER);
	}
	dxLstPutTail(_user->authzLst,_authz);

	return rCode;
}

/*get match Authz object*/
CCLAPI 
UaAuthz  uaUserGetMatchAuthz(IN UaUser _this, IN char* _domain)
{
	UaAuthz authz=NULL;
	int num=0,idx=0;
	if((_this == NULL) || (_domain == NULL)){
		UaCoreERR("[uaUserGetMatchAuthz] user or domain is NULL!\n");
		return NULL;
	}
	num=dxLstGetSize(_this->authzLst);
	for(idx=0;idx<num;idx++){
		authz=dxLstPeek(_this->authzLst,idx);
		if(strICmp(authz->domain,_domain)==0)
			break;
		authz=NULL;
	}

	return authz;
}

/*Get the digest callback */
CCLAPI
DigestCB uaUserGetDigestCB(IN UaUser _this)
{
	if(_this)
		return _this->digestFunc;
	return NULL;
}		

/*Set the digest callback */
CCLAPI 
RCODE uaUserSetDigestCB(IN UaUser _this, IN DigestCB _cb)
{
	RCODE rCode=RC_ERROR;
	if(_this){
		_this->digestFunc=_cb;
		rCode=RC_OK;
	}else
		UaCoreERR("[uaUserSetDigestCB] UaUser is NULL!\n");
	return rCode;
}
/****************************************************************************/
/*input realm,username,password*/
CCLAPI 
UaAuthz	uaAuthzNew(IN const char* _realm,IN const char* _user,IN const char* _passwd)
{
	UaAuthz authz=NULL;

	if((_realm == NULL) || (_user ==NULL)||(_passwd==NULL)){
		UaCoreERR("[uaAuthzNew] realm,userid or password is NULL!\n");
		return NULL;
	}
	authz=(UaAuthz)calloc(1,sizeof(struct uaAuthzObj));
	authz->domain=strDup(_realm);
	authz->username=strDup(_user);
	authz->passwd=strDup(_passwd);

	return authz;
}

CCLAPI 
void uaAuthzFree(IN UaAuthz _this)
{
	if(_this != NULL){
		if(_this->domain != NULL){
			free(_this->domain);
			_this->domain=NULL;
		}
		if(_this->passwd != NULL){
			free(_this->passwd);
			_this->passwd=NULL;
		}
		if(_this->username!=NULL){
			free(_this->username);
			_this->username=NULL;
		}
		free(_this);
		_this=NULL;
	}
}

CCLAPI 
UaAuthz uaAuthzDup(IN UaAuthz _src)
{
	UaAuthz dest=NULL;

	if(_src!=NULL){
		dest=uaAuthzNew(_src->domain,_src->username,_src->passwd);
	}
	
	return dest;
}


/*get Authz object related information*/
CCLAPI 
char* uaAuthzGetRealm(IN UaAuthz _this)
{
	if(_this != NULL)
		return _this->domain;
	else
		return NULL;
}

CCLAPI 
char* uaAuthzGetUserName(IN UaAuthz _this)
{
	if(_this != NULL)
		return _this->username;
	else
		return NULL;
}

CCLAPI 
char* uaAuthzGetPasswd(IN UaAuthz _this)
{
	if(_this!=NULL)
		return _this->passwd;
	else
		return NULL;
}


char* uaUserGetAuthData(IN UaUser _this)
{
	if(_this)
		return _this->authData;
	else
		return NULL;
}

RCODE	uaUserSetAuthData(IN UaUser _this, IN const char *authd)
{
	if(_this == NULL){
		UaCoreERR("[uaUserSetAuthData] user object is NULL!\n");
		return RC_ERROR;
	}
	/*if(authd == NULL){
		UaCoreERR("[uaUserSetAuthData] input authdata is NULL!\n");
		return RC_ERROR;
	}*/

	if(_this->authData){
		free(_this->authData);
		_this->authData=NULL;
	}
	if(authd)
		_this->authData=strDup(authd);
	return RC_OK;

}