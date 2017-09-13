/*******************************************************************************
                  Copyright (c) 2006  Infineon Technologies AG
                    Am Campeon 1-12; 81726 Munich, Germany

   THE DELIVERY OF THIS SOFTWARE AS WELL AS THE HEREBY GRANTED NON-EXCLUSIVE,
   WORLDWIDE LICENSE TO USE, COPY, MODIFY, DISTRIBUTE AND SUBLICENSE THIS
   SOFTWARE IS FREE OF CHARGE.

   THE LICENSED SOFTWARE IS PROVIDED "AS IS" AND INFINEON EXPRESSLY DISCLAIMS
   ALL REPRESENTATIONS AND WARRANTIES, WHETHER EXPRESS OR IMPLIED, INCLUDING
   WITHOUT LIMITATION, WARRANTIES OR REPRESENTATIONS OF WORKMANSHIP,
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, DURABILITY, THAT THE
   OPERATING OF THE LICENSED SOFTWARE WILL BE ERROR FREE OR FREE OF ANY
   THIRD PARTY CLAIMS, INCLUDING WITHOUT LIMITATION CLAIMS OF THIRD PARTY
   INTELLECTUAL PROPERTY INFRINGEMENT.

   EXCEPT FOR ANY LIABILITY DUE TO WILFUL ACTS OR GROSS NEGLIGENCE AND
   EXCEPT FOR ANY PERSONAL INJURY INFINEON SHALL IN NO EVENT BE LIABLE FOR
   ANY CLAIM OR DAMAGES OF ANY KIND, WHETHER IN AN ACTION OF CONTRACT,
   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*******************************************************************************
   Module      : common.c
   Date        : 2005-10-01
   Desription  : Common stuff like init of board, chip, pcm, getting file
                 descriptor of phone, data, pcm channel.
******************************************************************************/

/* ============================= */
/* Includes                      */
/* ============================= */
#include "common.h"

/** Default FW file includes, to be provided */
#ifdef VIN_2CPE

#include "2cpe_600.c"
#include "edspdramfw_rtp_0_16_234_v21.c"
#include "edsppramfw_rtp_0_16_234_v21.c"

#else /* VIN_2CPE */

#ifndef VMMC
#include "CRAM_vip.c"
#include "DRAMfw_vip.c"
#include "PRAMfw_vip.c"
#endif /* VMMC */

#endif /* VIN_2CPE */

/* ============================= */
/* Defines                       */
/* ============================= */
#ifdef VMMC
#include "lib_vmmc.h"
/** Vmmc config device */

const char* IFX_CTRL_DEV = "/dev/vmmc10";
/* FIXME: IOCTL should use common name. */
#define FIO_VINETIC_VERS FIO_GET_VERS
#define FIO_VINETIC_RCMD FIO_READ_CMD
#define FIO_VINETIC_WCMD FIO_WRITE_CMD
#define FIO_VINETIC_RDREG FIO_RDREG
#define FIO_VINETIC_WRREG FIO_WRREG

#else
/** Vinetic config device */
const char* IFX_CTRL_DEV = "/dev/vin10";
#endif /* VMMC */

#ifdef FXO
const char* IFX_DUS_CTRL_DEV = "/dev/dus10";
const char* IFX_DUS_CH_DEV = "/dev/dus1";
const char* IFX_CPC_CTRL_DEV = "/dev/cpc5621";
#endif /* FXO */

/* ============================= */
/* Local variable definition     */
/* ============================= */

/** Prepare the macro table expansion for access mode names strings */
#undef sys_am
#define sys_am(generic,system,separator) #generic separator

/** Character array with access mode string lookup table. */
const IFX_char_t* access_mode_strings[] =
{
   SYS_ACCESS_MODE
};

/** Prepare the macro table expansion for access mode values */
#undef sys_am
#define sys_am(generic,system,separator) system separator
/** Character array with access mode string lookup table. */
const IFX_uint8_t access_mode_val[] =
{
   SYS_ACCESS_MODE
};


#ifdef USE_FILESYSTEM

#ifdef VMMC
//static IFX_char_t *sPRAMFile = "/firmware/INCA-IP2.bin";
static IFX_char_t *sPRAMFile = "/root/firmware.bin";
#else /* VMMC */
static IFX_char_t *sPRAMFile = "/tmp/pramfw.bin";
static IFX_char_t *sDRAMFile = "/tmp/dramfw.bin";
#endif /* VMMC */

/** File holding coefficients. */
//static IFX_char_t *sBBDFile  = "/tmp/bbd.bin";
static IFX_char_t *sBBDFile  = "/root/bbd.bin";

#endif /* USE_FILESYSTEM */


/* ============================= */
/* Global variable definition    */
/* ============================= */

/** File descriptor for board */
IFX_int32_t fdSys = -1;
/** File descriptor of control device */
IFX_int32_t fdDevCtrl = -1;
/** File descriptors of channels */
IFX_int32_t fdDevCh[MAX_SYS_CH_RES] = {-1};
#ifdef FXO
/** File descriptor for Duslic */
IFX_int32_t fdDusCtrl = -1;
IFX_int32_t fdDusCh[2] = {-1};
/** File descriptor for CPC */
IFX_int32_t fdCpcCtrl = -1;
#endif /* FXO */

/* ============================= */
/* Local function declaration    */
/* ============================= */

#ifdef USE_FILESYSTEM
static IFX_return_t ReadBinFile(IFX_char_t* filename, IFX_uint8_t** buf,
                                IFX_int32_t* size);
#endif /* USE_FILESYSTEM */

#ifdef VMMC
static IFX_void_t Fillin_FW_Ptr(VMMC_IO_INIT *vinit);
static IFX_void_t Release_FW_Ptr(VMMC_IO_INIT *vinit);
#else /* VMMC */
static IFX_void_t Fillin_FW_Ptr(VINETIC_IO_INIT *vinit);
static IFX_void_t Release_FW_Ptr(VINETIC_IO_INIT *vinit);
#endif /* VMMC */


/* ============================= */
/* Local function definition     */
/* ============================= */

