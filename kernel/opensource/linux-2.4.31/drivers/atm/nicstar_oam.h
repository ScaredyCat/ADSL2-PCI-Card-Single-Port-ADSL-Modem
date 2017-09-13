/******************************************************************************
 *
 * nicstar.h
 *
 * Date: 28/05/2004
 * Version: 2.01
 *
 * Header file for the LINUX nicstar2 device driver for PROATM cards.
 *
 * IMPORTANT: This file is derived from the nicstar.h file from Rui Prior 
 *             Some portions of this file still exist here.
 *
 *
 * Author: Christian Bucari: bucari@prosum.net
 * Copyright (c) 2002, 2003, 2004 Prosum
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it 
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation. 
 * You should have received a copy of the  GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 ******************************************************************************
 */


#ifndef _LINUX_NICSTAR_H_
#define _LINUX_NICSTAR_H_

#undef NS_DEBUG_SPINLOCKS

/* Includes *******************************************************************/

#include <linux/types.h>
#include <linux/pci.h>
#include <linux/uio.h>
#include <linux/skbuff.h>
#include <linux/atmdev.h>
#include <linux/atm_nicstar.h>


/* Options ********************************************************************/

#define NS_RCQ_SUPPORT          /* Enable RAW CELL QUEUE receipt */
#define NS_VPIBITS 2            /* 0, 1, 2, or 8 */
#define NS_VCIBASE 0
#define NS_VPIBASE 0

#define NS_B0BUFSIZE 240        /* 48, 96, 240 or 2048 */
#define NS_B1BUFSIZE 16384      /* 2048, 4096, 8192 or 16384 */
#define NS_TST_RESERVED 340     /* N. entries reserved for UBR/ABR/VBR */

/******************************************************************************/

/* Threshold values : reg #38-#41*/
        /* unit = 512/16 = 32 */
#define NS_B0THLD    (0x1<<28)
#define NS_B1THLD    (0x1<<28)
#define NS_B2THLD    0x0
#define NS_B3THLD    0x0
#define NS_B0NITHLD  0x0
#define NS_B1NITHLD  0x0
#define NS_B2NITHLD  0x0
#define NS_B3NITHLD  0x0
#define NS_B0CITHLD  0x0
#define NS_B1CITHLD  0x0
#define NS_B2CITHLD  0x0
#define NS_B3CITHLD  0x0
#define NS_B0SIZE    (NS_B0BUFSIZE/48)  /*5*/
#define NS_B1SIZE    (NS_B1BUFSIZE/48)  /*341*/

        /* Number of buffers initially allocated */

#define NUM_B0 48
#define NUM_B1 48       /* Must be even */
#define NUM_HB 4        /* Pre-allocated huge buffers */
#define NUM_IOVB 128    /* Iovec buffers */

        /* Free buffers queue pointers */

#define NS_FBQ0_RP  0x0 
#define NS_FBQ0_WP  0x00000000 /*NUM_B0=32 in decimal */
#define NS_FBQ1_RP  0x0 
#define NS_FBQ1_WP  0x00000000 /*NUM_B1=32 in decimal */
#define NS_FBQ2_RP  0x0 
#define NS_FBQ2_WP  0x0
#define NS_FBQ3_RP  0x0 
#define NS_FBQ3_WP  0x0

        /* Lower level for count of buffers */
#define MIN_B0 16       /* Must be even */
#define MIN_B1 16       /* Must be even */
#define MIN_HB 6
#define MIN_IOVB 16

        /* Upper level for count of buffers */
#define MAX_B0 256 /*64*/       /* Must be even, <= 508 */
#define MAX_B1 256 /*64*/       /* Must be even, <= 508 */
#define MAX_HB 10
#define MAX_IOVB 256 /*80*/

        /* These are the absolute maximum allowed for the ioctl() */
#define TOP_B0 508 /*256*/      /* Must be even, <= 508 */
#define TOP_B1 508 /*128*/      /* Must be even, <= 508 */
#define TOP_HB 64
#define TOP_IOVB 256


#define MAX_TBD_PER_VC 1        /* Number of TBDs before a TSR */
#define MAX_TBD_PER_SCQ 20      /* Only meaningful for variable rate SCQs */

#define SCQFULL_TIMEOUT (5 * HZ)

#define NS_POLL_PERIOD (HZ)

#define PCR_TOLERANCE (1.0001)



/* ESI stuff ******************************************************************/

#define NICSTAR_EPROM_PROSUM_MAC_ADDR_OFFSET 36
#define NICSTAR_EPROM_IDT_MAC_ADDR_OFFSET 0x6C 
#define PROSUM_MAC_0 0x00
#define PROSUM_MAC_1 0xC0
#define PROSUM_MAC_2 0xFD

/* #defines *******************************************************************/

#define NS_MAX_CARDS 5          /* Maximum number of cards. Must be <= 5 */
#define NS_RSQSIZE 8192         /* 2048, 4096 or 8192 */
#define NS_IOREMAP_SIZE 4096

