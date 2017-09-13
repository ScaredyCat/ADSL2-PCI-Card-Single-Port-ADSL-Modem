#ifndef _EASY334_IO_H
#define _EASY334_IO_H
/****************************************************************************
       Copyright (c) 2004, Infineon Technologies.  All rights reserved.

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
 ****************************************************************************
   Module      : easy334_io.h
   Date        : 2004-10-25
   Description : This io file contains structures, typedefs and defines shared
                 which other software modules (application code, other drivers,
                 ...).
*******************************************************************************/

/** \file
    This io file contains board driver related structures, typedefs and defines
    shared which other software modules (application code, other drivers, ...).
*/
/** \mainpage Vinetic Board Driver.

    \section s_one 1) Generalities

     This is the documentation of the board driver template for vinetic systems.
     The source code is written in a portable way and script files to generate
     the source code for a specific system are provided. In this case, only the
     low level access (drv_easy334_access.c) must be adapted to the system used,
     unless the developer needs new ioctls which aren't implemented.

    \section s_two 2) Concept and Architecture

     The idea of implementing a board driver is to lower the complexity of the
     proper vinetic driver by separating it from board related stuff, so that
     it can be reusable with \e very \e less \e portability \e efforts.

     As the vinetic chip is used on several, very different boards,
     the following concept applies:

     \li Only one Vinetic driver \e freed from Board Support and containing
         \e only Vinetic Intelligence
     \li One Board Driver supporting the plattform used and interacting with
         the vinetic driver, containing the board intelligence.
     \li User interfaces playing the mediator between the both drivers

      The following figure outlines the overall vinetic software system
      architecture.\n\n

      \image html SofwareSystemArch.emf

    \section s_three 3) Operating systems supported

    -# LINUX
    -# VXWORKS
*/

/** \defgroup EASY334_INTERFACE Vinetic EASY334 Board Driver Interface
    Lists the entire interface to the Board driver
@{  */

/** \defgroup EASY334_INTERFACE_CONFIG EASY334 Configuration Interfaces */
/** \defgroup EASY334_INTERFACE_INIT EASY334 Initialization Interfaces */
/** \defgroup EASY334_INTERFACE_REGACCESS EASY334 Register Access Interfaces */
/** \defgroup EASY334_INTERFACE_MISC EASY334 Driver miscelaneous interfaces */
/** \defgroup EASY334_INTERFACE_OPERATIONAL EASY334 Driver operational interfaces */


/* ============================= */
/* Global Defines                */
/* ============================= */

/* magic number */
#define EASY334_IOC_MAGIC 'X'

/** This interface sets the trace/log level.

   \param int The parameter points to the trace/log level to set

   \return none

   \ingroup EASY334_INTERFACE_MISC

   \code
    // trace level normal
    ioctl(fd, FIO_EASY334_DEBUGLEVEL, DBG_LEVEL_NORMAL);
   \endcode */
#define FIO_EASY334_DEBUGLEVEL                         _IO(EASY334_IOC_MAGIC, 1)

/** This interface provides the EASY334 Driver version string on request.

   \param char* The parameter points to the version character string

   \return 0 if successful, otherwise -1 in case of an error

   \ingroup EASY334_INTERFACE_MISC

   \code
   char Version[80];
   int ret;

   ret = ioctl(fd, FIO_EASY334_GET_VERSION, (int)&Version);
   printf(“\nEASY334 Driver Version:%s\n”, Version);
   \endcode */
#define FIO_EASY334_GET_VERSION                        _IO(EASY334_IOC_MAGIC, 2)

/** This interface stores the configuration needed for initialization
    (access mode, irq mode, clock rate).

   \param EASY334_Config_t* The parameter points to a
   \ref EASY334_Config_t structure.

   \return 0 if successful, otherwise -1 in case of an error

   \remarks This interface can be called by the user application to change the
            default configuration. Otherwise, the initialization can be done
            directly using \ref FIO_EASY334_INIT.

   \ingroup EASY334_INTERFACE_CONFIG

   \code
     // This example shows how to do a configuration prior to board
     // initialization.
     EASY334_Config_t param;
     memset (&param, 0, sizeof(EASY334_Config_t));
     // set access mode
     param.nAccessMode = EASY334_ACCESS_16BIT_MOTOROLA;
     // set clock rate
     param.nClkRate    = EASY334_CLK_4096_KHZ;
     // set irq line mode
     param.nIrqMode    = EASY334_LEVEL_IRQ;
     ret = ioctl(fd, FIO_EASY334_CONFIG, &param);
   \endcode

   \code
     // This example shows how to set back the default configuration if another
     // configuration was done before.
     ret = ioctl(fd, FIO_EASY334_CONFIG, 0);
   \endcode */
