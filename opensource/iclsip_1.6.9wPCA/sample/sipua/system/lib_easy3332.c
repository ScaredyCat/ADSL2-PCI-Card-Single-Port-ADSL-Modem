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
   Module       : lib_easy3332.c
   Date         : 2005-06-07
   Description  : easy3332 board library.
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
#include "easy3332_io.h"
#include "vinetic_io.h"
#include "lib_easy3332.h"

/* ============================= */
/* Local defines                 */
/* ============================= */

/* ============================= */
/* Local variable definition     */
/* ============================= */

/* ============================= */
/* Global variable declaration   */
/* ============================= */

/* ============================= */
/* Global variable definition    */
/* ============================= */

/* ============================= */
/* Local function definition     */
/* ============================= */

/* ====================================== */
/* Board Driver/Device Control Functions  */
/* ====================================== */

/******************************************************************************/
/**
   Set trace/log level

   \param fdBoard - board file  descriptor
   \param level   - level to set

   \return none
*/
/******************************************************************************/
IFX_void_t   easy3332_set_tracelevel (IFX_uint32_t fdBoard, IFX_uint32_t level)
{
   ioctl (fdBoard, FIO_EASY3332_DEBUGLEVEL, level);

   return;
}

/******************************************************************************/
/**
   Get driver version

   \param fdBoard       - board file  descriptor
   \param vers_string   - version string pointer

   \return IFX_SUCCESS / IFX_ERROR
*/
/******************************************************************************/
IFX_int32_t easy3332_get_drvversion (IFX_uint32_t fdBoard, IFX_char_t *vers_string)
{
   IFX_int32_t ret;

   ret = ioctl (fdBoard, FIO_EASY3332_GET_VERSION, (IFX_uint32_t)vers_string);

   return ret;
}

/******************************************************************************/
/**
   Get driver version

   \param fdBoard       - board file  descriptor
   \param board_vers    - board version pointer

   \return IFX_SUCCESS / IFX_ERROR
*/
/******************************************************************************/
IFX_int32_t easy3332_get_boardversion (IFX_uint32_t fdBoard, IFX_int32_t *board_vers)
{
   IFX_int32_t ret;

   ret = ioctl (fdBoard, FIO_EASY3332_GET_BOARDVERS, (IFX_uint32_t)board_vers);

   return ret;
}

/******************************************************************************/
/**
   Set configuration

   \param fdBoard       - board file  descriptor
   \param access_mode   - access mode to set physically
   \param clock_rate    - clock rate

   \return IFX_SUCCESS / IFX_ERROR
*/
/******************************************************************************/
IFX_int32_t easy3332_set_config (IFX_uint32_t fdBoard, IFX_uint32_t access_mode,
                                IFX_uint32_t clock_rate)
{
   IFX_int32_t ret;
   EASY3332_Config_t config;

   memset (&config, 0, sizeof (config));
   config.nAccessMode = access_mode;
   config.nClkRate    = clock_rate;

   ret = ioctl (fdBoard, FIO_EASY3332_CONFIG, (IFX_uint32_t)&config);

   return ret;
}

/******************************************************************************/
/**
   Set default configuration

   \param fdBoard    - board file  descriptor

   \return IFX_SUCCESS / IFX_ERROR
*/
/******************************************************************************/
IFX_int32_t easy3332_reset_config   (IFX_uint32_t fdBoard)
{
   IFX_int32_t ret;

   ret = ioctl (fdBoard, FIO_EASY3332_CONFIG, (IFX_uint32_t)0);

   return ret;
}

/******************************************************************************/
/**
   Get configuration

   \param fdBoard       - board file  descriptor
   \param access_mode   - ptr to access mode
   \param clock_rate    - ptr to clock rate

   \return IFX_SUCCESS / IFX_ERROR
*/
/******************************************************************************/
IFX_int32_t easy3332_get_config (IFX_uint32_t fdBoard, IFX_uint32_t *access_mode,
                                IFX_uint32_t *clock_rate)
{
   IFX_int32_t ret;
   EASY3332_Config_t config;

   memset (&config, 0, sizeof (config));
   ret = ioctl (fdBoard, FIO_EASY3332_GETCONFIG, (IFX_uint32_t)&config);
   if (ret == IFX_SUCCESS)
   {
      if (access_mode != NULL) *access_mode = config.nAccessMode;
      if (clock_rate  != NULL) *clock_rate  = config.nClkRate;
   }

   return ret;
}