#define BUF_B0 0x00000000       /* These are used for push_rxbufs() */
#define BUF_B1 0x00000001       /* register CMD, Write_FreeBufQ : BUFQS bits */
#define BUF_B2 0x00000002
#define BUF_B3 0x00000003

#define NS_HBUFSIZE 65568       /* Size of max. AAL5 PDU */
#define NS_MAX_IOVECS (2 + (65568 - NS_B0BUFSIZE) / \
                       (NS_B1BUFSIZE - (NS_B1BUFSIZE % 48)))
#define NS_IOVBUFSIZE (NS_MAX_IOVECS * (sizeof(struct iovec)))

#define NS_B0BUFSIZE_USABLE (NS_B0BUFSIZE - NS_B0BUFSIZE % 48)
#define NS_B1BUFSIZE_USABLE (NS_B1BUFSIZE - NS_B1BUFSIZE % 48)

#define NS_AAL0_HEADER (ATM_AAL0_SDU - ATM_CELL_PAYLOAD)        /* 4 bytes */

#define NS_B0SKBSIZE (NS_B0BUFSIZE + NS_AAL0_HEADER)
#define NS_B1SKBSIZE (NS_B0BUFSIZE + NS_B1BUFSIZE)


/* NICStAR structures located in host memory **********************************/

/* RSQ - Receive Status Queue 
 *
 * Written by the NICStAR, read by the device driver.
 */

typedef struct ns_rsqe
{
   u32 word_1;
   u32 buffer_handle;
   u32 final_aal5_crc32;
   u32 word_4;
} ns_rsqe;

#define ns_rsqe_vpi(ns_rsqep) \
        ((le32_to_cpu((ns_rsqep)->word_1) & 0x00FF0000) >> 16)
#define ns_rsqe_vci(ns_rsqep) \
        (le32_to_cpu((ns_rsqep)->word_1) & 0x0000FFFF)
#define ns_rsqe_pti(ns_rsqep) \
        ((le32_to_cpu((ns_rsqep)->word_1) & 0xFF000000) >> 24)

#define NS_RSQE_TYPE       0xC0000000 
#define NS_RSQE_VALID      0x80000000   /* still exists */
#define NS_RSQE_POOL       0x00030000
#define NS_RSQE_BUFASSIGN  0x00008000  
#define NS_RSQE_NZGFC      0x00004000
#define NS_RSQE_EOPDU      0x00002000
#define NS_RSQE_CBUF       0x00001000 
#define NS_RSQE_CONGESTION 0x00000800
#define NS_RSQE_CLP        0x00000400
#define NS_RSQE_CRCERR     0x00000200

#define NS_RSQE_BUFSIZE_B0 0x00000000
#define NS_RSQE_BUFSIZE_B1 0x00001000

#define ns_rsqe_type(ns_rsqep) \
        (le32_to_cpu((ns_rsqep)->word_4) & NS_RSQE_TYPE)
#define ns_rsqe_valid(ns_rsqep) \
        (le32_to_cpu((ns_rsqep)->word_4) & NS_RSQE_VALID)
#define ns_rsqe_pool(ns_rsqep) \
        (le32_to_cpu((ns_rsqep)->word_4) & NS_RSQE_POOL)
#define ns_rsqe_bufassign(ns_rsqep) \
        (le32_to_cpu((ns_rsqep)->word_4) & NS_RSQE_BUFASSIGN)
#define ns_rsqe_nzgfc(ns_rsqep) \
        (le32_to_cpu((ns_rsqep)->word_4) & NS_RSQE_NZGFC)
#define ns_rsqe_eopdu(ns_rsqep) \
        (le32_to_cpu((ns_rsqep)->word_4) & NS_RSQE_EOPDU)
#define ns_rsqe_cbuf(ns_rsqep) \
        (le32_to_cpu((ns_rsqep)->word_4) & NS_RSQE_CBUF)
#define ns_rsqe_congestion(ns_rsqep) \
        (le32_to_cpu((ns_rsqep)->word_4) & NS_RSQE_CONGESTION)
#define ns_rsqe_clp(ns_rsqep) \
        (le32_to_cpu((ns_rsqep)->word_4) & NS_RSQE_CLP)
#define ns_rsqe_crcerr(ns_rsqep) \
        (le32_to_cpu((ns_rsqep)->word_4) & NS_RSQE_CRCERR)

#define ns_rsqe_cellcount(ns_rsqep) \
        (le32_to_cpu((ns_rsqep)->word_4) & 0x000001FF)
#define ns_rsqe_init(ns_rsqep) \
        ((ns_rsqep)->word_4 = cpu_to_le32(0x00000000)) 

#define NS_RSQ_NUM_ENTRIES (NS_RSQSIZE / 16)
#define NS_RSQ_ALIGNMENT NS_RSQSIZE



/* RCQ - Raw Cell Queue
 *
 * Written by the NICStAR, read by the device driver.
 */

typedef struct cell_payload
{
   u32 word[12];
} cell_payload;