#define FIO_EASY334_CONFIG                             _IO(EASY334_IOC_MAGIC, 3)

/** This interface reads the actual configuration

   \param EASY334_Config_t* The parameter points to a
   \ref EASY334_Config_t structure.

   \return 0 if successful, otherwise -1 in case of an error.

   \remark This interface can be used to read the configuration stored with
           \ref FIO_EASY334_CONFIG.

   \ingroup EASY334_INTERFACE_CONFIG

   \code
     EASY334_Config_t param;
     memset (&param, 0, sizeof(EASY334_Config_t));
     ret = ioctl(fd, FIO_EASY334_GETCONFIG, &param);
   \endcode */
#define FIO_EASY334_GETCONFIG                          _IO(EASY334_IOC_MAGIC, 4)

/** This interface initializes the board (chip select, access mode, clock rate)

   \param int This interface expects no parameter. It should be set to 0.

   \return number of Vinetic chips onboard if successful, otherwise -1 in
           case of an error.

   \remark This interface must be called before the board can be used. In case
           the default configuration is not suitable,
           use \ref FIO_EASY334_CONFIG to store the configuration parameters.

   \ingroup EASY334_INTERFACE_INIT

   \code
    ret = ioctl(fd, FIO_EASY334_INIT, 0);
   \endcode */
#define FIO_EASY334_INIT                               _IO(EASY334_IOC_MAGIC, 5)

/** This interface resets the vinetic

   \param EASY334_Reset_t * The parameter points to a
   \ref EASY334_Reset_t structure.

   \return 0 if successful, otherwise -1 in case of an error.

   \remark This interface can be called to reset a specific vinetic chip.

   \ingroup EASY334_INTERFACE_INIT

   \code
   // complete reset of vinetic chip 0
    EASY334_Reset_t param;

    param.nChipNum = 0;
    param.nResetMode = EASY334_RESET_ACTIVE_DEACTIVE;
    ret = ioctl(fd, FIO_EASY334_RESETCHIP, &param);
   \endcode */
#define FIO_EASY334_RESETCHIP                          _IO(EASY334_IOC_MAGIC, 6)

/** This interface provides chip key information (base address, irq line) on
    request according to chip number.

   \param EASY334_GetDevParam_t* The parameter points to a
   \ref EASY334_GetDevParam_t structure.

   \return 0 if successful, otherwise -1 in case of an error.

   \remark This interface can be called only after the initialization was
           succesfully done using \ref FIO_EASY334_INIT.

   \ingroup EASY334_INTERFACE_INIT

   \code
     EASY334_GetDevParam_t boardParam;
     int chipNum = 0, i;

     chipNum = ioctl(fd, FIO_EASY334_INIT, 0);
     if (chipNum > 0)
     {
        for (i = 0; i < chipNum; i++)
        {
           boardParam.nChipNum = i;
           ret = ioctl(fd, FIO_EASY334_GETBOARDPARAMS, &boardParam);
           if (ret == IFX_SUCESS)
           {
              initilize_vinetic_driver (&boardParam);
           }
           else
              break;
        }
     }
   \endcode */
#define FIO_EASY334_GETBOARDPARAMS                     _IO(EASY334_IOC_MAGIC, 7)

/** This interface reads a board register according to specified family.

   \param EASY334_Reg_t* The parameter points to a
   \ref EASY334_Reg_t structure.

   \return 0 if successful, otherwise -1 in case of an error.

   \remark This interface can be called only after the initialization was
           succesfully done using \ref FIO_EASY334_INIT.

   \ingroup EASY334_INTERFACE_REGACCESS

   \code

   \endcode */
#define FIO_EASY334_CPLDREG_READ                       _IO(EASY334_IOC_MAGIC, 8)

/** This interface writes a board register according to specified family.

   \param EASY334_Reg_t* The parameter points to a
   \ref EASY334_Reg_t structure.

   \return 0 if successful, otherwise -1 in case of an error.

   \remark This interface can be called only after the initialization was
           succesfully done using \ref FIO_EASY334_INIT.

   \ingroup EASY334_INTERFACE_REGACCESS

*/
#define FIO_EASY334_CPLDREG_WRITE                      _IO(EASY334_IOC_MAGIC, 9)

/** This interface modifies a board register according to specified family.

   \param EASY334_Reg_t* The parameter points to a
   \ref EASY334_Reg_t structure.

   \return 0 if successful, otherwise -1 in case of an error.

   \remark This interface can be called only after the initialization was
           succesfully done using \ref FIO_EASY334_INIT.

   \ingroup EASY334_INTERFACE_REGACCESS
*/
#define FIO_EASY334_CPLDREG_MODIFY                    _IO(EASY334_IOC_MAGIC, 10)