#ifdef USE_FILESYSTEM
/**
   parses file and fill in given buffer

   \param filename  - binary file name
   \param buf       - buffer in which the file should be written
   \param size      - size of read buffer

   \return  read length or error code
*/
static IFX_return_t ReadBinFile(IFX_char_t* filename, IFX_uint8_t** buf,
                                IFX_int32_t* size)
{
   IFX_int32_t fd;
   IFX_return_t ret = IFX_SUCCESS;


   /* routine check : no file name given ? */
   /* routine check : memory for parameter size available ? */
   if ((IFX_NULL == filename) || (IFX_NULL == buf) || (IFX_NULL == size)
        || (0 == strlen(filename)) )
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Invalid input argument(s). (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   fd = open(filename, O_RDONLY, 0x644);
   if (0 >= fd)
   {
      /* No file by this name ? */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Can't open file %s, %s. \
           (File: %s, line: %d)\n",
            filename, strerror(errno), __FILE__, __LINE__));
      ret = IFX_ERROR;
   }

   /* Get file size */
   if (IFX_SUCCESS == ret)
   {
      *size = lseek(fd, 0, SEEK_END);
   }

   if (0 > *size)
   {
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Error get size of file %s, %s. \
           (File: %s, line: %d)\n",
            filename, strerror(errno), __FILE__, __LINE__));
      ret = IFX_ERROR;
   }

   /* Move pointer back to start of file, otherwise ZERO bytes will
      be read. */
   lseek(fd, 0, SEEK_SET);

   /* Allocate buffer memory if ptr is nil */
   if ((IFX_SUCCESS == ret) && (IFX_NULL == *buf))
   {
      *buf = (IFX_uint8_t *) malloc(*size);
      if (IFX_NULL == *buf)
      {
         TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
              ("No memory for file %s, %s. \
              (File: %s, line: %d)\n",
               filename, strerror(errno), __FILE__, __LINE__));
         ret = IFX_ERROR;
      }
   }

   /* Now read file into buffer */
   if ((IFX_SUCCESS == ret) && (read(fd, *buf, *size) < *size))
   {
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Error while reading file %s, %s. \
           (File: %s, line: %d)\n",
            filename, strerror(errno), __FILE__, __LINE__));
      ret = IFX_ERROR;
   }

   close(fd);

   return ret;
} /* ReadBinFile() */
#endif /* USE_FILESYSTEM */


/**
   Fills in firmware pointers.

   \param  vinit - handle to VINETIC_IO_INIT structure, not nil

   \return none

   \remarks
     In case no firmware binaries is found in the file system or no file system
     is used, default binaries will be used
*/
#ifdef VMMC
static void Fillin_FW_Ptr(VMMC_IO_INIT *vinit)
{
   IFX_int32_t ret_fwram = IFX_ERROR, ret_bbd = IFX_ERROR;
   static IFX_uint8_t *pPram = IFX_NULL;
   static IFX_uint8_t *p_bbd = IFX_NULL;
   printf("Fillin_FW_Ptr\n\r");
   /* check input arguments */
   if ((IFX_NULL == vinit))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Invalid input argument(s). (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return;
   }
   memset(vinit, 0, sizeof(VMMC_IO_INIT));
#ifdef USE_FILESYSTEM
   /* fill in DRAM and PRAM buffers from binary */
   ret_fwram = ReadBinFile(sPRAMFile, &pPram, &vinit->pram_size);
   if ((IFX_ERROR == ret_fwram) && (IFX_NULL == pPram))
   {
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Error reading file %s. (File: %s, line: %d)\n",
            sPRAMFile, __FILE__, __LINE__));
      free(pPram);
   }
   if ((IFX_SUCCESS == ret_fwram) && (IFX_NULL != pPram))
   {
      vinit->pPRAMfw = pPram;
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Filesystem firmware binaries used (%s)\n",sPRAMFile));
      printf("Filesystem firmware binaries used (%s)\n",sPRAMFile);
   }

   ret_bbd = ReadBinFile(sBBDFile, &p_bbd, &vinit->bbd_size);
   /* release bbd memory if applicable */
   if ((IFX_ERROR == ret_bbd) && (IFX_NULL == p_bbd))
   {
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Error reading file %s. (File: %s, line: %d)\n",
            sBBDFile, __FILE__, __LINE__));
      free(p_bbd);
   }
   if ((IFX_SUCCESS == ret_bbd) && (p_bbd))
   {
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, ("Filesystem BBD binary used (%s)\n",
                                       sBBDFile));
      printf("Filesystem bbd binaries used (%s)\n",sBBDFile);
      vinit->pBBDbuf = p_bbd;
  }

#endif /* USE_FILESYSTEM */
   return;
}

/**
   Release in firmware pointers.

   \param  vinit - handle to VINETIC_IO_INIT structure, not nil
   \return
      none
   \remarks
     In case no firmware binaries is found in the file system or no file system
     is used, default binaries will be used
*/
static IFX_void_t Release_FW_Ptr(VMMC_IO_INIT *vinit)
{
   /* check input arguments */
   if ((IFX_NULL == vinit))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Invalid input argument(s). (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return;
   }

} /* Release_FW_Ptr() */

#else /* VMMC */

/**
   Fills in firmware pointers.

   \param  vinit - handle to VINETIC_IO_INIT structure, not nil

   \return none

   \remarks
     In case no firmware binaries is found in the file system or no file system
     is used, default binaries will be used
*/
static IFX_void_t Fillin_FW_Ptr(VINETIC_IO_INIT* vinit)
{
   IFX_int32_t ret_fwram = IFX_ERROR, ret_cram = IFX_ERROR;

   static IFX_uint8_t* pPram = IFX_NULL;
   static IFX_uint8_t* pDram = IFX_NULL;
   static IFX_uint8_t* p_cram = IFX_NULL;


   if ((IFX_NULL == vinit))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Invalid input argument(s). (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return;
   }

   /* Set all entries to nil */
   memset(vinit, 0, sizeof(VINETIC_IO_INIT));

#ifdef USE_FILESYSTEM
   /* Fill in DRAM and PRAM buffers from binary */
   ret_fwram = ReadBinFile(sPRAMFile, &pPram, &vinit->pram_size);
   /* in case of error, release memory */
   if ((IFX_ERROR == ret_fwram) && (IFX_NULL == pPram))
   {
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Error reading file %s. (File: %s, line: %d)\n",
            sPRAMFile, __FILE__, __LINE__));

      free(pPram);
   }
   else
   {
      ret_fwram = ReadBinFile(sDRAMFile, &pDram, &vinit->dram_size);
      /* in case of error, release pram and dram memory if applicable */
      if (IFX_ERROR == ret_fwram)
      {
         TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
              ("Error reading file %s. (File: %s, line: %d)\n",
               sDRAMFile, __FILE__, __LINE__));

         if (IFX_NULL != pPram)
         {
            free (pPram);
         }
         if (IFX_NULL != pDram)
         {
            free (pDram);
         }
      }
      else
      {
         if (pPram)
         {
            vinit->pPRAMfw = pPram;
         }
         if (pDram)
         {
            vinit->pDRAMfw = pDram;
         }
         TRACE(TAPIDEMO, DBG_LEVEL_NORMAL,
              ("Filesystem firmware PRAM/DRAM binaries used\n"));
      }
   } /* else */