typedef struct ns_rcqe
{
   u32 word_1;
   u32 word_2;
   u32 word_3;
   u32 word_4;
   cell_payload payload;
} ns_rcqe;

#define NS_RCQE_SIZE 64         /* bytes */

#define ns_rcqe_islast(ns_rcqep) \
        (le32_to_cpu((ns_rcqep)->word_2) != 0x00000000)
#define ns_rcqe_cellheader(ns_rcqep) \
        (le32_to_cpu((ns_rcqep)->word_1))
#define ns_rcqe_nextbufhandle(ns_rcqep) \
        (le32_to_cpu((ns_rcqep)->word_2))



/* SCQ - Segmentation Channel Queue 
 *
 * Written by the device driver, read by the NICStAR.
 */

typedef struct ns_scqe
{
   u32 word_1;
   u32 word_2;
   u32 word_3;
   u32 word_4;
} ns_scqe;

   /* NOTE: SCQ entries can be either a TBD (Transmit Buffer Descriptors)
            or TSR (Transmit Status Requests) */

#define NS_SCQE_TYPE_TBD 0x00000000
#define NS_SCQE_TYPE_TSR 0x80000000

/* used for flags in ns_tbd_mkword_1 and ns_tbd_mkword_1_novbr*/
#define NS_TBD_EOPDU 0x40000000
#define NS_TBD_AAL0  0x00000000
#define NS_TBD_AAL34 0x04000000
#define NS_TBD_AAL5  0x08000000
#define NS_TBD_OAM   0x10000000
#define NS_TBD_TSIF  0x20000000
#define NS_TBD_GTSI  0x02000000

#define NS_TBD_VPI_MASK 0x0FF00000
#define NS_TBD_VCI_MASK 0x000FFFF0
#define NS_TBD_VC_MASK (NS_TBD_VPI_MASK | NS_TBD_VCI_MASK)

#define NS_TBD_VPI_SHIFT 20
#define NS_TBD_VCI_SHIFT 4

#define ns_tbd_mkword_1(flags, tsrtag, buflen) \
      (cpu_to_le32((flags) | tsrtag <<20 | (buflen)))

#define ns_tbd_mkword_3(control, pdulen) \
      (cpu_to_le32((control) << 16 | (pdulen)))

#define ns_tbd_mkword_4(gfc, vpi, vci, pt, clp) \
      (cpu_to_le32((gfc) << 28 | (vpi) << 20 | (vci) << 4 | (pt) << 1 | (clp)))



#define NS_TSR_TSIF        0x20000000

#define NS_TSR_SCDISUBR0 0xFFFF         /* Use as scdi for SCD reserved for UBR */
                                                                        /* unspecified pcr */

#define ns_tsr_mkword_1(flags) \
        (cpu_to_le32(NS_SCQE_TYPE_TSR | (flags)))

#define ns_tsr_mkword_2(scdi, scqi) \
        (cpu_to_le32((scdi) << 16 | 0x00008000 | (scqi)))

#define ns_scqe_is_tsr(ns_scqep) \
        (le32_to_cpu((ns_scqep)->word_1) & NS_SCQE_TYPE_TSR)


#define SCQ_NUM_ENTRIES 64  /* 1024/16 */
#define SCQ_SIZE 1024 
#define NS_SCQE_SIZE 16




/* TSQ - Transmit Status Queue
 *
 * Written by the NICSTAR, read by the device driver.
 */

typedef struct ns_tsi
{
   u32 word_1;
   u32 word_2;
} ns_tsi;

   /* NOTE: The first word can be a status word copied from the TSR which
            originated the TSI, or a timer overflow indicator. In this last
                case, the value of the first word is all zeroes. */

#define NS_TSI_EMPTY          0x80000000
#define NS_TSI_TIMESTAMP_MASK 0x00FFFFFF
#define NS_TSI_TYPE_MASK      0x60000000
#define TSR_TAG  (0x1F << 24)
#define NS_TSI_TYPE_TMROF     0x00000000
#define NS_TSI_TYPE_TSR       0x20000000
#define NS_TSI_TYPE_IDLE      0x40000000
#define NS_TSI_TYPE_TBD       0x60000000

#define ns_tsi_isempty(ns_tsip) \
        (le32_to_cpu((ns_tsip)->word_2) & NS_TSI_EMPTY)
#define ns_tsi_gettimestamp(ns_tsip) \
        (le32_to_cpu((ns_tsip)->word_2) & NS_TSI_TIMESTAMP_MASK)
#define ns_tsi_gettype(ns_tsip) \
        (le32_to_cpu((ns_tsip)->word_2) & NS_TSI_TYPE_MASK) 
#define ns_tsi_gettag(ns_tsip) \
        (le32_to_cpu((ns_tsip)->word_2) & NS_TSI_TAG_MASK) 

#define ns_tsi_init(ns_tsip) \
        ((ns_tsip)->word_2 = cpu_to_le32(NS_TSI_EMPTY))
        
#define NS_TSQSIZE 8192
#define NS_TSQ_NUM_ENTRIES 1024
#define NS_TSQ_ALIGNMENT 8192


