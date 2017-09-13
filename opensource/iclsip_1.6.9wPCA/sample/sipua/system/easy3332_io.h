#ifndef _EASY3332_IO_H
#define _EASY3332_IO_H
/****************************************************************************
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
 ****************************************************************************
   Module      : easy3332_io.h
   Date        : 2005-06-07
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
     low level access (drv_easy3332_access.c) must be adapted to the system used,
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

/** \defgroup EASY3332_INTERFACE Vinetic EASY3332 Board Driver Interface
    Lists the entire interface to the Board driver
@{  */

/** \defgroup EASY3332_INTERFACE_CONFIG EASY3332 Configuration Interfaces */
/** \defgroup EASY3332_INTERFACE_INIT EASY3332 Initialization Interfaces */
/** \defgroup EASY3332_INTERFACE_REGACCESS EASY3332 Register Access Interfaces */
/** \defgroup EASY3332_INTERFACE_MISC EASY3332 Driver miscelaneous interfaces */
/** \defgroup EASY3332_INTERFACE_OPERATIONAL EASY3332 Driver operational interfaces */


/* ============================= */
/* Global Defines                */
/* ============================= */

/* magic number */
#define EASY3332_IOC_MAGIC 'X'

/** This interface sets the trace/log level.

   \param int The parameter points to the trace/log level to set

   \return none

   \ingroup EASY3332_INTERFACE_MISC

   \code
    // trace level normal
    ioctl(fd, FIO_EASY3332_DEBUGLEVEL, DBG_LEVEL_NORMAL);
   \endcode */
#define FIO_EASY3332_DEBUGLEVEL                       _IO(EASY3332_IOC_MAGIC, 1)

/** This interface provides the EASY3332 Driver version string on request.

   \param char* The parameter points to the version character string

   \return 0 if successful, otherwise -1 in case of an error

   \ingroup EASY3332_INTERFACE_MISC

   \code
   char Version[80];
   int ret;

   ret = ioctl(fd, FIO_EASY3332_GET_VERSION, (int)&Version);
   printf(“\nEASY3332 Driver Version:%s\n”, Version);
   \endcode */
#define FIO_EASY3332_GET_VERSION                      _IO(EASY3332_IOC_MAGIC, 2)

/** This interface stores the configuration needed for initialization
    (access mode, irq mode, clock rate).

   \param EASY3332_Config_t* The parameter points to a
   \ref EASY3332_Config_t structure.

   \return 0 if successful, otherwise -1 in case of an error

   \remarks This interface can be called by the user application to change the
            default configuration. Otherwise, the initialization can be done
            directly using \ref FIO_EASY3332_INIT.

   \ingroup EASY3332_INTERFACE_CONFIG

   \code
     // This example shows how to do a configuration prior to board
     // initialization.
     EASY3332_Config_t param;
     memset (&param, 0, sizeof(EASY3332_Config_t));
     // set access mode
     param.nAccessMode = EASY3332_ACCESS_16BIT_MOTOROLA;
     // set clock rate
     param.nClkRate    = EASY3332_CLK_4096_KHZ;
     // set irq line mode
     param.nIrqMode    = EASY3332_LEVEL_IRQ;
     ret = ioctl(fd, FIO_EASY3332_CONFIG, &param);
   \endcode

   \code
     // This example shows how to set back the default configuration if another
     // configuration was done before.
     ret = ioctl(fd, FIO_EASY3332_CONFIG, 0);
   \endcode */
#define FIO_EASY3332_CONFIG                           _IO(EASY3332_IOC_MAGIC, 3)

/** This interface reads the actual configuration

   \param EASY3332_Config_t* The parameter points to a
   \ref EASY3332_Config_t structure.

   \return 0 if successful, otherwise -1 in case of an error.

   \remark This interface can be used to read the configuration stored with
           \ref FIO_EASY3332_CONFIG.

   \ingroup EASY3332_INTERFACE_CONFIG

   \code
     EASY3332_Config_t param;
     memset (&param, 0, sizeof(EASY3332_Config_t));
     ret = ioctl(fd, FIO_EASY3332_GETCONFIG, &param);
   \endcode */
