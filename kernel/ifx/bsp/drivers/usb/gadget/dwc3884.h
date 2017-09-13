/*
 * Device controller for the Synopsys 3884-0 DWC USB 2.0 HS OTG Subsystem-AHB.
 * Specs and errata are available from http://www.synopsys.com.
 *
 * Synopsys Inc. supported the development of this driver.
 *  
 */

/*
 * Copyright (C) 2006 Christian Schroeder, EMSYS GmbH, Ilmenau, Germany
 * Copyright (C) 2006 EMSYS GmbH, ILmenau, Germany, GmbH (http://www.emsys.de) 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*----------------------------- general settings -----------------------------*/

/* #define DEBUG */
/* #define VERBOSE */

#define GADGET_SUPPORTS_DMA          1       /* DMA mode available */
#define GADGET_NEEDS_ALIGNED_BUFFERS 1       /* 32 bit aligned buffers needed */
#define GADGET_HAS_ONE_STEP_SETADDR  1       /* set address in one step */        
#define GADGET_UTMI_PHY_BUSWIDTH     16      /* phy bus width (16 bit or 8 bit) */
#define GADGET_TOUT_CALIBRATION      0       /* timeout calibration (0..7) */
#define GADGET_AHB_BURSTLENGTH       7       /* ahb burst length (0..7) */
/*---------------------------- naming macros ---------------------------------*/

#define __GADGET                   dwc3884
#define __CONCAT(x,y)              x ## y
#define __STGFY(x)                 # x
#define GADGET_NAME(x,y)           __CONCAT(x,y)
#define GADGET_STRING(x)           __STGFY(x)

/*---------------------------- flag macros -----------------------------------*/

#define SET_FLAG(p,r)              (*r)  |= 1<<(p)
#define CLEAR_FLAG(p,r)            (*r)  &= ~(1<<(p))
#define TEST_FLAG(p,r)             ((*r) & 1<<(p)) ? 1 : 0
#define MIN(x,y)                   (x < y) ? x : y
#define MAX(x,y)                   (x > y) ? x : y

/*----------------------------- local constants ------------------------------*/

#define DMA_ADDR_INVALID           (~(dma_addr_t)0)    /* invalid dma address */
#define EP_COUNT                   16                    /* maximum number of endpoints */
#define EP0_BUFLEN                 128                   /* length of the ep0 buffer */
#define CONNECT_DEBOUNCE           100                   /* milliseconds */
#define DISCONNECT_DEBOUNCE        100                   /* milliseconds */

#define CSR_BASE                   0xBE101000
#define CSR_SIZE                   0x00001000
#define FIFO_BASE                  0xBE120000            /* 2048 x 35 bit FIFO */
#define FIFO_SIZE                  0x0003F000 
#define CORE_REGS_OFFSET           0x00000000
#define PWCLK_REGS_OFFSET          0x00000E00
#define USBD_REGS_OFFSET           0x00000800
#define EP_IN_REGS_OFFSET          0x00000900
#define EP_OUT_REGS_OFFSET         0x00000B00

/* ep0 plus up to 15 data endpoints */
#define EP_REG_SIZE                0x20                  /* size of ep's register block */
#define FIFO_SIZE_PERIODIC         32                    /* tx fifo size for periodic ep */

/* additional registers to set partially */
#define GPIO_REGS_BASE             0xBE100B00
#define PMU_REGS_BASE              0xBF102000
#define CGU_REGS_BASE              0xBF103000
#define RCU_REGS_BASE              0xBF203000

/* missing in usb.h */
#define USB_ENDPOINT_XFER_NONE     4
#define USB_DIR_BIDIR              0xFE
#define USB_DIR_NONE               0xFF
#define USB_FEATURE_STALL          0
#define USB_FEATURE_WKUP           1
#define USB_DIR_MASK               0x80

/* endpoint related status flags */
#define EP_IS_STOPPED              0                     /* stopped by software */
#define EP_IS_DEACTIVATED          1                     /* dectivated (not ep0) */
#define EP_IS_DISABLED             2                     /* NAK handshake */
#define EP_IS_STALLED              3                     /* STALL handshake */
#define EP_QUEUE_STOP_PENDING      6                     /* fifo stop sequence started */
#define EP_QUEUE_STOPPED           7                     /* fifo stopped */

