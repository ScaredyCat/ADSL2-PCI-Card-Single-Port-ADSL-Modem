/***********************************************************************/
/*                                                                     */
/*   Program:    rt_linux.c                                            */
/*   Created:    4/21/2006 1:17:38 PM                                  */
/*   Author:     Wu Xi-Kun                                             */
/*   Comments:   `description`                                         */
/*                                                                     */
/*---------------------------------------------------------------------*/
/*                                                                     */
/* History:                                                            */
/*    Revision 1.1 4/21/2006 1:17:38 PM  xsikun                        */
/*    Initial revision                                                 */
/*                                                                     */
/***********************************************************************/

#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>

#include <linux/spinlock.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/pci.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/ethtool.h>
#include <linux/wireless.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/if_arp.h>
#include <linux/ctype.h>
#include <linux/vmalloc.h>


#include <linux/wireless.h>
#include <net/iw_handler.h>

// load firmware
#define __KERNEL_SYSCALLS__
#include <linux/unistd.h>
#include <asm/uaccess.h>


#define MEM_ALLOC_FLAG      (GFP_ATOMIC) //(GFP_DMA | GFP_ATOMIC)

#ifndef IFNAMSIZ
#define IFNAMSIZ 16
#endif

//#define CONFIG_CKIP_SUPPORT

#undef __inline
#define __inline	   static inline 

#define CHAR            signed char
#define INT             signed int
#define SHORT           signed short
#define UINT            unsigned int 
#undef  ULONG           
//#define ULONG           unsigned int
#define ULONG           unsigned long /* 32-bit in 32-bit CPU or
										 64-bit in 64-bit CPU */
#define USHORT          unsigned short
#define UCHAR           unsigned char

typedef int					INT32;

typedef int (*HARD_START_XMIT_FUNC)(struct sk_buff *skb, struct net_device *net_dev);

// add by kathy
#ifdef CONFIG_AP_SUPPORT
#ifdef RT2860
//HANK2007/11/20 04:05�U��
#define PROFILE_PATH			"/flash/RT2860AP.dat"
#define RTMP_FIRMWARE_FILE_NAME "/etc/Wireless/RT2860AP/RT2860AP.bin"
#define NIC_DEVICE_NAME			"RT2860AP"
#define DRIVER_VERSION			"1.7.0.0"
#define CARD_INFO_PATH			"/etc/Wireless/RT2860AP/RT2860APCard.dat"
#endif // RT2860 //
#endif // CONFIG_AP_SUPPORT //


#ifdef RT2860
#ifndef PCI_DEVICE
#define PCI_DEVICE(vend,dev) \
	.vendor = (vend), .device = (dev), \
	.subvendor = PCI_ANY_ID, .subdevice = PCI_ANY_ID
#endif // PCI_DEVICE //
#endif // RT2860 //

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)

#define RTMP_TIME_AFTER(a,b)		\
	(typecheck(unsigned long, (unsigned long)a) && \
	 typecheck(unsigned long, (unsigned long)b) && \
	 ((long)(b) - (long)(a) < 0))

#define RTMP_TIME_AFTER_EQ(a,b)	\
	(typecheck(unsigned long, (unsigned long)a) && \
	 typecheck(unsigned long, (unsigned long)b) && \
	 ((long)(a) - (long)(b) >= 0))
#define RTMP_TIME_BEFORE(a,b)	RTMP_TIME_AFTER_EQ(b,a)
#else
#define RTMP_TIME_AFTER(a,b) time_after(a, b)
#endif

#define BOOLEAN         unsigned char
//#define LARGE_INTEGER s64
#define VOID            void
//#define LONG            int
#define LONG 			long
#define LONGLONG        s64
#define ULONGLONG       u64
typedef VOID            *PVOID;
typedef CHAR            *PCHAR;
typedef UCHAR           *PUCHAR;
typedef USHORT          *PUSHORT;
typedef LONG            *PLONG;
typedef ULONG           *PULONG;
typedef UINT            *PUINT;

#define OS_HZ			HZ

typedef union _LARGE_INTEGER {
    struct {
        UINT LowPart;
        INT32 HighPart;
    } ;
    struct {
        UINT LowPart;
        INT32 HighPart;
    } u;
    s64 QuadPart;
} LARGE_INTEGER;


#define ETH_LENGTH_OF_ADDRESS	6

#define IN
#define OUT

#define TRUE        1
#define FALSE       0

#define NDIS_STATUS                             INT
#define NDIS_STATUS_SUCCESS                     0x00
#define NDIS_STATUS_FAILURE                     0x01
#define NDIS_STATUS_INVALID_DATA				0x02
#define NDIS_STATUS_RESOURCES                   0x03

#define MIN_NET_DEVICE_FOR_AID			0x00		//0x00~0x3f
#define MIN_NET_DEVICE_FOR_MBSSID		0x00		//0x00,0x10,0x20,0x30
#define MIN_NET_DEVICE_FOR_WDS			0x10		//0x40,0x50,0x60,0x70
#define MIN_NET_DEVICE_FOR_APCLI		0x20


struct os_lock  {
	spinlock_t		lock;
	unsigned long  	flags;
};

#ifdef CONFIG_AP_SUPPORT
#ifdef IAPP_SUPPORT
typedef struct _RT_SIGNAL_STRUC {
	USHORT					Sequence;
    UCHAR					MacAddr[ETH_LENGTH_OF_ADDRESS];
    UCHAR					CurrAPAddr[ETH_LENGTH_OF_ADDRESS];
    UCHAR					Sig;
} RT_SIGNAL_STRUC, *PRT_SIGNAL_STRUC;