/******************************************************************************/
/**
   Initialize board

   \param fdBoard       - board file  descriptor
   \param chips_num     - number of vinetic chips on board

   \return IFX_SUCCESS / IFX_ERROR
*/
/******************************************************************************/
IFX_int32_t easy3332_init_board (IFX_uint32_t fdBoard, IFX_uint32_t *chips_num)
{
   IFX_int32_t ret;

   ret = ioctl (fdBoard, FIO_EASY3332_INIT, (IFX_uint32_t)0);
   if (ret >= 0)
   {
      if (chips_num != NULL) *chips_num = ret;
      ret = IFX_SUCCESS;
   }

   return ret;
}


/******************************************************************************/
/**
   Access Mode conversion from board to vinetic driver interfaces

   \param vin_access_mode - vinetic access mode

   \return board access mode or IFX_ERROR
*/
/******************************************************************************/
IFX_int32_t easy3332_v2b_ifce_accessmode (IFX_uint32_t vin_access_mode)
{
   IFX_int32_t access_mode = -1;

   switch (vin_access_mode)
   {
   case  VIN_ACCESS_SPI:
      access_mode = EASY3332_ACCESS_SPI;
      break;
   case  VIN_ACCESS_PAR_8BIT:
      access_mode = EASY3332_ACCESS_8BIT_MOTOROLA;
      break;
      break;
   case  VIN_ACCESS_PARINTEL_MUX8:
      access_mode = EASY3332_ACCESS_8BIT_INTELMUX;
      break;
   case  VIN_ACCESS_PARINTEL_DMUX8:
      access_mode = EASY3332_ACCESS_8BIT_INTELDEMUX;
      break;
   default:
      printf ("lib_easy3332: Unknown Vinetic "
              "Access Mode 0x%lX\n\r", vin_access_mode);
      return IFX_ERROR;
   }

   return access_mode;
}

/******************************************************************************/
/**
   Access Mode conversion from board to vinetic driver interfaces

   \param vin_access_mode - vinetic access mode

   \return vinetic access mode or IFX_ERROR
*/
/******************************************************************************/
IFX_int32_t easy3332_b2v_ifce_accessmode (IFX_uint32_t access_mode)
{
   IFX_int32_t vin_access_mode = -1;

   switch (access_mode)
   {
   case  EASY3332_ACCESS_8BIT_MOTOROLA:
      vin_access_mode = VIN_ACCESS_PAR_8BIT;
      break;
   case  EASY3332_ACCESS_8BIT_INTELMUX:
      vin_access_mode = VIN_ACCESS_PARINTEL_MUX8;
      break;
   case  EASY3332_ACCESS_8BIT_INTELDEMUX:
      vin_access_mode = VIN_ACCESS_PARINTEL_DMUX8;
      break;
   case EASY3332_ACCESS_SPI:
      vin_access_mode = VIN_ACCESS_SPI;
      break;
   default:
      printf ("lib_easy3332: Unknown Physical "
              "Access Mode 0x%lX\n\r", access_mode);
      return IFX_ERROR;
   }

   return vin_access_mode;
}

/******************************************************************************/
/**
   Resets the chip and makes it ready for access

   \param fdBoard       - board file  descriptor
   \param chip_num      - chip number

   \return IFX_SUCCESS / IFX_ERROR

   \remark
      chip reset will be activated and deactivated in the recommended sequence
      and time frame.
*/
/******************************************************************************/
IFX_int32_t easy3332_reset_chip (IFX_uint32_t fdBoard, IFX_uint32_t chip_num)
{
   IFX_int32_t ret;
   EASY3332_Reset_t param;

   memset (&param, 0, sizeof (param));
   param.nChipNum   = chip_num;
   param.nResetMode = EASY3332_RESET_ACTIVE_DEACTIVE;

   ret = ioctl (fdBoard, FIO_EASY3332_RESETCHIP, (IFX_uint32_t)&param);

   return ret;
}

/******************************************************************************/
/**
   Activates chip reset

   \param fdBoard       - board file  descriptor
   \param chip_num      - chip number

   \return IFX_SUCCESS / IFX_ERROR

   \remark
      To access the chip, trhe reset line must be deactivated and the access
      time frame must be respected.

   \see
      easy3332_chip_clearreset()
*/
/******************************************************************************/
IFX_int32_t easy3332_chip_setreset (IFX_uint32_t fdBoard, IFX_uint32_t chip_num)
{
   IFX_int32_t ret;
   EASY3332_Reset_t param;

   memset (&param, 0, sizeof (param));
   param.nChipNum   = chip_num;
   param.nResetMode = EASY3332_RESET_ACTIVE;

   ret = ioctl (fdBoard, FIO_EASY3332_RESETCHIP, (IFX_uint32_t)&param);

   return ret;

}