#ifdef VIN_2CPE
   ret_cram = ReadBinFile(sBBDFile, &p_cram, &vinit->cram_size);
   /* Release bbd memory if applicable */
   if (IFX_SUCCESS == ret_cram)
   {
      TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, ("Filesystem BBD binary used\n"));
      if (IFX_NULL != p_cram)
      {
         vinit->pCram = p_cram;
      }
   }
   else if (IFX_NULL != p_cram)
   {
      free(p_cram);
   }
#endif /* VIN_2CPE */


#endif /* USE_FILESYSTEM */

   /* If file system support isn't used or error occured while reading binary,
      use default PRAM/DRAM bins */
   if (IFX_ERROR == ret_fwram)
   {
      TRACE(TAPIDEMO, DBG_LEVEL_NORMAL,
           ("Default firmware PRAM/DRAM binaries used\n"));
      vinit->pPRAMfw = (IFX_uint8_t *) vinetic_fw_pram;
      vinit->pDRAMfw = (IFX_uint8_t *) vinetic_fw_dram;
      vinit->dram_size = sizeof(vinetic_fw_dram);
      vinit->pram_size = sizeof(vinetic_fw_pram);
   }
   /* If file system support isn't used or error occured while reading binary,
      use default CRAM bins */
   if (IFX_ERROR == ret_cram)
   {
#ifdef VIN_2CPE
      TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, ("Default bbd binaries used\n"));
      vinit->pCram = (IFX_uint8_t *) bbd_buf;
      vinit->cram_size = bbd_size;
#else
      TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, ("Default cram binaries used\n"));
      vinit->pCram = (IFX_uint8_t *) pCram;
      vinit->cram_size = sizeof(pCram);
#endif /* VIN_2CPE */
   }

   return;
} /* Fillin_FW_Ptr() */


/**
   Release in firmware pointers.

   \param  vinit - handle to VINETIC_IO_INIT structure, not nil

   \return none

*/
static IFX_void_t Release_FW_Ptr(VINETIC_IO_INIT* vinit)
{
   if (IFX_NULL == vinit)
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Invalid input argument(s). (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return;
   }

   /* Pram fw was read from file, as default isn't set: free memory */
   if (vinit->pPRAMfw != vinetic_fw_pram)
   {
      free(vinit->pPRAMfw);
   }

   /* Dram fw was read from file, as default isn't set: free memory */
   if (vinit->pDRAMfw != vinetic_fw_dram)
   {
      free(vinit->pDRAMfw);
   }

#ifdef VIN_2CPE
   if (vinit->pBBDbuf != bbd_buf)
   {
      free(vinit->pBBDbuf);
   }
#endif /* VIN_2CPE */

} /* Release_FW_Ptr() */

#endif /* VMMC */

/* ============================= */
/* Global function definition    */
/* ============================= */

/**
   Sets file descriptors.

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remarks
      On error, CommonClose_FDs() must be called to close all open fd-s.
*/
IFX_return_t Common_Set_FDs()
{
   IFX_uint8_t i = 0, opened = 0;
   IFX_char_t sdev_name[20];

#ifndef VMMC /* for VMMC fdSys is not necessary. */
   if (0 >= fdSys)
   {
      /* Get system fd */
#ifdef VXWORKS
      fdSys = open(SYSTEM_DEV, O_RDWR, 0x644);
#else /* VXWORKS */
      fdSys = open(SYSTEM_DEV, O_RDWR);
#endif /* VXWORKS */

      if (0 >= fdSys)
      {
         TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
              ("Invalid system file descriptor. (File: %s, line: %d)\n",
               __FILE__, __LINE__));
         return IFX_ERROR;
      }
      opened++;
   }
#endif /* VMMC */

   if (0 >= fdDevCtrl)
   {
      /* get vinetic control fd */
#ifdef VXWORKS
      fdDevCtrl = open(IFX_CTRL_DEV, O_RDWR, 0x644);
#else /* VXWORKS */
      fdDevCtrl = open(IFX_CTRL_DEV, O_RDWR);
#endif /* VXWORKS */

      if (0 >= fdDevCtrl)
      {
         TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
              ("Invalid vinetic control file descriptor."
               "(File: %s, line: %d)\n",
               __FILE__, __LINE__));
         return IFX_ERROR;
      }
      opened++;
   }

   for (i = 0; i < MAX_SYS_CH_RES; i++)
   {
      if (0 >= fdDevCh[i])
      {
#ifdef VMMC
         sprintf(sdev_name, "/dev/vmmc1%d", i+1);
#else
         sprintf(sdev_name,"/dev/vin1%d", i+1);
#endif /* VMMC */
         fdDevCh[i] = open(sdev_name, O_RDWR, 0x644);
         if (0 >= fdDevCh[i])
         {
            TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
                 ("Invalid vinetic channel (ch%d) file descriptor."
                  "(File: %s, line: %d)\n",
                  i, __FILE__, __LINE__));
            return IFX_ERROR;
         }
         opened++;
      }
   }

