#ifndef __DN_BSOP_H_
#define __DN_BSOP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "adt/dx_lst.h"

typedef struct BSEntityObj BSEntity;
typedef BSEntity* pBSEntity;

CCLAPI 
RCODE InitDualNet();

CCLAPI 
RCODE GetBSList(OUT DxLst BSLst);

CCLAPI
RCODE AssociateBS(IN char* _ssid, IN int _ssidLenth);

CCLAPI
RCODE GetAssociatedBSID(OUT char* BSID);

CCLAPI
RCODE Disassociate();

CCLAPI 
RCODE UnInitDualNet();

CCLAPI
void SortBS(IN OUT DxLst BSLst);

/*pMAC must be char[6]*/
CCLAPI
RCODE GetMAC(IN pBSEntity entity, OUT unsigned char* pMAC);

CCLAPI
RCODE GetSSID(IN pBSEntity entity, OUT char* pSSID, IN OUT int* pI);

CCLAPI
long GetRSSI(IN pBSEntity entity);

CCLAPI
void FreeBsEntity(IN OUT pBSEntity pEntity);

#ifdef __cplusplus
}
#endif

#endif /*__DN_BSOP_H_*/