/******************************************************************************/
/**
   Clears chip reset

   \param fdBoard       - board file  descriptor
   \param chip_num      - chip number

   \return IFX_SUCCESS / IFX_ERROR

   \remark
      If the chip reset was activated before, using this function will clear
      the chip reset so that the chip can be accessed.

   \see
      easy3332_chip_clearreset()
*/
/******************************************************************************/
IFX_int32_t easy3332_chip_clearreset (IFX_uint32_t fdBoard, IFX_uint32_t chip_num)
{
   IFX_int32_t ret;
   EASY3332_Reset_t param;

   memset (&param, 0, sizeof (param));
   param.nChipNum   = chip_num;
   param.nResetMode = EASY3332_RESET_DEACTIVE;

   ret = ioctl (fdBoard, FIO_EASY3332_RESETCHIP, (IFX_uint32_t)&param);

   return ret;
}

/******************************************************************************/
/**
   Get chip parameters according to number

   \param fdBoard       - board file  descriptor
   \param chip_num      - chip number
   \param access_mode   - ptr to chip access mode set physically.
   \param clock_rate    - ptr to chip clock rate
   \param base_address  - ptr to chip base address
   \param irq_num       - ptr to chip irq number

   \return IFX_SUCCESS / IFX_ERROR
*/
/******************************************************************************/
IFX_int32_t easy3332_get_chipparam  (IFX_uint32_t fdBoard, IFX_uint32_t chip_num,
                                    IFX_uint32_t *access_mode,
                                    IFX_uint32_t *clock_rate,
                                    IFX_uint32_t *base_address,
                                    IFX_int32_t  *irq_num)
{
   IFX_int32_t ret;
   EASY3332_GetDevParam_t param;

   memset (&param, 0, sizeof (param));
   param.nChipNum = chip_num;
   ret = ioctl (fdBoard, FIO_EASY3332_GETBOARDPARAMS, (IFX_uint32_t)&param);
   if (ret == IFX_SUCCESS)
   {
      if (access_mode  != NULL) *access_mode  = param.nAccessMode;
      if (clock_rate   != NULL) *clock_rate   = param.nClkRate;
      if (base_address != NULL) *base_address = param.nBaseAddrPhy;
      if (irq_num      != NULL) *irq_num      = param.nIrqNum;
   }

   return ret;
}

/******************************************************************************/
/**
   Set led

   \param fdBoard       - board file  descriptor
   \param led_num       - led number
   \param led_state     - led state

   \return IFX_SUCCESS / IFX_ERROR
*/
/******************************************************************************/
IFX_int32_t easy3332_set_led (IFX_uint32_t fdBoard, IFX_uint32_t led_num,
                             IFX_boolean_t led_state)
{
   IFX_int32_t ret;
   EASY3332_Led_t param;

   memset (&param, 0, sizeof (param));
   param.nLedNum   = led_num;
   param.bLedState = led_state;

   ret = ioctl (fdBoard, FIO_EASY3332_SETLED, (IFX_uint32_t)&param);

   return ret;
}

/******************************************************************************/
/**
   Write register

   \param fdBoard       - board file  descriptor
   \param reg_offset    - register offset
   \param reg_value     - register value

   \return IFX_SUCCESS / IFX_ERROR
*/
/******************************************************************************/
IFX_int32_t easy3332_write_reg (IFX_uint32_t fdBoard, IFX_uint8_t reg_offset,
                               IFX_uint8_t reg_value)
{
   IFX_int32_t ret;
   EASY3332_Reg_t param;

   memset (&param, 0, sizeof (param));
   param.nRegOffset   = reg_offset;
   param.nRegVal      = reg_value;

   ret = ioctl (fdBoard, FIO_EASY3332_CPLDREG_WRITE, (IFX_uint32_t)&param);

   return ret;
}