// definition of signal
#define	SIG_NONE					0
#define SIG_ASSOCIATION				1
#define SIG_REASSOCIATION			2
#endif // IAPP_SUPPORT //
#endif // CONFIG_AP_SUPPORT //

struct os_cookie {
#ifdef RT2860
	struct pci_dev 			*pci_dev;
	dma_addr_t		  		pAd_pa;
#endif // RT2860 //


	struct tasklet_struct 	rx_done_task;
	struct tasklet_struct 	mgmt_dma_done_task;
	struct tasklet_struct 	ac0_dma_done_task;
	struct tasklet_struct 	ac1_dma_done_task;
	struct tasklet_struct 	ac2_dma_done_task;
	struct tasklet_struct 	ac3_dma_done_task;
	struct tasklet_struct 	hcca_dma_done_task;
	struct tasklet_struct	tbtt_task;
#ifdef RT2860
	struct tasklet_struct	fifo_statistic_full_task;
#endif // RT2860 //

#ifdef CONFIG_AP_SUPPORT
#ifdef DFS_SUPPORT
	struct tasklet_struct	pulse_radar_detect_task;
	struct tasklet_struct	width_radar_detect_task;
#endif // DFS_SUPPORT //
#ifdef CARRIER_DETECTION_SUPPORT
	struct tasklet_struct	carrier_sense_task;
#endif // CARRIER_DETECTION_SUPPORT //
#endif // CONFIG_AP_SUPPORT //

	unsigned long			apd_pid; //802.1x daemon pid
#ifdef CONFIG_AP_SUPPORT
#ifdef IAPP_SUPPORT
	RT_SIGNAL_STRUC			RTSignal;
	unsigned long			IappPid; //IAPP daemon pid
#endif // IAPP_SUPPORT //
#endif // CONFIG_AP_SUPPORT //
	INT						ioctl_if_type;
	INT 					ioctl_if;
};	

typedef struct _VIRTUAL_ADAPTER
{
	struct net_device		*RtmpDev;
	struct net_device		*VirtualDev;
} VIRTUAL_ADAPTER, PVIRTUAL_ADAPTER;

#undef  ASSERT
#define ASSERT(x)                                                               \
{                                                                               \
    if (!(x))                                                                   \
    {                                                                           \
        printk(KERN_WARNING __FILE__ ":%d assert " #x "failed\n", __LINE__);    \
    }                                                                           \
}
	
typedef struct os_cookie	* POS_COOKIE;	
typedef struct pci_dev 		* PPCI_DEV;	
typedef struct net_device	* PNET_DEV;
typedef void				* PNDIS_PACKET;
typedef char				NDIS_PACKET;
typedef PNDIS_PACKET		* PPNDIS_PACKET;
typedef	dma_addr_t			NDIS_PHYSICAL_ADDRESS;
typedef	dma_addr_t			* PNDIS_PHYSICAL_ADDRESS;
//typedef struct timer_list	RALINK_TIMER_STRUCT;
//typedef struct timer_list	* PRALINK_TIMER_STRUCT;
//typedef struct os_lock		NDIS_SPIN_LOCK;
typedef spinlock_t			NDIS_SPIN_LOCK;
typedef struct timer_list	NDIS_MINIPORT_TIMER;
typedef void				* NDIS_HANDLE;
typedef char 				* PNDIS_BUFFER;



void hex_dump(char *str, unsigned char *pSrcBufVA, unsigned int SrcBufLen);

dma_addr_t linux_pci_map_single(void *handle, void *ptr, size_t size, int direction);
void linux_pci_unmap_single(void *handle, dma_addr_t dma_addr, size_t size, int direction);


////////////////////////////////////////
// MOVE TO rtmp.h ?
/////////////////////////////////////////
#define PKTSRC_NDIS             0x7f
#define PKTSRC_DRIVER           0x0f
#define PRINT_MAC(addr)	\
	addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]


#define RT2860_PCI_DEVICE_ID		0x0601
#if 0
/* This defines the direction arg to the DMA mapping routines. */
#define PCI_DMA_BIDIRECTIONAL	0
#define PCI_DMA_TODEVICE	1
#define PCI_DMA_FROMDEVICE	2
#define PCI_DMA_NONE		3
#endif

#ifdef RT2860
#define PCI_MAP_SINGLE(_handle, _ptr, _size, _dir) \
	linux_pci_map_single(_handle, _ptr, _size, _dir)

#define PCI_UNMAP_SINGLE(_handle, _ptr, _size, _dir) \
	linux_pci_unmap_single(_handle, _ptr, _size, _dir)
#endif // RT2860 //



#define BEACON_FRAME_DMA_CACHE_WBACK(_ptr, _size)	\
	dma_cache_wback(_ptr, _size)


//////////////////////////////////////////
//
//////////////////////////////////////////


#define NdisMIndicateStatus(_w, _x, _y, _z)
#define NdisMediaStateConnected		1
#define NdisMediaStateDisconnected	0
typedef unsigned int	NDIS_MEDIA_STATE;


#ifdef WIN_NDIS
typedef	NDIS_MINIPORT_TIMER	RTMP_OS_TIMER;
#else
typedef struct timer_list	RTMP_OS_TIMER;
#endif
	