/* device related status flags */
#define DEV_IS_SELFPOWERED         0
#define DEV_HAS_REM_WAKEUP         1
#define DEV_IS_INITIALIZED         2
#define DEV_IS_ENABLED             3
#define DEV_GOT_IRQ                4
#define DEV_GOT_MEM_REGION         5
#define DEV_CTRL_REQ_WRITE         6
#define DEV_CTRL_REQ_DATALESS      7
#define DEV_EP0_STATUS_DELAY       8 

/* request related status flags */
#define REQ_IS_QUEUED              1
#define REQ_IS_UNALIGNED           2

/* td flags */
#define TD_XFLEN                   0x7                   /* request length for low level control request */
#define TD_IS_LAST                 3                     /* the last td of a request */
#define TD_IS_STATUS               4                     /* status stage instead of data stage */
#define TD_SET_ADDRESS             5                     /* set the device address */
#define TD_STALL_EP0               6                     /* stall ep0 */
#define TD_IS_CTRL_OUT             7                     /* control out transfer */
#define TD_IS_SHORT                8                     /* end of transfer */

/* misc */
#define EP_STALL                   1
#define EP_DESTALL                 1
#define EP_NO_STALL                0
#define EP_NO_DESTALL              0

typedef enum ERROR {
  OK = 0, FAIL = 1, INVALID = 2
} ERROR;

typedef enum BOOLEAN {
  FALSE = 0, TRUE = 1
} BOOLEAN;

/*---------------------------- register interface ----------------------------*/

#include "dwc3884_regs.h"

/*---------------------------- driver specific data types --------------------*/

#ifdef  __KERNEL__

enum GADGET_NAME(__GADGET,_xfer_errror) {
  XFER_OK    = 0,
  XFER_ERROR = 1
};

union GADGET_NAME(__GADGET,_rq_data) {
  u32 raw[2];
  struct usb_ctrlrequest ctrlreq;
};

typedef union GADGET_NAME(__GADGET,_rq_data) GADGET_NAME(__GADGET,_rq_data);

struct GADGET_NAME(__GADGET,_xfer_status) {
  struct GADGET_NAME(__GADGET,_ep*)          endpoint;   /* the endpoint */
  struct GADGET_NAME(__GADGET,_request*)     request;    /* actual processed request */
  unsigned long                              expected;   /* expected xfer size */
  unsigned long                              actual;     /* transferred size */
  unsigned char                              error;      /* transfer error */ 
};

typedef struct GADGET_NAME(__GADGET,_xfer_status) GADGET_NAME(__GADGET,_xfer_status);

struct GADGET_NAME(__GADGET,_ep) {
  struct usb_ep                              ep;         /* the ep */
  struct GADGET_NAME(__GADGET,_dev*)         dev;        /* the controller */
  unsigned long                              irqs;       /* interrupt counter */
  const struct usb_endpoint_descriptor*      desc;       /* the endpoint descriptor */
  unsigned char                              num;        /* endpoint index */
  unsigned char                              type;       /* endpoint type */
  unsigned char                              dir;        /* endpoint direction */
  unsigned char                              fifo_space; /* available fifo space */
  unsigned short                             fifo_size;  /* fifo size */
  unsigned char                              fifo_num;   /* number of periodic transmit fifo */
  GADGET_NAME(__GADGET,_xfer_status)         xfstat;     /* the transfer descriptor */
  unsigned short                             flags;      /* ep status flags */
  struct GADGET_NAME(__GADGET,_ep_in_regs __iomem*)  in_regs;    
                                                         /* the in ep specific registers */
  struct GADGET_NAME(__GADGET,_ep_out_regs __iomem*) out_regs;   
                                                         /* the out ep specific registers */
  struct list_head                           queue;      /* request head for td queue */
};

typedef struct GADGET_NAME(__GADGET,_ep) GADGET_NAME(__GADGET,_ep);

