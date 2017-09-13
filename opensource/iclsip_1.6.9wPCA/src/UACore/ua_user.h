/* 
 * Copyright (C) 2000-2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * ua_user.h
 *
 * $Id: ua_user.h,v 1.25 2004/09/24 05:46:13 tyhuang Exp $
 */

#ifndef UA_USER_H
#define UA_USER_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <sip/sip_cm.h>
#include <sip/sip.h>
#include "ua_cm.h"

/* Define user information, (username,password) */

CCLAPI UaUser	uaUserNew(void);
CCLAPI RCODE	uaUserFree(IN UaUser);
/* duplicate a user,input source UaUser object */
CCLAPI UaUser	uaUserDup(IN UaUser);

CCLAPI const char* uaUserGetName(IN UaUser);
CCLAPI RCODE	uaUserSetName(IN UaUser, IN const char* ); /*set user name */

CCLAPI const char* uaUserGetDisplayName(IN UaUser);
CCLAPI RCODE	uaUserSetDisplayName(IN UaUser, IN const char* );

CCLAPI const char* uaUserGetLocalAddr(IN UaUser);
CCLAPI RCODE	uaUserSetLocalAddr(IN UaUser,IN const char* );

/*get local port */
CCLAPI int uaUserGetLocalPort(IN UaUser _this);
/*set local port */
CCLAPI RCODE uaUserSetLocalPort(IN UaUser _this, IN const int port);

CCLAPI const char* uaUserGetLocalHost(IN UaUser);
CCLAPI RCODE	uaUserSetLocalHost(IN UaUser,IN const char*);

/*if user not specified the contact address, it will generate from other parameters*/
CCLAPI const char* uaUserGetContactAddr(IN UaUser,IN int);
const char* uaUserGetSIPSContact(IN UaUser);
CCLAPI int uaUserGetNumOfContactAddr(IN UaUser);
CCLAPI RCODE	uaUserSetContactAddr(IN UaUser, IN const char*);
CCLAPI RCODE	uaUserDelContactAddr(IN UaUser, IN const char*);

CCLAPI int	uaUserGetExpires(IN UaUser);
CCLAPI RCODE	uaUserSetExpires(IN UaUser,IN const int /*msec*/);

CCLAPI int	uaUserGetMaxForwards(IN UaUser);
CCLAPI RCODE	uaUserSetMaxForwards(IN UaUser, IN const int);

/* 0:rfc2543(c=0.0.0.0),1:rfc3264(a=sendonly) */
CCLAPI unsigned short uaUserGetHoldVersion(IN UaUser);
CCLAPI RCODE	uaUserSetHoldVersion(IN UaUser, IN unsigned short);

/*add a new UaAuthz Object into user setting */
CCLAPI RCODE	uaUserAddAuthz(IN UaUser,IN UaAuthz);

/*get match Authz object*/
CCLAPI UaAuthz  uaUserGetMatchAuthz(IN UaUser, IN char*);

/* get response parameter of authorizated */
typedef void (*DigestCB)(SipDigestAuthz* author,const char* passwd);

/*Get the digest callback */
CCLAPI DigestCB uaUserGetDigestCB(IN UaUser );

/*Set the digest callback */
CCLAPI RCODE uaUserSetDigestCB(IN UaUser , IN DigestCB);
/*-------------------------------------*/
/*UaAuthz object operation*/
/*input realm,username,password*/
CCLAPI UaAuthz	uaAuthzNew(IN const char*,IN const char*,IN const char*);
CCLAPI void	uaAuthzFree(IN UaAuthz);
CCLAPI UaAuthz  uaAuthzDup(IN UaAuthz);

/*get Authz object related information*/
CCLAPI char*	uaAuthzGetRealm(IN UaAuthz);
CCLAPI char*	uaAuthzGetUserName(IN UaAuthz);
CCLAPI char*	uaAuthzGetPasswd(IN UaAuthz);

/*--------------------------------------
 internal function
----------------------------------------*/
char* uaUserGetAuthData(IN UaUser);
RCODE	uaUserSetAuthData(IN UaUser, IN const char* );

#ifdef  __cplusplus
}
#endif

#endif /* UA_USER_H */