typedef struct  _RALINK_TIMER_STRUCT    {
    RTMP_OS_TIMER		TimerObj;       // Ndis Timer object
	BOOLEAN				Valid;			// Set to True when call RTMPInitTimer
    BOOLEAN             State;          // True if timer cancelled
    BOOLEAN             Repeat;         // True if periodic timer
    ULONG               TimerValue;     // Timer value in milliseconds
	ULONG				cookie;			// os specific object
}   RALINK_TIMER_STRUCT, *PRALINK_TIMER_STRUCT;




//#define DBG	1

//
//  MACRO for debugging information
//

#ifdef DBG
extern ULONG    RTDebugLevel;

#define DBGPRINT_RAW(Level, Fmt)    \
{                                   \
    if (Level <= RTDebugLevel)      \
    {                               \
        printk Fmt;               \
    }                               \
}

#define DBGPRINT(Level, Fmt)    DBGPRINT_RAW(Level, Fmt)


#define DBGPRINT_ERR(Fmt)           \
{                                   \
    printk("ERROR!!! ");          \
    printk Fmt;                  \
}

#define DBGPRINT_S(Status, Fmt)		\
{									\
	printk Fmt;					\
}
			

#else
#define DBGPRINT(Level, Fmt)
#define DBGPRINT_RAW(Level, Fmt)
#define DBGPRINT_S(Status, Fmt)
#define DBGPRINT_ERR(Fmt)
#endif


//
//  spin_lock enhanced for Nested spin lock
//
#define NdisAllocateSpinLock(__lock)      \
{                                       \
    spin_lock_init((spinlock_t *)(__lock));               \
}

#define NdisFreeSpinLock(lock)          \
{                                       \
}


#define RTMP_SEM_LOCK(__lock)					\
{												\
	spin_lock_bh((spinlock_t *)(__lock));				\
}

#define RTMP_SEM_UNLOCK(__lock)					\
{												\
	spin_unlock_bh((spinlock_t *)(__lock));				\
}

#if 0 // sample, IRQ LOCK
#define RTMP_IRQ_LOCK(__lock, __irqflags)					\
{													\
	spin_lock_irqsave((spinlock_t *)__lock, __irqflags);	\
	pAd->irq_disabled |= 1; \
}

#define RTMP_IRQ_UNLOCK(__lock, __irqflag)						\
{														\
	pAd->irq_disabled &= 0; \
	spin_unlock_irqrestore((spinlock_t *)(__lock), ((unsigned long)__irqflag));	\
}
#else

// sample, use semaphore lock to replace IRQ lock, 2007/11/15
#define RTMP_IRQ_LOCK(__lock, __irqflags)			\
{													\
	__irqflags = 0;									\
	spin_lock_bh((spinlock_t *)(__lock));			\
	pAd->irq_disabled |= 1; \
}

#define RTMP_IRQ_UNLOCK(__lock, __irqflag)			\
{													\
	pAd->irq_disabled &= 0; \
	spin_unlock_bh((spinlock_t *)(__lock));			\
}

#define RTMP_INT_LOCK(__lock, __irqflags)			\
{													\
	spin_lock_irqsave((spinlock_t *)__lock, __irqflags);	\
}

#define RTMP_INT_UNLOCK(__lock, __irqflag)			\
{													\
	spin_unlock_irqrestore((spinlock_t *)(__lock), ((unsigned long)__irqflag));	\
}
#endif