#ifdef VMMC
   /* Check opened fds */
   if (opened != (MAX_SYS_CH_RES + 1))
   {
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("One or more fds actually in use ... breaking."
            "(File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }
#else
   /* Check opened fds */
   if (opened != (MAX_SYS_CH_RES + 2))
   {
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("One or more fds actually in use ... breaking."
            "(File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }
#endif /* VMMC */

   TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, ("Device and channel fds opened."));

#ifdef FXO
   if (0 >= fdDusCtrl)
   {
      /* get duslic control fd */
#ifdef VXWORKS
      fdDusCtrl = open(IFX_DUS_CTRL_DEV, O_RDWR, 0x644);
#else /* VXWORKS */
      fdDusCtrl = open(IFX_DUS_CTRL_DEV, O_RDWR);
#endif /* VXWORKS */

      if (0 >= fdDusCtrl)
      {
         TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
              ("Invalid Duslic control file descriptor."
               "(File: %s, line: %d)\n",
               __FILE__, __LINE__));
         return IFX_ERROR;
      }
   }
   for (i = 0; i < 2; i++)
   {
      if (0 >= fdDusCh[i])
      {
         sprintf(sdev_name,"/dev/dus1%d", i+1);
         fdDusCh[i] = open(sdev_name, O_RDWR, 0x644);
         if (0 >= fdDusCh[i])
         {
            TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
                 ("Invalid Duslic channel (ch%d) file descriptor."
                  "(File: %s, line: %d)\n",
                  i, __FILE__, __LINE__));
            return IFX_ERROR;
         }
      }
   }

   if (0 >= fdCpcCtrl)
   {
      /* get CPC control fd */
#ifdef VXWORKS
      fdCpcCtrl = open(IFX_CPC_CTRL_DEV, O_RDWR, 0x644);
#else /* VXWORKS */
      fdCpcCtrl = open(IFX_CPC_CTRL_DEV, O_RDWR);
#endif /* VXWORKS */

      if (0 >= fdCpcCtrl)
      {
         TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
              ("Invalid CPC control file descriptor."
               "(File: %s, line: %d)\n",
               __FILE__, __LINE__));
         return IFX_ERROR;
      }
   }
   TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, ("Duslic and CPC fds opened."));
#endif /* FXO */

   return IFX_SUCCESS;
} /* CommonSet_FDs() */


/**
   Close file descriptors.

   \return none
*/
IFX_void_t Common_Close_FDs()
{
   IFX_int32_t i = 0;

#ifndef VMMC /* for VMMC fdSys is not necessary. */
   if (0 < fdSys)
   {
      close(fdSys);
      fdSys = -1;
   }
#endif /* VMMC */

   if (0 < fdDevCtrl)
   {
      close(fdDevCtrl);
      fdDevCtrl = -1;
   }

   for (i = 0; i < MAX_SYS_CH_RES; i++)
   {
      if (fdDevCh [i] > 0)
      {
         close(fdDevCh[i]);
         fdDevCh[i] = -1;
      }
   }

   TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, ("Device and channel fds closed.\n"));
#ifdef FXO
   if (0 < fdCpcCtrl)
   {
      close(fdCpcCtrl);
      fdDevCtrl = -1;
   }
   if (0 < fdDusCtrl)
   {
      close(fdDusCtrl);
      fdDevCtrl = -1;
   }

   for (i = 0; i < 2; i++)
   {
      if (fdDusCh [i] > 0)
      {
         close(fdDusCh[i]);
         fdDusCh[i] = -1;
      }
   }

   TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, ("Duslic and CPC fds closed.\n"));
#endif
} /* CommonClose_FDs() */


/**
   Return file descriptor for device control channel.

   \return -1 if err or device not open, otherwise file descriptor.
*/
IFX_int32_t Common_GetDevCtrlCh(IFX_void_t)
{
   return fdDevCtrl;
} /* Common_GetDevCtrlCh() */


/**
   Return file descriptor for specific channel (it makes no difference if
                                                phone or data)

   \param nChNum - channel number (starting from 0)

   \return -1 if err or device not open, otherwise file descriptor
*/
IFX_int32_t Common_GetDeviceOfCh(IFX_int32_t nChNum)
{
   if ((0 > nChNum) || (MAX_SYS_CH_RES < nChNum))
      return -1;

   return fdDevCh[nChNum];
} /* Common_GetDeviceOfCh() */


#ifdef FXO
/**
   Return file descriptor for device control channel.

   \return -1 if err or device not open, otherwise file descriptor.
*/
IFX_int32_t Common_GetCpcCtrlCh(IFX_void_t)
{
   return fdCpcCtrl;
} /* Common_GetCpcCtrlCh() */

/**
   Return file descriptor for device control channel.

   \return -1 if err or device not open, otherwise file descriptor.
*/
IFX_int32_t Common_GetDusCtrlCh(IFX_void_t)
{
   return fdDusCtrl;
} /* Common_GetDusCtrlCh() */


/**
   Return file descriptor for specific channel (it makes no difference if
                                                phone or data)

   \param nChNum - channel number (starting from 0)

   \return -1 if err or device not open, otherwise file descriptor
*/
IFX_int32_t Common_GetDusOfCh(IFX_int32_t nChNum)
{
   if ((0 > nChNum) || (2 < nChNum))
      return -1;

   return fdDusCh[nChNum];
} /* Common_GetDusOfCh() */

#endif

