/************************************************************************
 *
 * Copyright (c) 2004
 * Infineon Technologies AG
 * St. Martin Strasse 53; 81669 Muenchen; Germany
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 ************************************************************************/
 
/******************************************************************************
   File        : $RCSfile: DANUBE_VCPU_DeviceDriver_Ext.h,v $
   Revision    : $Revision: 1.1 $
   Date        : $Date: 2005/08/30 10:42:55 $
   Description :

******************************************************************************/
#ifndef _DANUBEVCPUDEVICEDRIVEREXT_h
#define _DANUBEVCPUDEVICEDRIVEREXT_h

/*---------------------------------------------------------------------------*/
/* Structure for communication with OAK MailBox driver functions. */
/*---------------------------------------------------------------------------*/
#if 0
typedef struct
{
   /* Pointer to OAK Mailbox write source */
   u32  *pWrite;              
   /* Pointer to OAK Mailbox read destination */
   u32  *pRead;              
   /* Number of bytes to write */
   u32   nWrite;              
   /* Number of bytes to read */
   u32   nRead;               
   /* Number of max. Messages and gets actual num message read */
   u32   nMsgRead;               
   /* Event received if not zero, don't care otherwise */
   u32   event;
                  
}OAK_READWRITE;
#endif

typedef struct
{
   u32  *pData;    /**< Pointer to data location in SDRAM to be passed to other CPU */
   u32  nDataBytes; /**< Amount of valid data bytes in SDRAM starting at pData */ 
   u32  RTP_PaylOffset;
} DSP_READWRITE;



typedef struct 
{
   unsigned char rw:1;
   unsigned char res1:4;
   unsigned char ptype:3;
   unsigned char res2:7;
   unsigned char chan:1;
   unsigned char odd:2;
   unsigned char res4:5;
   unsigned char length;
   unsigned long timestamp;	
   unsigned char ptd;
   unsigned char cp:1;
   unsigned char res5:2;
   unsigned char sid:1;
   unsigned char dec:4;
   unsigned int res6;
   u32* usa_payload_data;
}DSPPACKET;



/* Access mode */
typedef enum 
{
   RUNTIME_OAKMEM_ACCESS,
   BOOTTIME_OAKMEM_ACCESS
}MEMACCS_TYPE ; 


/* Run-time Memory access type (Set address "Command Packet" ) */
typedef enum 
{
    STATIC_MEM_ACCESS,
    CONTINUOS_MEM_ACCESS
}RUNACCS_TYPE ;

/* Run time memory access structure */
typedef struct
{
   u16         oakAddress ; 
   u16      bankAddr ;  
   RUNACCS_TYPE   type ;
}OAK_RUNTIME_MEMACCESS ;

/* pad the lower word to zero so as to add the 16 bit half word to header of
 setting the access type */
typedef enum 
{
   ACCESS_OAK_DRAM = 0x10000,
   ACCESS_OAK_PRAM = 0x20000
}BOOTACCS_TYPE ; 

/* Boot time memory access structure */
typedef struct
{
   u16         oakAddress_1 ; 
   u16         oakAddress_2 ;
   u16         bankAddr ;      
   BOOTACCS_TYPE  type ;    
}OAK_BOOTTIME_MEMACCESS ;

/* OAK access modes */
typedef enum 
{
   OAKWRITE,
   OAKREAD
}OAK_ACCESS_MODE ;

/* Mailbox access modes */
/* 
typedef enum 
{
   DOWNSTREAM, 
   UPSTREAM
}MAILBOX_DIRECTION ;
*/


/* Register access type */
typedef enum 
{
    REG_READ,
    REG_WRITE,
    REG_MODIFY
}REGISTER_ACCESS ;

/* Register access format */
typedef struct 
{
    u16        regOffset ;
   REGISTER_ACCESS accessType ;
   u32         data ;
}REG_ACS_FMT ;