#ifdef RT2860
#if 1
#if defined(INF_TWINPASS) || defined(IKANOS_VX_1X0)
//Patch for ASIC turst read/write bug, needs to remove after metel fix
#define RTMP_IO_READ32(_A, _R, _pV)									\
{																	\
	(*_pV = readl((void *)((_A)->CSRBaseAddress + MAC_CSR0)));		\
	(*_pV = readl((void *)((_A)->CSRBaseAddress + (_R))));			\
	(*_pV = SWAP32(*((UINT32 *)(_pV))));                           \
}
#define RTMP_IO_READ8(_A, _R, _pV)									\
{																	\
	(*_pV = readl((void *)((_A)->CSRBaseAddress + MAC_CSR0)));		\
	(*_pV = readb((void *)((_A)->CSRBaseAddress + (_R))));			\
}
#define RTMP_IO_WRITE32(_A, _R, _V)									\
{																	\
	UINT32	_Val;													\
	_Val = readl((void *)((_A)->CSRBaseAddress + MAC_CSR0));		\
	_Val = SWAP32(_V);												\
	writel(_Val, (void *)((_A)->CSRBaseAddress + (_R)));			\
}
#define RTMP_IO_WRITE8(_A, _R, _V)									\
{																	\
	UINT	Val;													\
	Val = readl((void *)((_A)->CSRBaseAddress + MAC_CSR0));		\
	writeb((_V), (PUCHAR)((_A)->CSRBaseAddress + (_R)));			\
}
#define RTMP_IO_WRITE16(_A, _R, _V)									\
{																	\
	UINT	Val;													\
	Val = readl((void *)((_A)->CSRBaseAddress + MAC_CSR0));		\
	writew(SWAP16((_V)), (PUSHORT)((_A)->CSRBaseAddress + (_R)));	\
}
#else
//Patch for ASIC turst read/write bug, needs to remove after metel fix
#define RTMP_IO_READ32(_A, _R, _pV)								\
{																\
	(*_pV = readl((void *)((_A)->CSRBaseAddress + MAC_CSR0)));		\
	(*_pV = readl((void *)((_A)->CSRBaseAddress + (_R))));			\
}
#define RTMP_IO_READ8(_A, _R, _pV)								\
{																\
	(*_pV = readl((void *)((_A)->CSRBaseAddress + MAC_CSR0)));			\
	(*_pV = readb((void *)((_A)->CSRBaseAddress + (_R))));				\
}
#define RTMP_IO_WRITE32(_A, _R, _V)												\
{																				\
	UINT	Val;																\
	Val = readl((void *)((_A)->CSRBaseAddress + MAC_CSR0));			\
	writel(_V, (void *)((_A)->CSRBaseAddress + (_R)));								\
}
#define RTMP_IO_WRITE8(_A, _R, _V)												\
{																				\
	UINT	Val;																\
	Val = readl((void *)((_A)->CSRBaseAddress + MAC_CSR0));			\
	writeb((_V), (PUCHAR)((_A)->CSRBaseAddress + (_R)));		\
}
#define RTMP_IO_WRITE16(_A, _R, _V)												\
{																				\
	UINT	Val;																\
	Val = readl((void *)((_A)->CSRBaseAddress + MAC_CSR0));			\
	writew((_V), (PUSHORT)((_A)->CSRBaseAddress + (_R)));	\
}
#endif
#else
//Patch for ASIC turst read/write bug, needs to remove after metel fix
#define RTMP_IO_READ32(_A, _R, _pV)								\
{																\
}
#define RTMP_IO_READ8(_A, _R, _pV)								\
{																\
}
#define RTMP_IO_WRITE32(_A, _R, _V)												\
{																				\
	printk("_V = %x\n", _V);													\
}
#define RTMP_IO_WRITE8(_A, _R, _V)												\
{																				\
	printk("_V = %x\n", _V);													\
}
#define RTMP_IO_WRITE16(_A, _R, _V)												\
{																				\
}
#endif 
#endif // RT2860 //


#ifndef wait_event_interruptible_timeout
#define __wait_event_interruptible_timeout(wq, condition, ret) \
do { \
        wait_queue_t __wait; \
        init_waitqueue_entry(&__wait, current); \
        add_wait_queue(&wq, &__wait); \
        for (;;) { \
                set_current_state(TASK_INTERRUPTIBLE); \
                if (condition) \
                        break; \
                if (!signal_pending(current)) { \
                        ret = schedule_timeout(ret); \
                        if (!ret) \
                                break; \
                        continue; \
                } \
                ret = -ERESTARTSYS; \
                break; \
        } \
        current->state = TASK_RUNNING; \
        remove_wait_queue(&wq, &__wait); \
} while (0)

#define wait_event_interruptible_timeout(wq, condition, timeout) \
({ \
        long __ret = timeout; \
        if (!(condition)) \
                __wait_event_interruptible_timeout(wq, condition, __ret); \
        __ret; \
})
#endif
#define ONE_TICK 1
#define OS_WAIT(_time) \
{	int _i; \
	long _loop = ((_time)/(1000/OS_HZ)) > 0 ? ((_time)/(1000/OS_HZ)) : 1;\
	wait_queue_head_t _wait; \
	init_waitqueue_head(&_wait); \
	for (_i=0; _i<(_loop); _i++) \
		wait_event_interruptible_timeout(_wait, 0, ONE_TICK); }


#ifdef WIN_NDIS
/* Modified by Wu Xi-Kun 4/21/2006 */
typedef void (*TIMER_FUNCTION)(
	IN  PVOID   SystemSpecific1, 
	IN  PVOID   FunctionContext, 
	IN  PVOID   SystemSpecific2, 
	IN  PVOID   SystemSpecific3);

#define RTMP_GREKEYPeriodicExec				GREKEYPeriodicExec
#define RTMP_CMTimerExec					CMTimerExec
#define RTMP_APQuickResponeForRateUpExec	APQuickResponeForRateUpExec
#else
/* Modified by Wu Xi-Kun 4/21/2006 */
typedef void (*TIMER_FUNCTION)(unsigned long);


#define RTMP_IndicateMediaState()
#define COPY_MAC_ADDR(Addr1, Addr2)             memcpy((Addr1), (Addr2), MAC_ADDR_LEN)

#define MlmeAllocateMemory(_pAd, _ppVA) os_alloc_mem(_pAd, _ppVA, MGMT_DMA_BUFFER_SIZE)
#define MlmeFreeMemory(_pAd, _pVA)     os_free_mem(_pAd, _pVA)

#ifdef RT2860
#define BUILD_TIMER_FUNCTION(_func)												\
void linux_##_func(unsigned long data)											\
{																				\
	PRALINK_TIMER_STRUCT	pTimer = (PRALINK_TIMER_STRUCT) data;				\
																				\
	_func(NULL, (PVOID) pTimer->cookie, NULL, NULL); 							\
	if (pTimer->Repeat)															\
		RTMP_OS_Add_Timer(&pTimer->TimerObj, pTimer->TimerValue);				\
}
#endif // RT2860 //



#define DECLARE_TIMER_FUNCTION(_func)			\
void linux_##_func(unsigned long data)			

#define GET_TIMER_FUNCTION(_func)				\
		linux_##_func							