/** This interface sets the specified led.

   \param EASY334_Led_t* points to a
   \ref EASY334_Led_t structure.

   \return 0 if successful, otherwise -1 in case of an error.

   \ingroup EASY334_INTERFACE_OPERATIONAL

   \code
    // This example sets led 0 on.
    EASY334_Led_t param
    param.nLedNum = 0;
    param.bLedState = 1:
    ret = ioctl(fd, FIO_EASY334_SETLED, &param);
   \endcode */
#define FIO_EASY334_SETLED                            _IO(EASY334_IOC_MAGIC, 11)

/** This interface reads the board version.

   \param int*  points to the board version.

   \return 0 if successful, otherwise -1 in case of an error.

   \ingroup EASY334_INTERFACE_OPERATIONAL

   \code
    // This example reads the board version.
    IFX_int32_t board_vers;
    ret = ioctl(fd, FIO_EASY334_GET_BOARDVERS, &board_vers);
   \endcode */
#define FIO_EASY334_GET_BOARDVERS                     _IO(EASY334_IOC_MAGIC, 12)

/** This interface sets the external analog multiplexer.

   \param int points to a
   \ref EASY334_AnalogMux_e enum.

   \return 0 if successful, otherwise -1 in case of an error.

   \ingroup EASY334_INTERFACE_OPERATIONAL

   \code
    // This example sets the analog multiplexer for VBATH measurement.
    ret = ioctl(fd, FIO_EASY334_SET_ANALOGMUX, EASY334_MUX_VBATH);
   \endcode */
#define FIO_EASY334_SET_ANALOGMUX                     _IO(EASY334_IOC_MAGIC, 13)

/** This interface sets the access mode with appropriate chip select.

   \param int points to a
   \ref EASY334_AccessMode_e enum.

   \return 0 if successful, otherwise -1 in case of an error.

   \ingroup EASY334_INTERFACE_OPERATIONAL

   \code
    // This example sets 16 bit motorola mode.
    ret = ioctl(fd, FIO_EASY334_SET_ACCESSMODE, EASY334_ACCESS_16BIT_MOTOROLA);
   \endcode */
#define FIO_EASY334_SET_ACCESSMODE                    _IO(EASY334_IOC_MAGIC, 14)

/** This interface sets the clock rate.

   \param int points to a
   \ref EASY334_ClockRate_e enum.

   \return 0 if successful, otherwise -1 in case of an error.

   \ingroup EASY334_INTERFACE_OPERATIONAL

   \code
    // This example sets a clockrate of 2 MHz.
    ret = ioctl(fd, FIO_EASY334_SET_CLOCKRATE, EASY334_CLK_2048_KHZ);
   \endcode */
#define FIO_EASY334_SET_CLOCKRATE                     _IO(EASY334_IOC_MAGIC, 15)

/** @} */

/* ============================= */
/* Global Defines to share with  */
/* application level             */
/* ============================= */

/** cpld cmd 1 register offset */
#define EASY334_CPLD_CMDREG1_OFF                     0xFE
/** cpld cmd 2 register offset */
#define EASY334_CPLD_CMDREG2_OFF                     0xFD
/** cpld cmd 3 register offset */
#define EASY334_CPLD_CMDREG3_OFF                     0xFC
/** cpld cmd 4 register offset */
#define EASY334_CPLD_CMDREG4_OFF                     0xFB
/** cpld cmd 5 register offset */
#define EASY334_CPLD_CMDREG5_OFF                     0xF8
/** cpld version register offset */
#define EASY334_CPLD_CMDVERS_OFF                     0xFA
/** cpld slid register offset */
#define EASY334_CPLD_CMDSLID_OFF                     0xF9

/* ============================= */
/* Global enums                  */
/* ============================= */

/** EASY334 Board Access Modes
   \ingroup EASY334_INTERFACE_INIT
   \ingroup EASY334_INTERFACE_OPERATIONAL
*/
typedef enum
{
   /** 16-bit Motorola parallel access */
   EASY334_ACCESS_16BIT_MOTOROLA = 0,
   /** 16-bit Intel Mux parallel access */
   EASY334_ACCESS_16BIT_INTELMUX = 1,
   /** 16-bit Intel Demux parallel access */
   EASY334_ACCESS_16BIT_INTELDEMUX = 2,
   /** 8-bit Motorola parallel access */
   EASY334_ACCESS_8BIT_MOTOROLA = 3,
   /** 8-bit Intel Mux parallel access */
   EASY334_ACCESS_8BIT_INTELMUX = 4,
   /** 8-bit Intel Demux parallel access */
   EASY334_ACCESS_8BIT_INTELDEMUX = 5,
   /** SPI access */
   EASY334_ACCESS_SPI = 6
} EASY334_AccessMode_e;