/* OAK memory access structure */
/*
typedef struct 
{
   u32           *pData ;
   u32           Length ;
   OAK_ACCESS_MODE  rw ;
   MEMACCS_TYPE     typeofAccess ;
   u32           maxPacketSize ;
   UINT8            ctype  ;
   UINT8            cmd[3] ;
   DSP_READWRITE    readWrite ;
   union tagMemoryAccessType
   {
      OAK_RUNTIME_MEMACCESS  runmem ;
      OAK_BOOTTIME_MEMACCESS bootmem ;
   }MemoryAccessType;

   u16    numMsg ;
}DANUBE_MEMACC;
*/
/*---------------------------------------------------------------------------*/
/* Structure for communication with OAK driver functions.                    */
/*---------------------------------------------------------------------------*/
typedef struct
{
   u32   *pData;
   u32   Length;
   
}VCPU_FWDWNLD;

typedef struct
{
    unsigned int    file_type ; /* code(PRAM) or data(DRAM) */
    unsigned int    total_size ; /* Total downloadable data length (in
    bytes) */
    unsigned int    size_desc ; /* Length of the description string
    (strlen +1,should be divisible by four )*/
    unsigned int    crc16 ;  /* file crc value */
    unsigned char   version[24]; /* version string */
    unsigned int    num_seg ;  /* Number of segments in the file */
}BinaryFileHeader;

typedef struct
{
    unsigned int start_address ;/* start address */
    unsigned int end_address ;/* end address (Note: This is the last address at
     which data is written)*/
    unsigned int length ; /* in halfwords */
    unsigned int bank ;   /* bank address */
    unsigned int crc16 ; /* block crc value */
    unsigned int padding_bit ; /* Was the binary block padded*/
}BinaryFileBlock;


#ifdef NEVER
/* Declaration for 32 bit access for the Command word headers */
#define CMD_PTYPE32(d)   (d<<8)
#define CMD_RW32(d)      (d<<15)
#define CMD_LENGTH32(d)  (d<<16)
#define CMD_CMD32(d)     (d<<24)
#define CMD_CTYPE32(d)   (d<<29)

#define VOICE_PTYPE32(d)   (d<<8)
#define VOICE_RW32(d)      (d<<15)
#define VOICE_LENGTH32(d)  (d<<16)

#define VOICE_CODEC32(d)   (d<<0)
#define VOICE_SID32(d)     (d<<4)
#define VOICE_PT32(d)      (d<<8)
#endif /* NEVER */


/* #define NUM_VOICE_CHANNEL     4  */

/* Maximum num of Messages that can be read/write from/to the FIFO at a time */
#define MAXVOICEMSGNUM           10
#define MAXCMDMSGNUM             10

/* Queue size in 32 bit values */
#define MAX_FIFO_CMD     64 
#define MAX_FIFO_VOICE   (4096 / 4)

/* Maximum number of OAK memory bytes (see firmware specs )that can be accessed
 at Run time (Note should be divisible by 4) . The same can be configured only
 for memory accesses at boot time.Set to Optimum size */
#define MAX_SIZE_CMD_MEMACC_PACKET     60  

/* maximum command buffer size (in bytes) , used in memory */
#define MAX_SIZE_COMMAND_MB   256 

/* Length in the packet field ( 16 bit data) */
#define MAX_VOICE_LEN   32             
#define MIN_VOICE_LEN 4
#define MAX_CMD_LEN   30     

#define PACKET_TYPE_CMD  6
#define PACKET_TYPE_VOICE 4

/* in bytes */
#define MAX_VOICE_PACKET_SIZE  ((MAX_VOICE_LEN<<1) + 4)
#define MAX_COMMAND_PACKET_SIZE  ((MAX_CMD_LEN<<1) + 4)


/* Declaration for device driver errno values */
#define S_iosLib_OAKMB              (259 << 16)