#ifdef CONFIG_AP_SUPPORT
DECLARE_TIMER_FUNCTION(APDetectOverlappingExec);
#endif // CONFIG_AP_SUPPORT //
DECLARE_TIMER_FUNCTION(MlmePeriodicExec);
DECLARE_TIMER_FUNCTION(MlmeRssiReportExec);
DECLARE_TIMER_FUNCTION(AsicRxAntEvalTimeout);
DECLARE_TIMER_FUNCTION(APSDPeriodicExec);
DECLARE_TIMER_FUNCTION(AsicRfTuningExec);

#ifdef CONFIG_AP_SUPPORT
DECLARE_TIMER_FUNCTION(GREKEYPeriodicExec);
DECLARE_TIMER_FUNCTION(CMTimerExec);
DECLARE_TIMER_FUNCTION(APQuickResponeForRateUpExec);
DECLARE_TIMER_FUNCTION(WPARetryExec);
DECLARE_TIMER_FUNCTION(EnqueueStartForPSKExec);
DECLARE_TIMER_FUNCTION(APScanTimeout);
#ifdef IDS_SUPPORT
DECLARE_TIMER_FUNCTION(RTMPIdsPeriodicExec);
#endif // IDS_SUPPORT //
#ifdef WSC_AP_SUPPORT
DECLARE_TIMER_FUNCTION(WscEAPOLTimeOutAction);
DECLARE_TIMER_FUNCTION(Wsc2MinsTimeOutAction);
DECLARE_TIMER_FUNCTION(WscUPnPMsgTimeOutAction);
DECLARE_TIMER_FUNCTION(WscUPnPM2DTimeOutAction);
DECLARE_TIMER_FUNCTION(WscEnqueueEapolStart);
#endif // WSC_AP_SUPPORT //

#endif // CONFIG_AP_SUPPORT //


#endif

void RTMP_GetCurrentSystemTime(LARGE_INTEGER *time);


/*
 * packet helper 
 * 	- convert internal rt packet to os packet or 
 *             os packet to rt packet
 */      
#define RTPKT_TO_OSPKT(_p)		((struct sk_buff *)(_p))
#define OSPKT_TO_RTPKT(_p)		((PNDIS_PACKET)(_p))

#define GET_OS_PKT_DATAPTR(_pkt) \
		(RTPKT_TO_OSPKT(_pkt)->data)

#define GET_OS_PKT_LEN(_pkt) \
		(RTPKT_TO_OSPKT(_pkt)->len)

#define GET_OS_PKT_DATATAIL(_pkt) \
		(RTPKT_TO_OSPKT(_pkt)->tail)

#define GET_OS_PKT_HEAD(_pkt) \
		(RTPKT_TO_OSPKT(_pkt)->head)

#define GET_OS_PKT_END(_pkt) \
		(RTPKT_TO_OSPKT(_pkt)->end)

#define GET_OS_PKT_NETDEV(_pkt) \
		(RTPKT_TO_OSPKT(_pkt)->dev)

#define GET_OS_PKT_TYPE(_pkt) \
		(RTPKT_TO_OSPKT(_pkt))

#define GET_OS_PKT_NEXT(_pkt) \
		(RTPKT_TO_OSPKT(_pkt)->next)


#define OS_NTOHS(_Val) \
		(ntohs(_Val))
#define OS_HTONS(_Val) \
		(htons(_Val))
#define OS_NTOHL(_Val) \
		(ntohl(_Val))
#define OS_HTONL(_Val) \
		(htonl(_Val))

/* statistics counter */
#define STATS_INC_RX_PACKETS(_pAd, _dev)
#define STATS_INC_TX_PACKETS(_pAd, _dev)

#define STATS_INC_RX_BYTESS(_pAd, _dev, len)
#define STATS_INC_TX_BYTESS(_pAd, _dev, len)

#define STATS_INC_RX_ERRORS(_pAd, _dev)
#define STATS_INC_TX_ERRORS(_pAd, _dev)

#define STATS_INC_RX_DROPPED(_pAd, _dev)
#define STATS_INC_TX_DROPPED(_pAd, _dev)


#define CB_OFF  10


//   check DDK NDIS_PACKET data structure and find out only MiniportReservedEx[0..7] can be used by our driver without
//   ambiguity. Fields after pPacket->MiniportReservedEx[8] may be used by other wrapper layer thus crashes the driver
//
//#define RTMP_GET_PACKET_MR(_p)			(RTPKT_TO_OSPKT(_p))

// User Priority
#define RTMP_SET_PACKET_UP(_p, _prio)			(RTPKT_TO_OSPKT(_p)->cb[CB_OFF+0] = _prio)
#define RTMP_GET_PACKET_UP(_p)					(RTPKT_TO_OSPKT(_p)->cb[CB_OFF+0])

// Fragment #
#define RTMP_SET_PACKET_FRAGMENTS(_p, _num)		(RTPKT_TO_OSPKT(_p)->cb[CB_OFF+1] = _num)   
#define RTMP_GET_PACKET_FRAGMENTS(_p)			(RTPKT_TO_OSPKT(_p)->cb[CB_OFF+1])

// 0x0 ~0x7f: TX to AP's own BSS which has the specified AID. if AID>127, set bit 7 in RTMP_SET_PACKET_EMACTAB too. 
//(this value also as MAC(on-chip WCID) table index)
// 0x80~0xff: TX to a WDS link. b0~6: WDS index
#define RTMP_SET_PACKET_WCID(_p, _wdsidx)		(RTPKT_TO_OSPKT(_p)->cb[CB_OFF+2] = _wdsidx)
#define RTMP_GET_PACKET_WCID(_p)          		((UCHAR)(RTPKT_TO_OSPKT(_p)->cb[CB_OFF+2]))