#define NS_TSI_SCDISUBR0 NS_TSR_SCDISUBR0  /* i.e. 0xFFFF */

#define ns_tsi_getword_1(ns_tsip) \
        (le32_to_cpu((ns_tsip)->word_1))
#define ns_tsi_getword_2(ns_tsip) \
        (le32_to_cpu((ns_tsip)->word_2))        

/* timer overflow */
#define ns_tsi_tmrof(ns_tsip) \
        (le32_to_cpu((ns_tsip)->word_1) == 0x00000000)
        
/* if the TSI is due to a TSR,status word of the TSR is copied in 1st TSI word
 */
#define ns_tsi_getscdindex(ns_tsip) \
        ((le32_to_cpu((ns_tsip)->word_1) & 0xFFFF0000) >> 16)
        
#define ns_tsi_getscqpos(ns_tsip) \
        (le32_to_cpu((ns_tsip)->word_1) & 0x00007FFF)



/* NICSTAR structures located in local SRAM ***********************************/

/* TCT - Transmit Connection Table
 *
 * Written by both the NICStAR and the device driver.
 */
#define NS_TCT_ENTRY_SIZE      8

/* word 1 of TCTE */   
#define NS_TCTE_TYPE_MASK      0xC0000000
#define NS_TCTE_TYPE_CBR       0x00000000
#define NS_TCTE_TYPE_ABR       0x80000000
#define NS_TCTE_TYPE_VBR       0x40000000
#define NS_TCTE_TYPE_UBR       0x00000000
/* word 3 */
#define NS_TCTE_TSIF               (1<<14)
/* word 4 of TCTE */
#define NS_TCTE_HALT           0x80000000  
#define NS_TCTE_IDLE           0x40000000
/* word 8 of UBR or CBR TCTE */
#define NS_TCTE_UBR_EN         0x80000000  

    
/* RCT - Receive Connection Table
 *
 * Written by both the NICStAR and the device driver.
 */

typedef struct ns_rcte
{
   u32 word_1;
   u32 buffer_handle;
   u32 dma_address;
   u32 aal5_crc32;
} ns_rcte;


#define NS_RCTE_INACTLIM        0xE0000000
#define NS_RCTE_INACTCOUNT      0x1C000000
#define NS_RCTE_CIVC            0x00800000
#define NS_RCTE_FBP             0x00600000
/*#define NS_RCTE_BSFB            0x00200000 */ /* Rev. D only */
#define NS_RCTE_NZGFC           0x00100000
#define NS_RCTE_CONNECTOPEN     0x00080000
#define NS_RCTE_AALMASK         0x00070000
#define NS_RCTE_AAL0            0x00000000
#define NS_RCTE_AAL34           0x00010000
#define NS_RCTE_AAL5            0x00020000
#define NS_RCTE_RCQ             0x00030000
#define NS_RCTE_OAM             0x00040000
#define NS_RCTE_RAWCELLINTEN    0x00008000
#define NS_RCTE_RXCONSTCELLADDR 0x00004000
#define NS_RCTE_BUFSTAT         0x00003000
#define NS_RCTE_EFCI            0x00000800
#define NS_RCTE_CLP             0x00000400
#define NS_RCTE_CRCERROR        0x00000200
#define NS_RCTE_CELLCOUNT_MASK  0x000001FF

#define NS_RCT_ENTRY_SIZE 4     /* Number of dwords */

   /* NOTE: We could make macros to contruct the first word of the RCTE,
            but that doesn't seem to make much sense... */



/* FBD - Free Buffer Descriptor
 *
 * Written by the device driver using via the command register.
 */

typedef struct ns_fbd
{
   u32 buffer_handle;
   u32 dma_address;
} ns_fbd;


/* TST - Transmit Schedule Table
 *
 * Written by the device driver.
 */

typedef u32 ns_tste;

#define NS_TST_OPCODE_MASK 0x60000000

#define NS_TST_OPCODE_NULL     0x00000000 /* Insert null cell */
#define NS_TST_OPCODE_FIXED    0x20000000 /* Cell from a fixed rate channel */
#define NS_TST_OPCODE_VARIABLE 0x40000000 /* Cell from variable rate channel */
#define NS_TST_OPCODE_END      0x60000000 /* Jump */

#define ns_tste_make(opcode, sramad) (opcode | sramad)

   /* NOTE:

      - When the opcode is FIXED, sramad specifies the connection number used to
      index the TCT.
      - When the opcode is END, sramad specifies the SRAM address in bytes of the
        location of the next TST entry to read.
    */



/* SCD - Segmentation Channel Descriptor
 *
 * Written by both the device driver and the NICStAR
 */

typedef struct ns_scd
{
   u32 word_1;
   u32 word_2;
   u32 partial_aal5_crc;
   u32 reserved;
   ns_scqe cache_a;
   ns_scqe cache_b;
} ns_scd;

#define NS_SCD_ENTRY_SIZE 12