struct GADGET_NAME(__GADGET,_request) {
  struct usb_request                         req;        /* the reqest */
  unsigned short                             flags;      /* status flags */
  struct list_head                           queue;      /* list head for request queue */
  dma_addr_t                                 dma;        /* the request buffers dma address */
#if GADGET_NEEDS_ALIGNED_BUFFERS
  void*                                      unaligned_buf; /* original buffer from gadget in 
                                                               case of REQ_IS_UNALIGNED */
#endif
};

typedef struct GADGET_NAME(__GADGET,_request) GADGET_NAME(__GADGET,_request);

struct GADGET_NAME(__GADGET,_td) {
  unsigned long                              length;     /* transfer length */
  unsigned long                              packets;    /* packet count */
  GADGET_NAME(__GADGET,_request*)            req;        /* associated request */
  unsigned long                              seqnum;     /* the sequence number */
  unsigned short                             flags;      /* status flags */
  volatile void __iomem*                     buf;        /* buffer address */
  struct list_head                           queue;      /* list head for td queue */
  dma_addr_t                                 dma;        /* dma address */
};

typedef struct GADGET_NAME(__GADGET,_td) GADGET_NAME(__GADGET,_td);

struct GADGET_NAME(__GADGET,_dev) {
  struct usb_gadget                          gadget;     /* the gadget */
  spinlock_t                                 lock;       /* a spin lock */
  struct GADGET_NAME(__GADGET,_ep)           ep[EP_COUNT]; /* the endpoints */ 
  struct usb_gadget_driver*                  driver;     /* the connected gadget driver */
  unsigned char                              speed;      /* the usb speed */
  unsigned short                             flags;      /* status flags */
  unsigned long                              chiprev;    /* the chip id */
  unsigned long                              tflen;      /* maximum transfer length */
  unsigned long                              pkcnt;      /* maximum packet counter length */
  unsigned short                             addr;       /* the usb address */
  unsigned short                             epcnt;      /* number of available endpoints */
  unsigned char                              disable_cnt; /* number of eps to disable (GLOBAL IN NAK...) */
  unsigned char                              usb_state;  /* the usb state (USB_STATE_* constants) */
  unsigned char                              prev_usb_state;  /* the previous usb state (used with suspend, wakeup) */
  /* periodic endpoints need to be assigned to a specific fifo */
  unsigned char                              tx_fifo_num[EP_COUNT];
  unsigned char                              tx_fifo_num_max;
  /* a local buffer for receiving setup packets and handling low level requests */
  volatile void __iomem*                     buf;        /* the cpu view */
  dma_addr_t                                 dma;        /* the hardware view */

  /* global registers */
  struct GADGET_NAME(__GADGET,_core_regs __iomem*)   core_regs;  
                                                         /* the otg core registers */
  struct GADGET_NAME(__GADGET,_pwclk_regs __iomem*)  pwclk_regs; 
                                                         /* the power and clock registers */
  struct GADGET_NAME(__GADGET,_usbd_regs __iomem*)   usbd_regs;  
                                                         /* the usb device registers */
  struct GADGET_NAME(__GADGET,_rcu_regs __iomem*)    rcu_regs; 
                                                         /* the reset control unit registers */
  struct GADGET_NAME(__GADGET,_pmu_regs __iomem*)    pmu_regs; 
                                                         /* the power management unit registers */
  struct GADGET_NAME(__GADGET,_cgu_regs __iomem*)    cgu_regs; 
                                                         /* the clock gating unit registers */
  struct GADGET_NAME(__GADGET,_gpio_regs __iomem*)   gpio_regs; 
                                                         /* the gpio registers */
};

typedef struct GADGET_NAME(__GADGET,_dev) GADGET_NAME(__GADGET,_dev);

/* endpoint configuartion table */
struct ep_config {
  unsigned short                             mps;        /* maximum packet size */
  unsigned char                              type;       /* endpoint type */
  unsigned char                              dir;        /* endpoint direction */
  unsigned long                              buflen;     /* maximum buffer length */
};

typedef struct ep_config ep_config;