// 0xff: PKTSRC_NDIS, others: local TX buffer index. This value affects how to a packet
#define RTMP_SET_PACKET_SOURCE(_p, _pktsrc)		(RTPKT_TO_OSPKT(_p)->cb[CB_OFF+3] = _pktsrc)
#define RTMP_GET_PACKET_SOURCE(_p)       		(RTPKT_TO_OSPKT(_p)->cb[CB_OFF+3])  

// RTS/CTS-to-self protection method
#define RTMP_SET_PACKET_RTS(_p, _num)      		(RTPKT_TO_OSPKT(_p)->cb[CB_OFF+4] = _num)
#define RTMP_GET_PACKET_RTS(_p)          		(RTPKT_TO_OSPKT(_p)->cb[CB_OFF+4])  
// see RTMP_S(G)ET_PACKET_EMACTAB

// TX rate index
#define RTMP_SET_PACKET_TXRATE(_p, _rate)		(RTPKT_TO_OSPKT(_p)->cb[CB_OFF+5] = _rate)
#define RTMP_GET_PACKET_TXRATE(_p)		  		(RTPKT_TO_OSPKT(_p)->cb[CB_OFF+5])

// From which Interface 
#define RTMP_SET_PACKET_IF(_p, _ifdx)		(RTPKT_TO_OSPKT(_p)->cb[CB_OFF+6] = _ifdx)
#define RTMP_GET_PACKET_IF(_p)		  		(RTPKT_TO_OSPKT(_p)->cb[CB_OFF+6])
#define RTMP_SET_PACKET_NET_DEVICE_MBSSID(_p, _bss)		RTMP_SET_PACKET_IF((_p), (_bss))
#define RTMP_SET_PACKET_NET_DEVICE_WDS(_p, _bss)		RTMP_SET_PACKET_IF((_p), ((_bss) + MIN_NET_DEVICE_FOR_WDS))
#define RTMP_SET_PACKET_NET_DEVICE_APCLI(_p, _idx)   	RTMP_SET_PACKET_IF((_p), ((_idx) + MIN_NET_DEVICE_FOR_APCLI))
#define RTMP_GET_PACKET_NET_DEVICE_MBSSID(_p)			RTMP_GET_PACKET_IF((_p))
#define RTMP_GET_PACKET_NET_DEVICE(_p)					RTMP_GET_PACKET_IF((_p))

#define RTMP_SET_PACKET_MOREDATA(_p, _morebit)		(RTPKT_TO_OSPKT(_p)->cb[CB_OFF+7] = _morebit)
#define RTMP_GET_PACKET_MOREDATA(_p)				(RTPKT_TO_OSPKT(_p)->cb[CB_OFF+7])

//#define RTMP_SET_PACKET_NET_DEVICE_MBSSID(_p, _bss)	(RTPKT_TO_OSPKT(_p)->cb[8] = _bss)
//#define RTMP_GET_PACKET_NET_DEVICE_MBSSID(_p)		(RTPKT_TO_OSPKT(_p)->cb[8])


#ifdef CONFIG_AP_SUPPORT
#ifdef UAPSD_AP_SUPPORT
/* if we queue a U-APSD packet to any software queue, we will set the U-APSD
   flag and its physical queue ID for it */
#define RTMP_SET_PACKET_UAPSD(_p, _flg_uapsd, _que_id) \
                    (RTPKT_TO_OSPKT(_p)->cb[CB_OFF+9] = ((_flg_uapsd<<7) | _que_id))

#define RTMP_SET_PACKET_QOS_NULL(_p)     (RTPKT_TO_OSPKT(_p)->cb[CB_OFF+9] = 0xff)
#define RTMP_GET_PACKET_QOS_NULL(_p)	 (RTPKT_TO_OSPKT(_p)->cb[CB_OFF+9])
#define RTMP_SET_PACKET_NON_QOS_NULL(_p) (RTPKT_TO_OSPKT(_p)->cb[CB_OFF+9] = 0x00)
#define RTMP_GET_PACKET_UAPSD_Flag(_p)   (((RTPKT_TO_OSPKT(_p)->cb[CB_OFF+9]) & 0x80) >> 7)
#define RTMP_GET_PACKET_UAPSD_QUE_ID(_p) ((RTPKT_TO_OSPKT(_p)->cb[CB_OFF+9]) & 0x7f)

#define RTMP_SET_PACKET_EOSP(_p, _flg)   (RTPKT_TO_OSPKT(_p)->cb[CB_OFF+10] = _flg)
#define RTMP_GET_PACKET_EOSP(_p)         (RTPKT_TO_OSPKT(_p)->cb[CB_OFF+10])
#endif // UAPSD_SUPPORT //
#endif // CONFIG_AP_SUPPORT //


#if 0
//#define RTMP_SET_PACKET_DHCP(_p, _flg)   	(RTPKT_TO_OSPKT(_p)->cb[CB_OFF+11] = _flg)
//#define RTMP_GET_PACKET_DHCP(_p)         	(RTPKT_TO_OSPKT(_p)->cb[CB_OFF+11])
#else
//
//	Sepcific Pakcet Type definition
//
#define RTMP_PACKET_SPECIFIC_CB_OFFSET	11