/* ABR NICStAR local SRAM memory map
 **********************************************/


/* TCT */
/* size set by CFG reg(b16-17)  8-words entries */

/* RCT */
/* size set by CFG reg(b16-17)  4-words entries */

/* RFBQ */  
#define NS_RFBQ_SIZE     0x400
 
/* DTST size set by ABRSTD (2x2Kword-tables) (and b0-11 null) */
#define NS_ABRVBRSIZE   (0x2<<24)
#define NS_DTST_SIZE 4096
  
/* TST */
#define NS_TST_SIZE_MAX         4096
#define NS_TST_RESERVED         340             /* entries reserved for UBR/ABR/VBR */

/* SCDs */
/* 12-word entries */
#define NS_SCD_NUM               250  /* max number of SCDs */

/* RXFIFO size set by RXFD (4K words) (and b0-13 null) */
#define NS_RXFIFO_SIZE          0x1000
#define NS_RXFD_SIZE            (3<<24)


/* NISCtAR operation registers ************************************************/


enum ns_regs
{
   DR0   = 0x00,      /* Data Register 0 R/W*/
   DR1   = 0x04,      /* Data Register 1 W */
   DR2   = 0x08,      /* Data Register 2 W */
   DR3   = 0x0C,      /* Data Register 3 W */
   CMD   = 0x10,      /* Command W */
   CFG   = 0x14,      /* Configuration R/W */
   STAT  = 0x18,      /* Status R/W */
   RSQB  = 0x1C,      /* Receive Status Queue Base W */
   RSQT  = 0x20,      /* Receive Status Queue Tail R */
   RSQH  = 0x24,      /* Receive Status Queue Head W */
   CDC   = 0x28,      /* Cell Drop Counter R/clear */
   VPEC  = 0x2C,      /* VPI/VCI Lookup Error Count R/clear */
   ICC   = 0x30,      /* Invalid Cell Count R/clear */
   RAWCT = 0x34,      /* Raw Cell Tail R */
   TMR   = 0x38,      /* Timer R */
   TSTB  = 0x3C,      /* Transmit Schedule Table Base R/W */
   TSQB  = 0x40,      /* Transmit Status Queue Base W */
   TSQT  = 0x44,      /* Transmit Status Queue Tail R */
   TSQH  = 0x48,      /* Transmit Status Queue Head W */
   GP    = 0x4C,      /* General Purpose R/W */
   VPM   = 0x50,       /* VPI/VCI Mask W */
   RXFD  = 0x54,      /* Receive FIFO descriptor */ 
   RXFT  = 0x58,      /* Receive FIFO Tail */ 
   RXFH  = 0x5C,      /* Receive FIFO  Head */
   RAWHND= 0x60,      /* Raw Cell Handle register */ 
   RXSTAT= 0x64,      /* Receive Conn. State register */ 
   ABRSTD= 0x68,      /* ABR Schedule TableDescriptor */
   ABRRQ = 0x6C,      /* ABR Ready Queue Pointer register */
   VBRRQ = 0x70,      /* VBR Ready Queue Pointer register */
   RTBL  = 0x74,      /* Rate Table Descriptor */
   MXDFCT= 0x78,      /* Maximum Deficit Count register */
   TXSTAT= 0x7C,      /* Transmit Conn. State register */
   TCMDQ = 0x80,      /* Transmit Command Queue register */
   IRCP  = 0x84,      /* Inactive Receive Conn. Pointer */
   FBQP0 = 0x88,      /* Free Buffer Queue Pointer Register 0 */
   FBQP1 = 0x8C,      /* Free Buffer Queue Pointer Register 1 */
   FBQP2 = 0x90,      /* Free Buffer Queue Pointer Register 2 */
   FBQP3 = 0x94,      /* Free Buffer Queue Pointer Register 3 */
   FBQS0 = 0x98,      /* Free Buffer Queue Size Register 0 */
   FBQS1 = 0x9C,      /* Free Buffer Queue Size Register 1 */
   FBQS2 = 0xA0,      /* Free Buffer Queue Size Register 2 */
   FBQS3 = 0xA4,      /* Free Buffer Queue Size Register 3 */
   FBQWP0= 0xA8,      /* Free Buffer Queue Write Pointer Register 0 */
   FBQWP1= 0xAC,      /* Free Buffer Queue Write Pointer Register 1 */
   FBQWP2= 0xB0,      /* Free Buffer Queue Write Pointer Register 2 */
   FBQWP3= 0xB4,      /* Free Buffer Queue Write Pointer Register 3 */
   NOW   = 0xB8       /* Current Transmit Schedule Table Address */
};


/* NICStAR commands issued to the CMD register ********************************/


/* Top 4 bits are command opcode, lower 28 are parameters. */

#define NS_CMD_NO_OPERATION         0x00000000
        /* params always 0 */

#define NS_CMD_OPENCLOSE_CONNECTION 0x20000000
        /* b19{1=open,0=close} b18-2{SRAM addr} */

#define NS_CMD_WRITE_SRAM           0x40000000
        /* b18-2{SRAM addr} b1-0{burst size} */