/* unknown device name string during open process */
#define S_iosLib_OAKMB_UNK_DEV_NAME (S_iosLib_OAKMB + 1)
/* Unknown case for IoCtl() */
#define S_iosLib_OAKMB_UNK_IOCTL    (S_iosLib_OAKMB + 2)
/* Init Driver Failed */
#define S_iosLib_OAKMB_INIT_DRV     (S_iosLib_OAKMB + 3)
/* Dev already opened/ installed*/
#define S_iosLib_OAKMB_DEV_INST     (S_iosLib_OAKMB + 4)
/* Packet type and destination FIFO mismatch*/
#define S_iosLib_OAKMB_FIFO_PKTMC   (S_iosLib_OAKMB + 5)
/* FIFO is empty (No packet available for read)*/
#define S_iosLib_OAKMB_FIFO_DAI     (S_iosLib_OAKMB + 6)
/* FIFO has insufficient space for the packet to be written*/
#define S_iosLib_OAKMB_FIFO_WRI     (S_iosLib_OAKMB + 7)
/* FIFO corrupted*/
#define S_iosLib_OAKMB_FIFO_SZEMC   (S_iosLib_OAKMB + 8)
/* Packet type and destination mailbox mismatch*/
#define S_iosLib_OAKMB_MB_PKTMC     (S_iosLib_OAKMB + 9)
/* Mailbox is empty (No packet available for read)*/
#define S_iosLib_OAKMB_MB_DAI       (S_iosLib_OAKMB + 10)
/* Mailbox has insufficient space for packet to be written*/
#define S_iosLib_OAKMB_MB_WRI       (S_iosLib_OAKMB + 11)
/* Mailbox corrupted */
#define S_iosLib_OAKMB_MB_SZEMC     (S_iosLib_OAKMB + 12)
/* Mailbox write aborted because of memory corruption in packet header */
#define S_iosLib_OAKMB_MB_MEM_CRPT  (S_iosLib_OAKMB + 13)
/* message are not 32-bit align in the length */
#define S_iosLib_OAKMB_MSG_DWORD    (S_iosLib_OAKMB + 13)


#if 0
/*---------------------------------------------------------------------------*/
/* Exported Functions                                                        */
/*---------------------------------------------------------------------------*/
s32 DANUBE_MPS_DSP_CommonOpen(MPS_COMM_DEV *pDev, MPS_MB_DEV_STR *pMBDev, BOOL bcommand);
s32 DANUBE_MPS_DSP_CommonClose(MPS_MB_DEV_STR *pDev);
s32  DANUBE_MPS_MbxReadUpstrmFifo( MPS_MB_DEV_STR  *pMBDev, 
                                    DSP_READWRITE   *pPkg, 
                                    s32             timeout);
s32 DANUBE_MPS_MbxWriteDwnstrCmd( register MPS_MB_DEV_STR *pMBDev, 
                                   DSP_READWRITE *readWrite );
s32 DANUBE_MPS_MbxWriteDwnstrData( register MPS_MB_DEV_STR *pMBDev, 
                                    DSP_READWRITE *readWrite );
u32 DANUBE_MPS_InitStructures(MPS_COMM_DEV *pDev);
void DANUBE_MPS_DSP_ReleaseStructures(MPS_COMM_DEV *pDev);   

void DANUBE_MPS_DSP_MbxClearFifo(MailboxParam_s *pFifo);
s8 DANUBE_MPS_MbxFifoMemAvailable( MailboxParam_s *mbx );
s32 DANUBE_MPS_DSP_Restart(void);
s32 DANUBE_MPS_DSP_Reset(void);
s32 DANUBE_MPS_VCPU_DownloadFirmware(MPS_MB_DEV_STR *pDev, VCPU_FWDWNLD *pFWDwnld);
#endif
/*
u32 DANUBE_MPS_MbxFifoMemAvailable(MailboxParam_s *pFifo);
int oak_write_fifo(MPS_MB_DEV_STR* pDev, DSP_READWRITE *pPkg);


u32 oak_init_structures(MPS_COMM_DEV *pDev);
int oak_write_cmd_mailbox(register MPS_MB_DEV_STR *pDev, DSP_READWRITE *readWrite );
int oak_write_voice_mailbox(register MPS_MB_DEV_STR *pDev, DSP_READWRITE *readWrite );


*/
#endif /* _DANUBEVCPUDEVICEDRIVEREXT_h */

