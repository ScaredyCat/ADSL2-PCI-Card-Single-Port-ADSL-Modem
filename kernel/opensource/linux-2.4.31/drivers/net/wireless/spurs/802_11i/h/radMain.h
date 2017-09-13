#ifndef RADMAIN_H
#define RADMAIN_H
#ifdef WLAN_HOSTOS_LINUX
#include <linux/net.h>
#include <linux/types.h>
#include <net/sock.h>
#endif //WLAN_HOSTOS_LINUX

#include "radDef.h"
#include "radVar.h"
#include "radDS.h"

void RadCli_UnInit(void);

extern INT16 FindRecRadSrv (RAD_SRV_INFO *RecRadSrv, UINT8 SrvType);

int CheckTimeOutEntry(void);

extern void RadCliAuth(UINT16 PortNum, UINT8 *Auth);

extern void RadCliAcc(UINT16 PortNum, UINT8 *Auth);

int sendto_async(
#ifdef WLAN_HOSTOS_LINUX
    struct socket *sock, 
#elif defined(WLAN_HOSTOS_VXWORKS)
    int sock,
#endif //WLAN_HOSTOS_LINUX
    const char *Buffer,const size_t Length, UINT32 flags, struct  sockaddr *addr, int addr_len);
int recvfrom_async(
#ifdef WLAN_HOSTOS_LINUX
    struct socket *sock, 
#elif defined(WLAN_HOSTOS_VXWORKS)
    int sock,
#endif //WLAN_HOSTOS_LINUX
    void * ubuf, size_t size, unsigned flags, struct sockaddr *addr, int *addr_len);

void SetRadSrv (RAD_SRV_INFO *RadSrv);

#endif