#define FIO_EASY3332_GETCONFIG                        _IO(EASY3332_IOC_MAGIC, 4)

/** This interface initializes the board (chip select, access mode, clock rate)

   \param int This interface expects no parameter. It should be set to 0.

   \return number of Vinetic chips onboard if successful, otherwise -1 in
           case of an error.

   \remark This interface must be called before the board can be used. In case
           the default configuration is not suitable,
           use \ref FIO_EASY3332_CONFIG to store the configuration parameters.

   \ingroup EASY3332_INTERFACE_INIT

   \code
    ret = ioctl(fd, FIO_EASY3332_INIT, 0);
   \endcode */
#define FIO_EASY3332_INIT                             _IO(EASY3332_IOC_MAGIC, 5)

/** This interface resets the vinetic

   \param EASY3332_Reset_t * The parameter points to a
   \ref EASY3332_Reset_t structure.

   \return 0 if successful, otherwise -1 in case of an error.

   \remark This interface can be called to reset a specific vinetic chip.

   \ingroup EASY3332_INTERFACE_INIT

   \code
   // complete reset of vinetic chip 0
    EASY3332_Reset_t param;

    param.nChipNum = 0;
    param.nResetMode = EASY3332_RESET_ACTIVE_DEACTIVE;
    ret = ioctl(fd, FIO_EASY3332_RESETCHIP, &param);
   \endcode */
#define FIO_EASY3332_RESETCHIP                        _IO(EASY3332_IOC_MAGIC, 6)

/** This interface provides chip key information (base address, irq line) on
    request according to chip number.

   \param EASY3332_GetDevParam_t* The parameter points to a
   \ref EASY3332_GetDevParam_t structure.

   \return 0 if successful, otherwise -1 in case of an error.

   \remark This interface can be called only after the initialization was
           succesfully done using \ref FIO_EASY3332_INIT.

   \ingroup EASY3332_INTERFACE_INIT

   \code
     EASY3332_GetDevParam_t boardParam;
     int chipNum = 0, i;

     chipNum = ioctl(fd, FIO_EASY3332_INIT, 0);
     if (chipNum > 0)
     {
        for (i = 0; i < chipNum; i++)
        {
           boardParam.nChipNum = i;
           ret = ioctl(fd, FIO_EASY3332_GETBOARDPARAMS, &boardParam);
           if (ret == IFX_SUCESS)
           {
              initilize_vinetic_driver (&boardParam);
           }
           else
              break;
        }
     }
   \endcode */
#define FIO_EASY3332_GETBOARDPARAMS                   _IO(EASY3332_IOC_MAGIC, 7)

/** This interface reads a board register according to specified family.

   \param EASY3332_Reg_t* The parameter points to a
   \ref EASY3332_Reg_t structure.

   \return 0 if successful, otherwise -1 in case of an error.

   \remark This interface can be called only after the initialization was
           succesfully done using \ref FIO_EASY3332_INIT.

   \ingroup EASY3332_INTERFACE_REGACCESS

   \code

   \endcode */
#define FIO_EASY3332_CPLDREG_READ                     _IO(EASY3332_IOC_MAGIC, 8)

/** This interface writes a board register according to specified family.

   \param EASY3332_Reg_t* The parameter points to a
   \ref EASY3332_Reg_t structure.

   \return 0 if successful, otherwise -1 in case of an error.

   \remark This interface can be called only after the initialization was
           succesfully done using \ref FIO_EASY3332_INIT.

   \ingroup EASY3332_INTERFACE_REGACCESS

*/
#define FIO_EASY3332_CPLDREG_WRITE                    _IO(EASY3332_IOC_MAGIC, 9)

/** This interface modifies a board register according to specified family.

   \param EASY3332_Reg_t* The parameter points to a
   \ref EASY3332_Reg_t structure.

   \return 0 if successful, otherwise -1 in case of an error.

   \remark This interface can be called only after the initialization was
           succesfully done using \ref FIO_EASY3332_INIT.

   \ingroup EASY3332_INTERFACE_REGACCESS
*/
#define FIO_EASY3332_CPLDREG_MODIFY                  _IO(EASY3332_IOC_MAGIC, 10)