#define NS_CMD_READ_SRAM            0x50000000
        /* b18-2{SRAM addr} */

#define NS_CMD_WRITE_FREEBUFQ       0x60000000
        /* b0{large buf indicator} */

#define NS_CMD_READ_UTILITY         0x80000000
        /* b8{1=select UTL_CS0} b9{1=select UTL_CS1} b7-0{bus addr} */

#define NS_CMD_WRITE_UTILITY        0x90000000
        /* b8{1=select UTL_CS0} b9{1=select UTL_CS1} b7-0{bus addr} */

#define NS_CMD_OPEN_CONNECTION (NS_CMD_OPENCLOSE_CONNECTION | 0x00080000)
#define NS_CMD_CLOSE_CONNECTION NS_CMD_OPENCLOSE_CONNECTION


/* NICStAR configuration bits *************************************************/

#define NS_CFG_SWRST          0x80000000    /* Software Reset */
#define NS_CFG_LOOP           0x40000000    /* Enable internal loop back */
#define NS_CFG_RXPATH         0x20000000    /* Receive Path Enable */
#define NS_CFG_IDLECLP        0x10000000    /* CLP bit of Null Cells */
#define NS_CFG_TFIFOSIZE_MASK 0x0C000000    /* Specifies size of tx FIFO */
#define NS_CFG_RSQSIZE_MASK   0x00C00000    /* Receive Status Queue Size */
#define NS_CFG_ICACCEPT       0x00200000    /* Invalid Cell Accept */
#define NS_CFG_IGNOREGFC      0x00100000    /* Ignore General Flow Control */
#define NS_CFG_VPIBITS_MASK   0x000C0000    /* VPI/VCI Bits Size Select */
#define NS_CFG_RCTSIZE_MASK   0x00030000    /* Receive Connection Table Size */
#define NS_CFG_VCERRACCEPT    0x00008000    /* VPI/VCI Error Cell Accept */
#define NS_CFG_RXINT_MASK     0x00007000    /* End of Receive PDU Interrupt
                                               Handling */
#define NS_CFG_RAWIE          0x00000800    /* Raw Cell Qu' Interrupt Enable */
#define NS_CFG_RSQAFIE        0x00000400    /* Receive Queue Almost Full
                                               Interrupt Enable */
#define NS_CFG_RXRM           0x00000200    /* Receive RM Cells */
#define NS_CFG_CACHE          0x00000100    /* Cache */
#define NS_CFG_TMRROIE        0x00000080    /* Timer Roll Over Interrupt
                                               Enable */
#define NS_CFG_FBIE           0x00000040    /* Free Buffer Queue Int Enable */          
                               

#define NS_CFG_TXEN           0x00000020    /* Transmit Operation Enable */
#define NS_CFG_TXIE           0x00000010    /* Transmit Status Interrupt
                                               Enable */
#define NS_CFG_TXURIE         0x00000008    /* Transmit Under-run Interrupt
                                               Enable */
#define NS_CFG_UMODE          0x00000004    /* Utopia Mode (cell/byte) Select */
#define NS_CFG_TSQFIE         0x00000002    /* Transmit Status Queue Full
                                               Interrupt Enable */
#define NS_CFG_PHYIE          0x00000001    /* PHY Interrupt Enable */

#define NS_CFG_RSQSIZE_2048 0x00000000
#define NS_CFG_RSQSIZE_4096 0x00400000
#define NS_CFG_RSQSIZE_8192 0x00800000

#define NS_CFG_VPIBITS_0 0x00000000
#define NS_CFG_VPIBITS_1 0x00040000
#define NS_CFG_VPIBITS_2 0x00080000
#define NS_CFG_VPIBITS_8 0x000C0000

#define NS_CFG_RCTSIZE_1024_ENTRIES  0x00000000   /* the one used */
#define NS_CFG_RCTSIZE_4096_ENTRIES  0x00010000
#define NS_CFG_RCTSIZE_16384_ENTRIES 0x00020000
#define NS_CFG_RCTSIZE_512_ENTRIES   0x00030000

#define NS_CFG_RXINT_NOINT   0x00000000
#define NS_CFG_RXINT_NODELAY 0x00001000
#define NS_CFG_RXINT_2800CLK 0x00002000
#define NS_CFG_RXINT_4F00CLK 0x00003000
#define NS_CFG_RXINT_7400CLK 0x00004000


/* Configuration Register */
#ifdef  NS_RCQ_SUPPORT
#define NS_DEFAULT_CFG  ( \
            NS_CFG_RXPATH | \
                        NS_CFG_FBIE | \
                        NS_CFG_RSQSIZE | \
                        NS_CFG_RXINT_NODELAY | \
                        NS_CFG_ICACCEPT | \
                        NS_CFG_RAWIE | \
                        NS_CFG_IGNOREGFC | \
                        NS_CFG_VCERRACCEPT | \
                        NS_CFG_RSQAFIE | \
                        NS_CFG_TMRROIE  | \
                        NS_CFG_TXEN | \
                        NS_CFG_TXIE | \
                        NS_CFG_TSQFIE | \
                        NS_CFG_PHYIE )