/**
   Basic device initialization prior to vinetic initialization.

   \param sysAccMode - system access mode (8/16-bit uC Mot/IntMux/IntDemux,
                       8Bit SPI)to set. This access mode must be converted
                       in a vinetic understandable access mode before doing
                       the vinetic driver initialization.

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark
      - In case of 8bit SPI, for the CPE system, base address  should be set to
        0x1F if not otherwise programmed by bootstrapping
      - Caller must make sure that the access mode is  supported
*/
IFX_return_t Common_InitSystem(sys_acc_mode_t sysAccMode)
{
   IFX_return_t ret = IFX_ERROR;
   IFX_uint32_t chipnum = 0, i;

#ifndef VMMC
   IFX_int32_t vin_access_mode = 0;
   IFX_uint32_t irq_num = 0, base_address = 0;
#endif


   PRINT_USED_ACCESS_MODE(sysAccMode)
   /* set system configuration */
   if (IFX_ERROR == System_Config(fdSys, sysAccMode))
   {
      return IFX_ERROR;
   }

   /* go on with initialization */
   if (IFX_ERROR == System_Init(fdSys, &chipnum))
   {
      return IFX_ERROR;
   }

   /* initialize each vinetic device */
   for (i = 0; i < chipnum; i++)
   {
      ret = IFX_ERROR;
      /* activate chip reset */
      if (IFX_ERROR == System_SetReset(fdSys, i))
      {
         break;
      }
      /* read vinetic device configuration parameters from board driver */
      if (IFX_ERROR == System_GetParams(fdSys, i, IFX_NULL, IFX_NULL,
                                       &base_address, &irq_num))
      {
         break;
      }
#ifndef VMMC
      /* get appropriate vinetic access mode */
      vin_access_mode = System_GetAccMode(sysAccMode);
      if (IFX_ERROR == vin_access_mode)
      {
         break;
      }
      /* configure vinetic device  */
      if (IFX_ERROR ==
           vinetic_init_device(fdDevCtrl, vin_access_mode, base_address,
                               irq_num))
      {
         break;
      }
#endif
      /* clear reset to allow vinetic access */
      if (IFX_ERROR == System_ClearReset(fdSys, i))
      {
         break;
      }
      /* this loop processed successfully */
      ret = IFX_SUCCESS;
   }

   if (IFX_ERROR == ret)
   {
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("ERROR: Basic Device Initialization fails on device %ld"
            "(File: %s, line: %d)\n",
            i, __FILE__, __LINE__));
   }

   return ret;
} /* CommonInitSystem() */


/**
   Basic device reset.

   \param dev_num    - id of device to reset

   \return 0 if successful, otherwise -1.
*/
IFX_return_t Common_ResetSystem(IFX_int32_t dev_num)
{
   IFX_return_t ret = IFX_SUCCESS;


   /* Activate chip reset */
   ret = System_SetReset(fdSys, dev_num);

#ifdef VMMC
   /* Reset internal vinetic device data */
   if (IFX_SUCCESS == ret)
   {
      ret = vmmc_reset_device(fdDevCtrl);
   }
#else
   /* Reset internal vinetic device data */
   if (IFX_SUCCESS == ret)
   {
      ret = vinetic_reset_device(fdDevCtrl);
   }
   /* Clear reset to allow vinetic access */
   if (IFX_SUCCESS == ret)
   {
      ret = System_ClearReset(fdSys, dev_num);
   }
#endif /* VMMC */

   if (IFX_ERROR == ret)
   {
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("ERROR: Device Reset fails on device %ld"
            "(File: %s, line: %d)\n",
            dev_num, __FILE__, __LINE__));
   }

   return ret;
} /* CommonResetSystem() */


/**
   Read version after succesfull initialization and display it.

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark
      This must be done only after a successfull basic device initialization
*/
IFX_return_t Common_GetVersions()
{
   IFX_return_t ret;
#ifdef VMMC
   VMMC_IO_VERSION vinVers;

   memset(&vinVers, 0, sizeof(VMMC_IO_VERSION));
#else
   VINETIC_IO_VERSION vinVers;

   memset(&vinVers, 0, sizeof(VINETIC_IO_VERSION));
#endif /* VMMC */

   ret = ioctl(fdDevCtrl, FIO_VINETIC_VERS, (IFX_int32_t) &vinVers);
   if (IFX_SUCCESS == ret)
   {
      SEPARATE
#ifdef VMMC
      printf("VMMC [version 0x%2X, type 0x%2X, channels %d] ready!\n\r"
             "FW Version %d.%d\n\rDriver version 0x%08lX\n\r",
              vinVers.nChip, vinVers.nType, vinVers.nChannel,
              vinVers.nEdspVers, vinVers.nEdspIntern, vinVers.nDrvVers);
#else
      printf("VINETIC [version 0x%2X, type 0x%2X, channels %d] ready!\n\r"
             "FW Version %d.%d\n\rDriver version 0x%08lX\n\r",
              vinVers.nChip, vinVers.nType, vinVers.nChannel,
              vinVers.nEdspVers, vinVers.nEdspIntern, vinVers.nDrvVers);
#endif /* VMMC */
      SEPARATE
   }

   return ret;
} /* CommonGetVersions() */