/** This interface sets the specified led.

   \param EASY3332_Led_t* points to a
   \ref EASY3332_Led_t structure.

   \return 0 if successful, otherwise -1 in case of an error.

   \ingroup EASY3332_INTERFACE_OPERATIONAL

   \code
    // This example sets led 0 on.
    EASY3332_Led_t param
    param.nLedNum = 0;
    param.bLedState = 1:
    ret = ioctl(fd, FIO_EASY3332_SETLED, &param);
   \endcode */
#define FIO_EASY3332_SETLED                          _IO(EASY3332_IOC_MAGIC, 11)

/** This interface reads the board version.

   \param int*  points to the board version.

   \return 0 if successful, otherwise -1 in case of an error.

   \ingroup EASY3332_INTERFACE_OPERATIONAL

   \code
    // This example reads the board version.
    IFX_int32_t board_vers;
    ret = ioctl(fd, FIO_EASY3332_GET_BOARDVERS, &board_vers);
   \endcode */
#define FIO_EASY3332_GET_BOARDVERS                   _IO(EASY3332_IOC_MAGIC, 12)

/** This interface sets the access mode with appropriate chip select.

   \param int points to a
   \ref EASY3332_AccessMode_e enum.

   \return 0 if successful, otherwise -1 in case of an error.

   \ingroup EASY3332_INTERFACE_OPERATIONAL

   \code
    // This example sets 16 bit motorola mode.
    ret = ioctl(fd, FIO_EASY3332_SET_ACCESSMODE, EASY3332_ACCESS_16BIT_MOTOROLA);
   \endcode */
#define FIO_EASY3332_SET_ACCESSMODE                  _IO(EASY3332_IOC_MAGIC, 14)

/** This interface sets the clock rate.

   \param int points to a
   \ref EASY3332_ClockRate_e enum.

   \return 0 if successful, otherwise -1 in case of an error.

   \ingroup EASY3332_INTERFACE_OPERATIONAL

   \code
    // This example sets a clockrate of 2 MHz.
    ret = ioctl(fd, FIO_EASY3332_SET_CLOCKRATE, EASY3332_CLK_2048_KHZ);
   \endcode */
#define FIO_EASY3332_SET_CLOCKRATE                   _IO(EASY3332_IOC_MAGIC, 15)

/** @} */

/* ============================= */
/* Global Defines to share with  */
/* application level             */
/* ============================= */

/** cpld version register offset */
#define EASY3332_CPLD_CPLDV_OFF                       0xF0
/** cpld timertick register 1 offset */
#define EASY3332_CPLD_TTREG1_OFF                      0xF1
/** cpld timertick register 2 offset */
#define EASY3332_CPLD_TTREG2_OFF                      0xF2
/** cpld vinetic gpio register offset */
#define EASY3332_CPLD_VINGPIO_OFF                     0xF3
/** cpld slic board id register offset */
#define EASY3332_CPLD_SLIC_ID_OFF                     0xF4
/** cpld slic io direction register offset */
#define EASY3332_CPLD_SLIC_I0DIR_OFF                  0xF6
/** cpld slic io data register offset */
#define EASY3332_CPLD_SLIC_IODATA_OFF                 0xF7
/** cpld led direction register offset */
#define EASY3332_CPLD_LED_DIR_OFF                     0xF9
/** cpld cmd 1 register offset */
#define EASY3332_CPLD_CMDREG1_OFF                     0xFA
/** cpld cmd 2 register offset */
#define EASY3332_CPLD_CMDREG2_OFF                     0xFB
/** cpld cmd 3 register offset */
#define EASY3332_CPLD_CMDREG3_OFF                     0xFC
/** cpld cmd 3 register offset */
#define EASY3332_CPLD_LED_DATA_OFF                    0xFD
/** cpld cmd 5 register offset */
#define EASY3332_CPLD_CMDREG5_OFF                     0xFE


/* ============================= */
/* Global enums                  */
/* ============================= */