#else
#define NS_DEFAULT_CFG  ( \
            NS_CFG_RXPATH | \
                        NS_CFG_FBIE | \
                        NS_CFG_RSQSIZE | \
                        NS_CFG_RXINT_NODELAY | \
                        NS_CFG_RSQAFIE | \
                        NS_CFG_TMRROIE  | \
                        NS_CFG_TXEN | \
                        NS_CFG_TXIE | \
                        NS_CFG_TSQFIE | \
                        NS_CFG_PHYIE )
#endif


/* NICStAR STATUS bits ********************************************************/

#define NS_STAT_FRAC3_MASK 0xF0000000
#define NS_STAT_FRAC2_MASK 0x0F000000
#define NS_STAT_FRAC1_MASK 0x00F00000
#define NS_STAT_FRAC0_MASK 0x000F0000
#define NS_STAT_TSIF       0x00008000   /* Transmit Status Queue Indicator */
#define NS_STAT_TXICP      0x00004000   /* Transmit Incomplete PDU */
#define NS_STAT_TSQF       0x00001000   /* Transmit Status Queue Full */
#define NS_STAT_TMROF      0x00000800   /* Timer Overflow */
#define NS_STAT_PHYI       0x00000400   /* PHY Device Interrupt */
#define NS_STAT_CMDBZ      0x00000200   /* Command Busy */
#define NS_STAT_FBQ3A      0x00000100   /* Free Buffer Queue Attention */
#define NS_STAT_FBQ2A      0x00000080   /* Free Buffer Queue Attention */
#define NS_STAT_RSQF       0x00000040   /* Receive Status Queue Full */
#define NS_STAT_EOPDU      0x00000020   /* End of PDU */
#define NS_STAT_RAWCF      0x00000010   /* Raw Cell Flag */
#define NS_STAT_FBQ1A      0x00000008   /* Free Buffer Queue Attention */
#define NS_STAT_FBQ0A      0x00000004   /* Free Buffer Queue Attention */
#define NS_STAT_RSQAF      0x00000002   /* Receive Status Queue Almost Full */

/* NICStAR TCMDQ Register *****************************************************/

#define NS_START                        0x01000000 
#define NS_UPD_LACR             0x02000000
#define NS_START_UPD_LACR       0x03000000
#define NS_UPD_INIT_ER          0x04000000
#define NS_HALT                         0x05000000

/* GP Register Bits *************************************************/
#define NS_GP_EECLK             (1<<2)
#define NS_GP_EECS      (1<<1)
#define NS_GP_EEDO      1
#define NS_GP_EEDI      (1<<16)
                                                           
/* NICSTAR free buf. queues ***************************************************/

/* registers #34-#37 */
#define NS_FBQPR_MASK      0x000007FF 
#define NS_FBQPW_MASK      0x07FF0000

#define diff(fbqp)  (int) ((int)((((fbqp) & NS_FBQPW_MASK)>>16) - ((fbqp) & NS_FBQPR_MASK))/2)
#define ns_fbqc_get(fbqp)  (diff(fbqp) >= 0 ? diff(fbqp) : 0x400 + diff(fbqp))

/* #defines which depend on other #defines ************************************/

#if (NS_RSQSIZE == 2048)
#define NS_CFG_RSQSIZE NS_CFG_RSQSIZE_2048
#elif (NS_RSQSIZE == 4096)
#define NS_CFG_RSQSIZE NS_CFG_RSQSIZE_4096
#elif (NS_RSQSIZE == 8192)
#define NS_CFG_RSQSIZE NS_CFG_RSQSIZE_8192
#else
#error NS_RSQSIZE is incorrect in nicstar.h
#endif /* NS_RSQSIZE */


#ifdef RCQ_SUPPORT
#define NS_CFG_RAWIE_OPT NS_CFG_RAWIE
#else
#define NS_CFG_RAWIE_OPT 0x00000000
#endif /* RCQ_SUPPORT */

/* PCI stuff ******************************************************************/

#ifndef PCI_VENDOR_ID_IDT
#define PCI_VENDOR_ID_IDT 0x111D
#endif /* PCI_VENDOR_ID_IDT */

#ifndef PCI_DEVICE_ID_IDT_IDT77252
#define PCI_DEVICE_ID_IDT_IDT77252 0x0003
#endif


/* Device driver structures ***************************************************/


typedef struct tsq_info
{
   void *org;
   ns_tsi *base;
   ns_tsi *next;
} tsq_info;