/** EASY334 Clock rates
   \ingroup EASY334_INTERFACE_INIT
   \ingroup EASY334_INTERFACE_OPERATIONAL
*/
typedef enum
{
   /** Clock = 8192 KHz */
   EASY334_CLK_8192_KHZ = 0,
   /** Clock = 4096 KHz */
   EASY334_CLK_4096_KHZ = 1,
   /** Clock = 3072 KHz */
   EASY334_CLK_3072_KHZ = 2,
   /** Clock = 2048 KHz */
   EASY334_CLK_2048_KHZ = 3,
   /** Clock = 1536 KHz */
   EASY334_CLK_1536_KHZ = 4,
   /** Clock = 1024 KHz */
   EASY334_CLK_1024_KHZ = 5,
   /** Clock = 512 KHz */
   EASY334_CLK_512_KHZ = 6
} EASY334_ClockRate_e;

/** EASY334 Reset Modes
   \ingroup EASY334_INTERFACE_INIT
*/
typedef enum
{
   /** Reset active */
   EASY334_RESET_ACTIVE = 0,
   /** Reset not active */
   EASY334_RESET_DEACTIVE = 1,
   /** Reset active and deactive */
   EASY334_RESET_ACTIVE_DEACTIVE = 2
} EASY334_ResetMode_e;

/** EASY334 Analog Multiplexer options for line testing support.
   \ingroup EASY334_INTERFACE_OPERATIONAL
*/
typedef enum
{
   /** Voltage on Ring Channel B */
   EASY334_MUX_RESET = 0,
   /** VBatH */
   EASY334_MUX_VBATH = 1,
   /** VBatL */
   EASY334_MUX_VBATL = 2,
   /** VBatR */
   EASY334_MUX_VBATR = 3,
   /** VHR */
   EASY334_MUX_VHR = 4,
   /** GND */
   EASY334_MUX_GND = 5,
   /** Tip/Ring Potential Difference */
   EASY334_MUX_TIPRING = 6,
   /** VCMS */
   EASY334_MUX_VCMS = 7
} EASY334_AnalogMux_e;

/* ============================= */
/* Global Structures             */
/* ============================= */

/** EASY334 Reset
   \ingroup EASY334_INTERFACE_INIT
*/
typedef struct
{
   /** Vinetic chip number for which
       the reset is done */
   unsigned int  nChipNum;
   /** Reset Mode according to enum
      \ref EASY334_ResetMode_e */
   EASY334_ResetMode_e nResetMode;
} EASY334_Reset_t;

/** EASY334 Board minimal configuration
   \ingroup EASY334_INTERFACE_CONFIG
*/
typedef struct
{
   /** Chip access Mode according to enum
      \ref EASY334_AccessMode_e */
   EASY334_AccessMode_e nAccessMode;
   /** Clock rate according to enum
      \ref EASY334_ClockRate_e */
   EASY334_ClockRate_e  nClkRate;
} EASY334_Config_t;

/** EASY334 Board Information.
   \remark
      Board Information is available only after initialization
   \ingroup EASY334_INTERFACE_INIT
*/
typedef struct
{
   /** Vinetic chip number for which
       information will be retrieved, in */
   unsigned int  nChipNum;
   /** Chip access Mode according to enum
      \ref EASY334_AccessMode_e, out */
   EASY334_AccessMode_e nAccessMode;
   /** Clock rate according to enum
      \ref EASY334_ClockRate_e, out */
   EASY334_ClockRate_e  nClkRate;
   /** Physical Base address to access
       the chip, out */
   unsigned long nBaseAddrPhy;
   /** Irq Line on which an interrupt
       handler will be installed, out.

       \remark
        In case -1 is set, polling mode
        will be assumed. */
   int           nIrqNum;
} EASY334_GetDevParam_t;

/** EASY334 Board Register access
   \ingroup EASY334_INTERFACE_REGACCESS
*/
typedef struct
{
   /** Register offset of register to access */
   unsigned char nRegOffset;
   /** Register mask if modify */
   unsigned char nRegMask;
   /** Register value to write / read */
   unsigned char nRegVal;
} EASY334_Reg_t;

/** EASY334 Led control
    \ingroup EASY334_INTERFACE_OPERATIONAL
*/
typedef struct
{
   /** Number of Led to set */
   unsigned int nLedNum;
   /** Led state to set

   - 0 : Led Off
   - 1 : Led  On */
   unsigned int bLedState;
} EASY334_Led_t;

/* ============================= */
/* Exported Functions            */
/* ============================= */

#endif /* _EASY334_IO_H */

