/*******************************************************************************
       Copyright (c) 2005, Infineon Technologies.  All rights reserved.

                               No Warranty
   Because the program is licensed free of charge, there is no warranty for
   the program, to the extent permitted by applicable law.  Except when
   otherwise stated in writing the copyright holders and/or other parties
   provide the program "as is" without warranty of any kind, either
   expressed or implied, including, but not limited to, the implied
   warranties of merchantability and fitness for a particular purpose. The
   entire risk as to the quality and performance of the program is with
   you.  should the program prove defective, you assume the cost of all
   necessary servicing, repair or correction.

   In no event unless required by applicable law or agreed to in writing
   will any copyright holder, or any other party who may modify and/or
   redistribute the program as permitted above, be liable to you for
   damages, including any general, special, incidental or consequential
   damages arising out of the use or inability to use the program
   (including but not limited to loss of data or data being rendered
   inaccurate or losses sustained by you or third parties or a failure of
   the program to operate with any other programs), even if such holder or
   other party has been advised of the possibility of such damages.
********************************************************************************
   Module       : lib_vmmc.c
   Date         : 2005-06-07
   Description  : VMMC (INCAIP2, Danube,TwinPass) board library.
*******************************************************************************/
/* ============================= */
/* Includes                      */
/* ============================= */

/* system includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h> /* valid for Linux/VxWorks */

/* project includes */
#include "ifx_types.h"
#include "vmmc_io.h"

/******************************************************************************/
/**
   Initialize board

   \param fdBoard       - board file  descriptor
   \param chips_num     - number of vinetic chips on board

   \return IFX_SUCCESS / IFX_ERROR
*/
/******************************************************************************/
IFX_int32_t vmmc_init_board (IFX_uint32_t fdBoard, IFX_uint32_t *chips_num)
{
   *chips_num = 1;
   
   return IFX_SUCCESS;
}

/******************************************************************************/
/**
   Resets Vmmc Control device of given file descriptor

   \param fdCtrlDev  - vmmc device file descriptor

   \return IFX_SUCCESS / IFX_ERROR

   \remark
      This function can be called to internally reset the device within the
      vmmc driver after a hardware reset.
*/
/******************************************************************************/
IFX_int32_t vmmc_reset_device (IFX_uint32_t fdCtrlDev)
{
   IFX_int32_t ret;

   ret = ioctl (fdCtrlDev, FIO_DEV_RESET, 0);

   return ret;
}