typedef struct scq_info
{
   void *org;
   ns_scqe *base;
   ns_scqe *last;
   ns_scqe *next;
   volatile ns_scqe *tail;              /* Not related to the nicstar register */
   int scq_type;                        /* CBR_SCQ, UBR_SCQ, VBR_SCQ or ABR_SCQ */
   struct sk_buff **skb;                /* Pointer to an array of pointers to the sk_buffs used for tx */
   struct sk_buff_head  waiting;        /* head of queue of waiting sk_buffs */
   u32 scd;                             /* SRAM address of the corresponding SCD */
   int tbd_count;                       /* Only meaningful on variable rate */
   spinlock_t lock;                     /* SCQ spinlock */
#ifdef NS_DEBUG_SPINLOCKS
   volatile long has_lock;
   volatile int cpu_lock;
#endif /* NS_DEBUG_SPINLOCKS */
} scq_info;


#define UBR_SCQ  1
#define CBR_SCQ  2
#define VBR_SCQ  3
#define ABR_SCQ  4

typedef struct rsq_info
{
   void *org;
   ns_rsqe *base;
   ns_rsqe *next;
   ns_rsqe *last;
} rsq_info;


typedef struct skb_pool
{
   volatile int count;                  /* number of buffers in the queue */
   struct sk_buff_head queue;
} skb_pool;

/* NOTE: for small and large buffer pools, the count is not used, as the
         actual value used for buffer management is the one read from the
         card. */


typedef struct vc_map
{
   int  conn;                                   /* connection number */
   struct atm_vcc *tx_vcc, *rx_vcc;
   struct sk_buff *rx_iov;              /* RX iovector skb */
   scq_info *scq;                               /* To keep track of the SCQ */
   u32 adr_scd;                                 /* SRAM address of the corresponding
                                                                SCD. 0x00000000 for UBR only */
   int tbd_count;
   u8   lacr;
   u8   init_er;
   volatile u8 tx:1;                    /* TX vc? */
   volatile u8 rx:1;                    /* RX vc? */
   volatile u8 active:1;                /* the connection is active */
   u8   pad;
} vc_map;

struct ns_skb_data
{
        struct atm_vcc *vcc;
        int iovcnt;
};

#define NS_SKB(skb) (((struct ns_skb_data *) (skb)->cb))

typedef struct ns_dev
{
   int index;                                           /* Card ID to the device driver */
   char name[16];                                       /* card name */
   int sram_size;                                       /* In k x 32bit words. 32 or 128 */
   unsigned long membase;                       /* Card's memory base address */
   unsigned long max_pcr;
   int rct_size;                                        /* Number of entries */
   int scd_size;
   int tst_num;                                 
   int vpibits;
   int vcibits;
   u32 maxvpi;
   u32 maxvci;
   u32 vpm;                                                     /* MASK register value */
   u32 rct;                                             /* SRAM address of RCT */
   u32 rate;                                            /* SRAM address of Rate Table */
   u32 tst;                                             /* SRAM address of TST */
   u32 dtst;                                            /* SRAM address of ABR-VBR TST (DTST)*/
   u32 scd;                                             /* SRAM address of SCDs */
   u32 scd_ubr0;                                        /* SRAM address of no pcr SCD */
   u32 rxfifo;                                          /* SRAM address of receive fifo */
   struct pci_dev *pcidev;
   struct atm_dev *atmdev;
   tsq_info tsq;
   rsq_info rsq;
   scq_info *scq0;                                      /* UBR SCQ for unspecified PCR */
   skb_pool b0pool;                                     /* Small buffers */
   skb_pool b1pool;                                     /* Large buffers */
   skb_pool hbpool;                                     /* Pre-allocated huge buffers */
   skb_pool iovpool;                            /* iovector buffers */
   volatile int fbie;                           /*  free buf. queue int. enabled */
   volatile int tst_free_entries;
   vc_map *vcmap;                                       /* table of VC descriptors*/
   vc_map *tste2vc[NS_TST_SIZE_MAX];
   vc_map *scd2vc[NS_SCD_NUM];
   buf_nr b0nr;
   buf_nr b1nr;
   buf_nr hbnr;
   buf_nr iovnr;
   int sbfqc;
   int lbfqc;
   u32 b0_handle;
   u32 b0_addr;
   u32 b1_handle;
   u32 b1_addr;
   struct sk_buff *rcbuf;               /* Current raw cell buffer */
   ns_rcqe *raw_ch;                             /* Raw cell queue head */
   u32 raw_hnd[2];                              /* raw cell queue tail pointer and rawcell handle*/
   unsigned intcnt;                             /* Interrupt counter */
   spinlock_t int_lock;                 /* Interrupt lock */
   spinlock_t res_lock;                 /* Card resource lock */
#ifdef NS_DEBUG_SPINLOCKS
   volatile long has_int_lock;
   volatile int cpu_int;
   volatile long has_res_lock;
   volatile int cpu_res;
#endif /* NS_DEBUG_SPINLOCKS */

} ns_dev;


   /* NOTE: Each tste2vc entry relates a given TST entry to the corresponding
            CBR vc. If the entry is not allocated, it must be NULL.
            
            There are two TSTs so the driver can modify them on the fly
            without stopping the transmission.
            
            scd2vc allows us to find out unused fixed rate SCDs, because
            they must have a NULL pointer here. */


#endif /* _LINUX_NICSTAR_H_ */