/** EASY3332 Board Access Modes
   \ingroup EASY3332_INTERFACE_INIT
   \ingroup EASY3332_INTERFACE_OPERATIONAL
*/
typedef enum
{
   /** 8-bit Motorola parallel access */
   EASY3332_ACCESS_8BIT_MOTOROLA = 0,
   /** 8-bit Intel Mux parallel access */
   EASY3332_ACCESS_8BIT_INTELMUX = 1,
   /** 8-bit Intel Demux parallel access */
   EASY3332_ACCESS_8BIT_INTELDEMUX = 2,
   /** SPI access */
   EASY3332_ACCESS_SPI = 3
} EASY3332_AccessMode_e;

/** EASY3332 Clock rates
   \ingroup EASY3332_INTERFACE_INIT
   \ingroup EASY3332_INTERFACE_OPERATIONAL
*/
typedef enum
{
   /** Clock = 4096 KHz */
   EASY3332_CLK_4096_KHZ = 0,
   /** Clock = 3072 KHz */
   EASY3332_CLK_3072_KHZ = 1,
   /** Clock = 2048 KHz */
   EASY3332_CLK_2048_KHZ = 2,
   /** Clock = 1536 KHz */
   EASY3332_CLK_1536_KHZ = 3,
   /** Clock = 1024 KHz */
   EASY3332_CLK_1024_KHZ = 4,
   /** Clock = 512 KHz */
   EASY3332_CLK_512_KHZ = 5
} EASY3332_ClockRate_e;

/** EASY3332 Reset Modes
   \ingroup EASY3332_INTERFACE_INIT
*/
typedef enum
{
   /** Reset active */
   EASY3332_RESET_ACTIVE = 0,
   /** Reset not active */
   EASY3332_RESET_DEACTIVE = 1,
   /** Reset active and deactive */
   EASY3332_RESET_ACTIVE_DEACTIVE = 2
} EASY3332_ResetMode_e;

/* ============================= */
/* Global Structures             */
/* ============================= */

/** EASY3332 Reset
   \ingroup EASY3332_INTERFACE_INIT
*/
typedef struct
{
   /** Vinetic chip number for which
       the reset is done */
   unsigned int  nChipNum;
   /** Reset Mode according to enum
      \ref EASY3332_ResetMode_e */
   EASY3332_ResetMode_e nResetMode;
} EASY3332_Reset_t;

/** EASY3332 Board minimal configuration
   \ingroup EASY3332_INTERFACE_CONFIG
*/
typedef struct
{
   /** Chip access Mode according to enum
      \ref EASY3332_AccessMode_e */
   EASY3332_AccessMode_e nAccessMode;
   /** Clock rate according to enum
      \ref EASY3332_ClockRate_e */
   EASY3332_ClockRate_e  nClkRate;
} EASY3332_Config_t;

/** EASY3332 Board Information.
   \remark
      Board Information is available only after initialization
   \ingroup EASY3332_INTERFACE_INIT
*/
typedef struct
{
   /** Vinetic chip number for which
       information will be retrieved, in */
   unsigned int  nChipNum;
   /** Chip access Mode according to enum
      \ref EASY3332_AccessMode_e, out */
   EASY3332_AccessMode_e nAccessMode;
   /** Clock rate according to enum
      \ref EASY3332_ClockRate_e, out */
   EASY3332_ClockRate_e  nClkRate;
   /** Physical Base address to access
       the chip, out */
   unsigned long nBaseAddrPhy;
   /** Irq Line on which an interrupt
       handler will be installed, out.

       \remark
        In case -1 is set, polling mode
        will be assumed. */
   int           nIrqNum;
} EASY3332_GetDevParam_t;

/** EASY3332 Board Register access
   \ingroup EASY3332_INTERFACE_REGACCESS
*/
typedef struct
{
   /** Register offset of register to access */
   unsigned char nRegOffset;
   /** Register mask if modify */
   unsigned char nRegMask;
   /** Register value to write / read */
   unsigned char nRegVal;
} EASY3332_Reg_t;

/** EASY3332 Led control
    \ingroup EASY3332_INTERFACE_OPERATIONAL
*/
typedef struct
{
   /** Number of Led to set */
   unsigned int nLedNum;
   /** Led state to set

   - 0 : Led Off
   - 1 : Led  On */
   unsigned int bLedState;
} EASY3332_Led_t;

/* ============================= */
/* Exported Functions            */
/* ============================= */

#endif /* _EASY3332_IO_H */