/******************************************************************************/
/**
   Read register

   \param fdBoard       - board file  descriptor
   \param reg_offset    - register offset
   \param reg_value     - ptr to register value

   \return IFX_SUCCESS / IFX_ERROR
*/
/******************************************************************************/
IFX_int32_t easy3332_read_reg (IFX_uint32_t fdBoard, IFX_uint8_t reg_offset,
                              IFX_uint8_t *reg_value)
{
   IFX_int32_t ret;
   EASY3332_Reg_t param;

   memset (&param, 0, sizeof (param));
   param.nRegOffset   = reg_offset;

   ret = ioctl (fdBoard, FIO_EASY3332_CPLDREG_READ, (IFX_uint32_t)&param);
   if (ret == IFX_SUCCESS)
   {
      if (reg_value != NULL) *reg_value = param.nRegVal;
   }

   return ret;
}

/******************************************************************************/
/**
   Modify register

   \param fdBoard       - board file  descriptor
   \param reg_offset    - register offset
   \param reg_value     - register value

   \return IFX_SUCCESS / IFX_ERROR
*/
/******************************************************************************/
IFX_int32_t easy3332_modify_reg (IFX_uint32_t fdBoard, IFX_uint8_t reg_offset,
                                IFX_uint8_t reg_mask, IFX_uint8_t reg_value)
{
   IFX_int32_t ret;
   EASY3332_Reg_t param;

   memset (&param, 0, sizeof (param));
   param.nRegOffset   = reg_offset;
   param.nRegMask     = reg_mask;
   param.nRegVal      = reg_value;

   ret = ioctl (fdBoard, FIO_EASY3332_CPLDREG_MODIFY, (IFX_uint32_t)&param);

   return ret;
}

/******************************************************************************/
/**
   Sets analog multiplexer

   \param fdBoard       - board file  descriptor
   \param access_mode   - access mode

   \return IFX_SUCCESS / IFX_ERROR
*/
/******************************************************************************/
IFX_int32_t easy3332_set_accessmode   (IFX_uint32_t fdBoard,
                                      IFX_uint32_t access_mode)
{
   IFX_int32_t ret;

   ret = ioctl (fdBoard, FIO_EASY3332_SET_ACCESSMODE, access_mode);

   return ret;
}

/******************************************************************************/
/**
   Sets analog multiplexer

   \param fdBoard       - board file  descriptor
   \param clock_rate    - clock rate

   \return IFX_SUCCESS / IFX_ERROR
*/
/******************************************************************************/
IFX_int32_t easy3332_set_clockrate    (IFX_uint32_t fdBoard,
                                      IFX_uint32_t clock_rate)
{
   IFX_int32_t ret;

   ret = ioctl (fdBoard, FIO_EASY3332_SET_CLOCKRATE, clock_rate);

   return ret;
}

/* ======================================= */
/* Vinetic Driver/Device Control Functions */
/* ======================================= */

/******************************************************************************/
/**
   Initialize Vinetic Control device of given file descriptor

   \param fdVinCtrlDev  - vinetic device file descriptor
   \param access_mode   - vinetic device access mode acc. to VIN_ACCESS enum
   \param base_address  - vinetic device base address
   \param irq_num       - vinetic device irq number

   \return IFX_SUCCESS / IFX_ERROR

   \remark
      The vinetic driver has also virtual access modes which can't be set
      phisically and which are needed for some evaluations. So the vinetic
      access mode is not equal to the physical access mode set.
      If the vinetic access mode isn't explicitely given, a decoding of the
      physical access mode set into a vinetic understandable access mode must
      be done.
*/
/******************************************************************************/
IFX_int32_t vinetic_init_device (IFX_uint32_t fdVinCtrlDev,
                                 VIN_ACCESS   vin_access_mode,
                                 IFX_uint32_t base_address,
                                 IFX_int32_t  irq_num)
{
   IFX_int32_t ret;
   VINETIC_BasicDeviceInit_t param;

   memset (&param, 0, sizeof (param));
   param.AccessMode   = vin_access_mode;
   param.nBaseAddress = base_address;
   param.nIrqNum      = irq_num;

   ret = ioctl (fdVinCtrlDev, FIO_VINETIC_BASICDEV_INIT, (IFX_uint32_t)&param);

   return ret;
}

/******************************************************************************/
/**
   Resets Vinetic Control device of given file descriptor

   \param fdVinCtrlDev  - vinetic device file descriptor

   \return IFX_SUCCESS / IFX_ERROR

   \remark
      This function can be called to internally reset the device within the
      vinetic driver after a hardware reset.
*/
/******************************************************************************/
IFX_int32_t vinetic_reset_device (IFX_uint32_t fdVinCtrlDev)
{
   IFX_int32_t ret;

   ret = ioctl (fdVinCtrlDev, FIO_VINETIC_DEV_RESET, 0);

   return ret;
}