#define RTMP_PACKET_SPECIFIC_DHCP		0x01
#define RTMP_PACKET_SPECIFIC_EAPOL		0x02
#define RTMP_PACKET_SPECIFIC_IPV4		0x04
#define RTMP_PACKET_SPECIFIC_VLAN		0x10
#define RTMP_PACKET_SPECIFIC_LLCSNAP	0x20

//Specific
#define RTMP_SET_PACKET_SPECIFIC(_p, _flg)	   	(RTPKT_TO_OSPKT(_p)->cb[CB_OFF+11] = _flg)												

//DHCP
#define RTMP_SET_PACKET_DHCP(_p, _flg)   													\
			do{																				\
				if (_flg)																	\
					(RTPKT_TO_OSPKT(_p)->cb[CB_OFF+11]) |= (RTMP_PACKET_SPECIFIC_DHCP);		\
				else																		\
					(RTPKT_TO_OSPKT(_p)->cb[CB_OFF+11]) &= (!RTMP_PACKET_SPECIFIC_DHCP);	\
			}while(0)
#define RTMP_GET_PACKET_DHCP(_p)		(RTPKT_TO_OSPKT(_p)->cb[CB_OFF+11] & RTMP_PACKET_SPECIFIC_DHCP)

//EAPOL
#define RTMP_SET_PACKET_EAPOL(_p, _flg)   													\
			do{																				\
				if (_flg)																	\
					(RTPKT_TO_OSPKT(_p)->cb[CB_OFF+11]) |= (RTMP_PACKET_SPECIFIC_EAPOL);		\
				else																		\
					(RTPKT_TO_OSPKT(_p)->cb[CB_OFF+11]) &= (!RTMP_PACKET_SPECIFIC_EAPOL);	\
			}while(0)
#define RTMP_GET_PACKET_EAPOL(_p)		(RTPKT_TO_OSPKT(_p)->cb[CB_OFF+11] & RTMP_PACKET_SPECIFIC_EAPOL)

#define RTMP_GET_PACKET_LOWRATE(_p)		(RTPKT_TO_OSPKT(_p)->cb[CB_OFF+11] & (RTMP_PACKET_SPECIFIC_EAPOL | RTMP_PACKET_SPECIFIC_DHCP))

//VLAN
#define RTMP_SET_PACKET_VLAN(_p, _flg)   													\
			do{																				\
				if (_flg)																	\
					(RTPKT_TO_OSPKT(_p)->cb[CB_OFF+11]) |= (RTMP_PACKET_SPECIFIC_VLAN);		\
				else																		\
					(RTPKT_TO_OSPKT(_p)->cb[CB_OFF+11]) &= (!RTMP_PACKET_SPECIFIC_VLAN);	\
			}while(0)
#define RTMP_GET_PACKET_VLAN(_p)		(RTPKT_TO_OSPKT(_p)->cb[CB_OFF+11] & RTMP_PACKET_SPECIFIC_VLAN)

//LLC/SNAP
#define RTMP_SET_PACKET_LLCSNAP(_p, _flg)   												\
			do{																				\
				if (_flg)																	\
					(RTPKT_TO_OSPKT(_p)->cb[CB_OFF+11]) |= (RTMP_PACKET_SPECIFIC_LLCSNAP);	\
				else																		\
					(RTPKT_TO_OSPKT(_p)->cb[CB_OFF+11]) &= (!RTMP_PACKET_SPECIFIC_LLCSNAP);	\
			}while(0)
			
#define RTMP_GET_PACKET_LLCSNAP(_p)		(RTPKT_TO_OSPKT(_p)->cb[CB_OFF+11] & RTMP_PACKET_SPECIFIC_LLCSNAP)

// IP
#define RTMP_SET_PACKET_IPV4(_p, _flg)														\
			do{																				\
				if (_flg)																	\
					(RTPKT_TO_OSPKT(_p)->cb[CB_OFF+11]) |= (RTMP_PACKET_SPECIFIC_IPV4);		\
				else																		\
					(RTPKT_TO_OSPKT(_p)->cb[CB_OFF+11]) &= (!RTMP_PACKET_SPECIFIC_IPV4);	\
			}while(0)
			
#define RTMP_GET_PACKET_IPV4(_p)		(RTPKT_TO_OSPKT(_p)->cb[CB_OFF+11] & RTMP_PACKET_SPECIFIC_IPV4)
	
#endif


// If this flag is set, it indicates that this EAPoL frame MUST be clear. 
#define RTMP_SET_PACKET_CLEAR_EAP_FRAME(_p, _flg)   (RTPKT_TO_OSPKT(_p)->cb[CB_OFF+12] = _flg)
#define RTMP_GET_PACKET_CLEAR_EAP_FRAME(_p)         (RTPKT_TO_OSPKT(_p)->cb[CB_OFF+12])

#define RTMP_SET_PACKET_5VT(_p, _flg)   (RTPKT_TO_OSPKT(_p)->cb[CB_OFF+22] = _flg)
#define RTMP_GET_PACKET_5VT(_p)         (RTPKT_TO_OSPKT(_p)->cb[CB_OFF+22])

#ifdef CONFIG_5VT_ENHANCE
#define BRIDGE_TAG 0x35564252    // depends on 5VT define in br_input.c
#endif