/**
   Tapi initialization.

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark
      This must be done only after a successfull basic device initialization
*/
IFX_return_t Common_InitSilicon()
{
   IFX_uint8_t              i = 0;
#ifdef VMMC
   VMMC_IO_INIT             vin_proc;
#else
   VINETIC_IO_INIT          vin_proc;
#endif /* VMMC */
   IFX_TAPI_CH_INIT_t       Init;


   /* Set vinetic init structure */
   Fillin_FW_Ptr(&vin_proc);

   /* Set tapi init structure */
   memset(&Init, 0, sizeof(IFX_TAPI_CH_INIT_t));
   Init.nMode = IFX_TAPI_INIT_MODE_VOICE_CODER;

   Init.pProc = (IFX_void_t*) &vin_proc;

   /* Initialize all tapi channels */
   for (i = 0; i <= MAX_SYS_CH_RES; i++)
   {
      if (MAX_SYS_CH_RES == i)
      {
         /* Read version */
         Common_GetVersions();
         break;
      }
      /* Initialize all system channels */
      if (0 != ioctl(fdDevCh[i], IFX_TAPI_CH_INIT, (IFX_int32_t) &Init))
      {
         break;
      }

      /* Set appropriate feeding on all (analog) line channels */
      if (i < MAX_SYS_LINE_CH)
      {
         /* Set line in standby */
         if (IFX_SUCCESS != ioctl(fdDevCh[i], IFX_TAPI_LINE_FEED_SET,
             IFX_TAPI_LINE_FEED_STANDBY))
         {
            break;
         }
#if 0
         /* Line will be set in active mode only on OFFHOOK event. */
         /* Set line in active mode */
         if (IFX_SUCCESS != ioctl(fdDevCh[i], IFX_TAPI_LINE_FEED_SET,
             IFX_TAPI_LINE_FEED_ACTIVE))
         {
            break;
         }
#endif
      }
   } /* for */

   /* Release fw ptr */
   Release_FW_Ptr(&vin_proc);

   if (MAX_SYS_CH_RES != i)
   {
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Init failed for ch %d, (File: %s, line: %d)\n",
            i, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   return IFX_SUCCESS;
} /* CommonInitSilicon() */


#ifdef EASY3332

/**
   PCM initialization.

   \param fMaster - flag indicating if this board will be master (1) or
                    slave (0)

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark Must be called after system inicialization.
*/
IFX_return_t Common_InitPCM(IFX_boolean_t fMaster)
{
   IFX_uint8_t reg1_value = 0;


   /* Read register */
   System_ReadReg(fdSys, CMDREG1_OFF, &reg1_value);

   TRACE(TAPIDEMO, DBG_LEVEL_NORMAL,
        ("Read CMDREG%d 0x%x\n", 1, reg1_value));

   if (IFX_TRUE == fMaster)
   {
      TRACE(TAPIDEMO, DBG_LEVEL_LOW, ("Set PCM on MASTER board\n"));
      reg1_value |= MASTER;
      reg1_value |= PCMEN_MASTER;
      reg1_value |= PCMON;
   }
   else
   {
      TRACE(TAPIDEMO, DBG_LEVEL_LOW, ("Set PCM on SLAVE board\n"));
      reg1_value &= ~MASTER;
      reg1_value &= ~PCMEN_MASTER;
      reg1_value |= PCMON;
   }

   System_WriteReg(fdSys, CMDREG1_OFF, reg1_value);

   TRACE(TAPIDEMO, DBG_LEVEL_NORMAL,
        ("Write CMDREG%d 0x%x\n", 1, reg1_value));

   return IFX_SUCCESS;
} /* Common_InitPCM() */


#elif EASY334

/**
   PCM initialization.

   \param fdSys   - system fd
   \param fMaster - flag indicating if this board will be master (1) or
                    slave (0)

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark Must be called after system inicialization.
*/
IFX_return_t Common_InitPCM(IFX_boolean_t fMaster)
{
   IFX_uint8_t reg1_value = 0;
   IFX_uint8_t reg2_value = 0;
   IFX_uint8_t reg3_value = 0;


   /* Read register */
   System_ReadReg(fdSys, CMDREG1_OFF, &reg1_value);
   System_ReadReg(fdSys, CMDREG2_OFF, &reg2_value);
   System_ReadReg(fdSys, CMDREG3_OFF, &reg3_value);

   TRACE(TAPIDEMO, DBG_LEVEL_NORMAL,
        ("Read CMDREG%d 0x%x\n", 1, reg1_value));
   TRACE(TAPIDEMO, DBG_LEVEL_NORMAL,
        ("Read CMDREG%d 0x%x\n", 2, reg2_value));
   TRACE(TAPIDEMO, DBG_LEVEL_NORMAL,
        ("Read CMDREG%d 0x%x\n", 3, reg3_value));

   /* Set some common values for both MASTER and SLAVE board */
   reg1_value = 0;
   reg1_value |= VRES;
   reg2_value = 0;
   reg2_value |= HIGHWAY_B;
   reg2_value |= MICRO_CONTROLER_MPC_VIN;
   reg2_value |= MOTOR_16_BIT_AND_PCM;
   reg3_value = 0;
   reg3_value |= MCLK_IS_DCL;
   reg3_value |= IOM_2_CLK_OUT;
   reg3_value |= FREQ_2048_KHZ_8_KHZ;

   if (IFX_TRUE == fMaster)
   {
      TRACE(TAPIDEMO, DBG_LEVEL_LOW, ("Set PCM on MASTER board\n"));
      reg3_value |= MASTER;
   }
   else
   {
      TRACE(TAPIDEMO, DBG_LEVEL_LOW, ("Set PCM on SLAVE board\n"));
      reg3_value &= ~MASTER;
   }

   System_WriteReg(fdSys, CMDREG1_OFF, reg1_value);
   System_WriteReg(fdSys, CMDREG2_OFF, reg2_value);
   System_WriteReg(fdSys, CMDREG3_OFF, reg3_value);

   TRACE(TAPIDEMO, DBG_LEVEL_NORMAL,
        ("Write CMDREG%d 0x%x\n", 1, reg1_value));
   TRACE(TAPIDEMO, DBG_LEVEL_NORMAL,
        ("Write CMDREG%d 0x%x\n", 2, reg2_value));
   TRACE(TAPIDEMO, DBG_LEVEL_NORMAL,
        ("Write CMDREG%d 0x%x\n", 3, reg3_value));


   return IFX_SUCCESS;
} /* Common_InitPCM() */

#elif VMMC

/**
   PCM initialization.

   \param fdSys   - system fd
   \param fMaster - flag indicating if this board will be master (1) or
                    slave (0)

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark Must be called after system inicialization.
*/
/* Winder, PCM not implemented in TAPI V3 for INCA2 yet. */
IFX_return_t Common_InitPCM(IFX_boolean_t fMaster)
{
   return IFX_SUCCESS;
} /* Common_InitPCM() */

#elif YOUR_SYSTEM

/**
   PCM initialization.

   \param fdSys   - system fd
   \param fMaster - flag indicating if this board will be master (1) or
                    slave (0)

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark Must be called after system inicialization.
*/
IFX_return_t Common_InitPCM(IFX_boolean_t fMaster)
{
   return IFX_SUCCESS;
} /* Common_InitPCM() */

#endif /* YOUR_SYSTEM */


/**
   Get capabilities, basically number of phone, data and pcm channels.

   \param nChannel_FD - FD to data or phone channel
   \param nPhoneChCnt - pointer to phone channels count
   \param nDataChCnt  - pointer to data channels count
   \param nPCM_ChCnt  - pointer to pcm channels count

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t Common_GetCapabilities(IFX_int32_t *nPhoneChCnt,
                                    IFX_int32_t *nDataChCnt,
                                    IFX_int32_t *nPCM_ChCnt)
{
   IFX_return_t ret = IFX_SUCCESS;
   IFX_int32_t nCapNr = 0;
   IFX_TAPI_CAP_t *pCapList = IFX_NULL;
   IFX_int32_t i = 0;


   if ((IFX_NULL == nPhoneChCnt) || (IFX_NULL == nDataChCnt)
       || (IFX_NULL == nPCM_ChCnt))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Invalid input argument(s). (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* Get the cap list size */
   ret = ioctl(fdDevCtrl, IFX_TAPI_CAP_NR, (int) &nCapNr);

   TRACE(TAPIDEMO, DBG_LEVEL_NORMAL,
        ("Number of capabilities %d.\n", (int) nCapNr));

#ifdef VXWORKS
   if (0 == nCapNr)
   {
      /** \todo Under VxWorks a count 0 is received, strange ??? */
      nCapNr = 20;
   }
#endif /* VXWORKS */

   if (0 == nCapNr)
   {
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("0 capabilities available read from fd %d of ctrl ch, "
              " was TAPI initialized? \n",
              (int) fdDevCtrl));
      return IFX_ERROR;
   }

   /* \todo VOS_Malloc() check for not enough memory?, yes but return NULL */
   pCapList = malloc(nCapNr * sizeof(IFX_TAPI_CAP_t));
   if (IFX_NULL == pCapList)
   {
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("pCapList is nil. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   memset(pCapList, 0, sizeof(pCapList));

   /* Get the cap list */
   ret = ioctl(fdDevCtrl, IFX_TAPI_CAP_LIST, (int) &(*pCapList));

   for (i = 0; i < nCapNr; i++)
   {
      switch (pCapList[i].captype)
      {
         case IFX_TAPI_CAP_TYPE_PCM:
               *nPCM_ChCnt = pCapList[i].cap;
               TRACE(TAPIDEMO, DBG_LEVEL_NORMAL,
                    ("Available PCM channels %d.\n", (int) *nPCM_ChCnt));
            break;
         case IFX_TAPI_CAP_TYPE_CODECS:
               *nDataChCnt = pCapList[i].cap;
               TRACE(TAPIDEMO, DBG_LEVEL_NORMAL,
                    ("Available data channels %d.\n", (int) *nDataChCnt));
            break;
         case IFX_TAPI_CAP_TYPE_PHONES:
               *nPhoneChCnt = pCapList[i].cap;
               TRACE(TAPIDEMO, DBG_LEVEL_NORMAL,
                    ("Available phone channels %d.\n", (int) *nPhoneChCnt));
            break;
         default:
            break;
      }
   }

   /* Free the allocated memory */
   free(pCapList);
   pCapList = IFX_NULL;

   return ret;
} /* Common_GetCapabilities() */


#if 0

/* --------------------------------------------------------
      !!! This functions are not used at the moment !!!
 */

/**
   Converts digit event into ascii code.

   \param digit - digit received as event

  \return
      ascii character of given digit
*/
IFX_char_t Common_Get_DTMF_Ascii(IFX_uint8_t digit)
{
   IFX_char_t ascii = digit;


   switch (digit)
   {
      case 0x0A: ascii  = '*';  break;
      case 0x0B: ascii  = '0';  break;
      case 0x0C: ascii  = '#';  break;
      case 0x1C: ascii  = 'A';  break;
      case 0x1D: ascii  = 'B';  break;
      case 0x1E: ascii  = 'C';  break;
      case 0x1F: ascii  = 'D';  break;
      default  : ascii  += '0';  break;
   }

   return ascii;
} /* CommonGet_DTMF_Ascii() */


/**
   Function to close the AC path so that the signal played is send back for
   detection.
*/
IFX_return_t Common_Close8kHzDigitalLoop(IFX_int32_t fdDevCtrl,
                                         IFX_uint8_t ch,
                                         IFX_boolean_t b_close)
{
   IFX_return_t ret = IFX_SUCCESS;

#ifdef VMMC /* Is it necessary for VMMC, vinder. */
#ifdef VIN_2CPE
   {
      IFX_uint16_t dc_dbg_cmd[2] = {0x0100, 0x0A05},
                   dc_dbg_data[5] = {0};

      if (ch > 1)
         return IFX_SUCCESS;

      ret = Common_ReadCmd(fdDevCtrl, (dc_dbg_cmd [0] | ch), dc_dbg_cmd [1],
                           dc_dbg_data);
      /* Do according modifications */
      if (IFX_TRUE == b_close)
      {
         dc_dbg_data [0] |= 0x1;
         dc_dbg_data [2] |= 0x807F;
      }
      else
      {
         dc_dbg_data [0] &= ~0x1;
         dc_dbg_data [2] &= ~0x0040;
      }
      /* Write dc test message to close 8 kHz digital loopback */
      ret = Common_WriteCmd(fdDevCtrl, (dc_dbg_cmd [0] | ch), dc_dbg_cmd [1],
                            dc_dbg_data);
   }
#else /* VIN_2CPE */
   {
      IFX_uint16_t tstr2_val = 0, bcr1 = 0;


      if (IFX_TRUE == b_close)
      {
         tstr2_val = 0x02C0;
         bcr1      = 0x2010;
      }
      else
      {
         /* To be adapted */
         tstr2_val = 0;
         bcr1      = 0;
      }
      /* Set TSTR2 = 0x02C0 to close 8 kHz digital loopback */
      ret = Common_WriteReg(fdDevCtrl, (0x0100 | ch), 0x0F, 1, &tstr2_val);
      /* Set BCR1 = 0x2010 to enable test loop */
      if (IFX_SUCCESS == ret)
      {
         ret = Common_WriteReg(fdDevCtrl, (0x0100 | ch), 0x07, 1, &bcr1);
      }
   }
#endif /* VIN_2CPE */
#endif /* VMMC */

   return ret;
} /* Common_Close8kHzDigitalLoop() */


/* ----------------------------------------------------------------------------
                   Only for debugging and testing purposes.
                    Using may disturb the driver operation
                               ONLY on 2CPE
   ----------------------------------------------------------------------------
 */


/**
   read command

   \param cmd1      - first command
   \param cmd2      - second command + length
   \param data      - ptr to data array

   \return
      IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark
      This must be done only after a successfull basic device initialization
*/
IFX_return_t Common_ReadCmd(IFX_uint16_t cmd1,
                            IFX_uint16_t cmd2,
                            IFX_uint16_t* data)
{
#ifdef VMMC
   VMMC_IO_MB_CMD mbxCmd;
#else /* VMMC */
   VMMC_IO_MB_CMD mbxCmd;
#endif /* VMMC */
   IFX_int32_t len = 0;


   if ((IFX_NULL == data))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Invalid input argument(s). (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* Fill control structure for write and read bytes */
   memset(&mbxCmd, 0, sizeof(mbxCmd));
#ifdef VMMC
   mbxCmd.cmd = cmd1<<16;
   mbxCmd.cmd |= cmd2;
#else /* VMMC */
   mbxCmd.cmd1 = cmd1;
   mbxCmd.cmd2 = cmd2;
#endif /* VMMC */
   len = (cmd2 & 0xFF);
   if ((0 == len) || (0x1D < len))
   {
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Cmd length %d out of range 1 <= Length <= 29. (File: %s, line: %d)\n",
            (cmd2 & 0xFF), __FILE__, __LINE__));
      return IFX_ERROR;
   }
   if (IFX_ERROR == ioctl(fdDevCtrl, FIO_VINETIC_RCMD, (IFX_int32_t) &mbxCmd))
   {
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("ReadCmd <cmd1=0x%04X, cmd2=0x%04X> failed."
            " (File: %s, line: %d)\n",
            cmd1, cmd2, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   memcpy(data, mbxCmd.pData, (len * 2));

   return IFX_SUCCESS;
} /* CommonReadCmd() */


/**
   writes command

   \param cmd1      - first command
   \param cmd2      - second command + length
   \param data      - ptr to data array

   \return
      IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark
      This must be done only after a successfull basic device initialization
*/
IFX_return_t Common_WriteCmd(IFX_uint16_t cmd1,
                             IFX_uint16_t cmd2,
                             IFX_uint16_t* data)
{
#ifdef VMMC
   VMMC_IO_MB_CMD mbxCmd;
#else
   VMMC_IO_MB_CMD mbxCmd;
#endif
   IFX_int32_t len = 0;


   if ((IFX_NULL == data))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Invalid input argument(s). (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* Fill control structure for write and read bytes */
   memset(&mbxCmd, 0, sizeof(mbxCmd));
#ifdef VMMC
   mbxCmd.cmd = cmd1<<16;
   mbxCmd.cmd |= cmd2;
#else /* VMMC */
   mbxCmd.cmd1 = cmd1;
   mbxCmd.cmd2 = cmd2;
#endif /* VMMC */
   len = (cmd2 & 0xFF);

   if ((0 == len) || (0x1D < len))
   {
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Cmd length %d out of range 1 <= Length <= 29. (File: %s, line: %d)\n",
            (cmd2 & 0xFF), __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* Map data to write */
   memcpy(mbxCmd.pData, data, (len * 2));

   if (IFX_ERROR == ioctl(fdDevCtrl, FIO_VINETIC_WCMD, (IFX_int32_t) &mbxCmd))
   {
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("WriteCmd <cmd1=0x%04X, cmd2=0x%04X> failed."
            " (File: %s, line: %d)\n",
            cmd1, cmd2, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   return IFX_SUCCESS;
} /* CommonWriteCmd() */


/**
   read register(s)

   \param cmd1      - first command header if applicable, non valid for CPE
   \param offset    - offset of first register to read
   \param len       - number of register to write from the base offset
   \param values    - ptr to register values array

   \return
      IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark
      This must be done only after a successfull basic device initialization
*/
IFX_return_t Common_ReadReg(IFX_uint16_t cmd1,
                            IFX_uint16_t offset,
                            IFX_int32_t len,
                            IFX_uint16_t* values)
{
   if ((IFX_NULL == values))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Invalid input argument(s). (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

#ifdef VIN_2CPE
   {
      VINETIC_IO_REG_ACCESS reg;


      memset(&reg, 0, sizeof(reg));
      reg.offset = offset;
      reg.count = len;
      if (IFX_ERROR == ioctl(fdDevCtrl, FIO_VINETIC_RDREG, (IFX_int32_t) &reg))
      {
         TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
              ("Readreg <off=0x%02X, len=%ld> failed. (File: %s, line: %d)\n",
               offset, len, __FILE__, __LINE__));
         return IFX_ERROR;
      }
      memcpy(values, reg.pData, len * 2);
   }
#else
   if (IFX_ERROR == Common_ReadCmd(fdDevCtrl, cmd1, (offset << 8 | len),
                                   values))
   {
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Readreg <cmd1=0x%04X, off=0x%02X, len=%ld> failed. "
            "(File: %s, line: %d)\n",
            cmd1, offset, len, __FILE__, __LINE__));
      return IFX_ERROR;
   }
#endif /* VIN_2CPE */

   return IFX_SUCCESS;
} /* CommonReadReg() */


/**
   writes register(s)

   \param cmd1      - first command header if applicable, non valid for CPE
   \param offset    - offset of first register to read
   \param len       - number of register to write from the base offset
   \param values    - ptr to register values array

   \return
      IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark
      This must be done only after a successfull basic device initialization
*/
IFX_return_t Common_WriteReg(IFX_uint16_t cmd1,
                             IFX_uint16_t offset,
                             IFX_int32_t len,
                             IFX_uint16_t* values)
{
   if ((IFX_NULL == values))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Invalid input argument(s). (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

#ifdef VIN_2CPE
   {
      VINETIC_IO_REG_ACCESS reg;


      memset(&reg, 0, sizeof (reg));
      reg.offset = offset;
      reg.count = len;
      memcpy(reg.pData, values, len * 2);
      if (IFX_ERROR == ioctl(fdDevCtrl, FIO_VINETIC_WRREG, (IFX_int32_t) &reg))
      {
         TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
              ("Writereg <off=%d, len=%ld> failed. (File: %s, line: %d)\n",
               offset, len, __FILE__, __LINE__));
         return IFX_ERROR;
      }
   }
#else
   if (IFX_ERROR == Common_WriteCmd(fdDevCtrl, cmd1, (offset << 8 | len),
                                    values))
   {
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Writereg <cmd1=%d, off=%d, len=%ld> failed. "
            "(File: %s, line: %d)\n",
            cmd1, offset, len, __FILE__, __LINE__));
      return IFX_ERROR;
   }
#endif /* VIN_2CPE */

   return IFX_SUCCESS;
} /* CommonWriteReg() */


/* ----------------------------------------------------------------------------
                   Only for debugging and testing purposes.
                    Using may disturb the driver operation
                               ONLY on 2CPE
   ----------------------------------------------------------------------------
 */


/* --------------------------------------------------------
      !!! This functions are not used at the moment !!!
 */

#endif