typedef unsigned char byte;
typedef unsigned short word16;
typedef unsigned long word32;

/*--------------------------- port access layer ------------------------------*/

/* FIXME: the functions are not atomic, do we have to spinlock the
          kernel for each register write operation */

/* returns the position of the fist "1" bit in a 32 bit word */
static inline u8 find_bit(volatile u32* r, byte first_pos)
{
  int i;
  u32 v;

  v = readl(r)>>first_pos;
  for (i=0; i<=32-first_pos; i++) {
    if ((v >> i) & 1) {
    return i;
    }
  }
  return 0xff;
}

/* simple (2 pow n) function */
static inline u32 pow2(u8 n)
{
  unsigned long p;
  int i;

  p=1;
  for (i=0;i<n;i++) {
    p*=2;
  }
  return p;
}

/* simple bitfield width function */
static inline u8 bw(u32 n)
{
  u8 i = 0;
  while(n) {
    n = n/2;
    i++;
  }
  return i;
}

/* calculates a mask for the slice functions below */
static inline u32 mask(u8 lo, u8 hi)
{
  int i;
  unsigned long m = 0;

  for (i=0; i<(hi-lo+1); i++) {
    m |= (1<<i);
  }
  return (m <<= lo);
}

/* reads a slice of bits from a 32 bit word*/
static inline u32 read_slice(u8 lo, u8 hi, volatile u32* r)
{
  return((readl(r) & (mask(lo,hi))) >> lo);
}

/* tests a bit in a 32 bit word */
static inline u8 test_rbit(u8 b, volatile u32* r)
{
  if (readl(r) & (1 << b)) {
    return 1;
  }
  else {
    return 0;
  }
}

/* sets a bit in a 32 bit word */
static inline void set_rbit(u8 b, volatile u32* r)
{
    writel((readl(r) | (1 << b)),r);
}

/* clears a bit in a 32 bit word */
static inline void clear_rbit(u8 b, volatile u32* r)
{
    writel((readl(r) & ~(1 << b)),r);
}

/* writes a slice of bits to a 32 bit word */
static inline void write_slice(u8 lo, u8 hi, volatile u32* r, u32 v)
{
  writel((readl(r) & ~mask(lo,hi)) | (v << lo),r);
}

/* writes a complete 32 bit register */
static inline void write_reg(volatile u32* r, u32 v)
{
  writel(v,r);
}

static inline u32 read_reg(volatile u32* r)
{
  return readl(r);
}

/*---------------------------- debugging stuff ----------------------------*/

#define xprintk(dev,level,fmt,args...) \
  printk(level "%s: " fmt , driver_name , ## args)

#ifdef DEBUG
#undef DEBUG
#define DEBUG(dev,fmt,args...) \
  xprintk(dev , KERN_DEBUG , fmt , ## args)
#else
#define DEBUG(dev,fmt,args...) \
  do { } while (0)
#endif

#ifdef VERBOSE
#define VDEBUG DEBUG
#else
#define VDEBUG(dev,fmt,args...) \
  do { } while (0)
#endif

#define ERROR(dev,fmt,args...) \
  xprintk(dev , KERN_ERR , fmt , ## args)
#define WARN(dev,fmt,args...)  \
  xprintk(dev , KERN_WARNING , fmt , ## args)
#define INFO(dev,fmt,args...)  \
  xprintk(dev , KERN_INFO , fmt , ## args)

/*---------------------------- portability --------------------------------*/

#ifndef container_of
#define container_of list_entry
#endif

#ifndef likely
#define likely(x)    (x)
#define unlikely(x)  (x)
#endif

#ifndef BUG_ON
#define BUG_ON(condition) \
  do { if (unlikely((condition)!=0)) BUG(); } while(0)
#endif

#ifndef WARN_ON
#define WARN_ON(x) \
  do { } while (0)
#endif

#ifndef IRQ_NONE
typedef void irqreturn_t;
#define IRQ_NONE
#define IRQ_HANDLED
#define IRQ_RETVAL(x)
#endif

#endif  /* __KERNEL__ */