#define NDIS_SET_PACKET_STATUS(_p, _status)


#define GET_SG_LIST_FROM_PACKET(_p, _sc)	\
    rt_get_sg_list_from_packet(_p, _sc)

#if 0
// NOTE: Has to copy 802.3 header to head of pData for compatibility 
// with older OS eariler than W2K
#define REPORT_ETHERNET_FRAME_TO_LLC(_pAd, _p8023hdr, _pData, _DataSize)    \
{                                                                           \
    NdisMoveMemory(_pData - LENGTH_802_3, _p8023hdr, LENGTH_802_3);         \
    _pAd->pRxData = _pData;                                                 \
    _pAd->Counters8023.GoodReceives++;                                      \
}
#endif



#define NdisMoveMemory(Destination, Source, Length) memmove(Destination, Source, Length)
#define NdisZeroMemory(Destination, Length)         memset(Destination, 0, Length)
#define NdisFillMemory(Destination, Length, Fill)   memset(Destination, Fill, Length)
#define NdisEqualMemory(Source1, Source2, Length)   (!memcmp(Source1, Source2, Length))
#define RTMPEqualMemory(Source1, Source2, Length)	(!memcmp(Source1, Source2, Length))							


#define RTMP_INC_REF(_A)		0
#define RTMP_DEC_REF(_A)		0
#define RTMP_GET_REF(_A)		0



/*
 * ULONG
 * RTMP_GetPhysicalAddressLow(
 *   IN NDIS_PHYSICAL_ADDRESS  PhysicalAddress);
 */
#define RTMP_GetPhysicalAddressLow(PhysicalAddress)		(PhysicalAddress)

/*
 * ULONG
 * RTMP_GetPhysicalAddressHigh(
 *   IN NDIS_PHYSICAL_ADDRESS  PhysicalAddress);
 */
#define RTMP_GetPhysicalAddressHigh(PhysicalAddress)		(0)

/*
 * VOID
 * RTMP_SetPhysicalAddressLow(
 *   IN NDIS_PHYSICAL_ADDRESS  PhysicalAddress,
 *   IN ULONG  Value);
 */
#define RTMP_SetPhysicalAddressLow(PhysicalAddress, Value)	\
			PhysicalAddress = Value;

/*
 * VOID
 * RTMP_SetPhysicalAddressHigh(
 *   IN NDIS_PHYSICAL_ADDRESS  PhysicalAddress,
 *   IN ULONG  Value);
 */
#define RTMP_SetPhysicalAddressHigh(PhysicalAddress, Value)


//CONTAINING_RECORD(pEntry, NDIS_PACKET, MiniportReservedEx);
#define QUEUE_ENTRY_TO_PACKET(pEntry) \
	(PNDIS_PACKET)(pEntry)

#define PACKET_TO_QUEUE_ENTRY(pPacket) \
	(PQUEUE_ENTRY)(pPacket)


#ifndef CONTAINING_RECORD
#define CONTAINING_RECORD(address, type, field)			\
((type *)((PCHAR)(address) - offsetof(type, field)))
#endif


#define RELEASE_NDIS_PACKET(_pAd, _pPacket, _Status)                    \
{                                                                       \
    if (RTMP_GET_PACKET_SOURCE(_pPacket) == PKTSRC_NDIS)                \
    {                                                                   \
		RTMPFreeNdisPacket(_pAd, _pPacket);                             \
        _pAd->RalinkCounters.PendingNdisPacketCount --;                 \
    }                                                                   \
    else                                                                \
        RTMPFreeNdisPacket(_pAd, _pPacket);                             \
}


#define SWITCH_PhyAB(_pAA, _pBB)    \
{                                                                           \
    ULONG	AABasePaHigh;                           \
    ULONG	AABasePaLow;                           \
    ULONG	BBBasePaHigh;                           \
    ULONG	BBBasePaLow;                           \
    BBBasePaHigh = RTMP_GetPhysicalAddressHigh(_pBB);                                                 \
    BBBasePaLow = RTMP_GetPhysicalAddressLow(_pBB);                                                 \
    AABasePaHigh = RTMP_GetPhysicalAddressHigh(_pAA);                                                 \
    AABasePaLow = RTMP_GetPhysicalAddressLow(_pAA);                                                 \
    RTMP_SetPhysicalAddressHigh(_pAA, BBBasePaHigh);                                                 \
    RTMP_SetPhysicalAddressLow(_pAA, BBBasePaLow);                                                 \
    RTMP_SetPhysicalAddressHigh(_pBB, AABasePaHigh);                                                 \
    RTMP_SetPhysicalAddressLow(_pBB, AABasePaLow);                                                 \
}


#define NdisWriteErrorLogEntry(_a, _b, _c, _d)
#define NdisMAllocateMapRegisters(_a, _b, _c, _d, _e)		NDIS_STATUS_SUCCESS


#define NdisAcquireSpinLock		RTMP_SEM_LOCK
#define NdisReleaseSpinLock		RTMP_SEM_UNLOCK

static inline void NdisGetSystemUpTime(ULONG *time)
{
	*time = jiffies;
}

//pPacket = CONTAINING_RECORD(pEntry, NDIS_PACKET, MiniportReservedEx);
#define QUEUE_ENTRY_TO_PKT(pEntry) \
		((PNDIS_PACKET) (pEntry))

int rt28xx_packet_xmit(struct sk_buff *skb);











