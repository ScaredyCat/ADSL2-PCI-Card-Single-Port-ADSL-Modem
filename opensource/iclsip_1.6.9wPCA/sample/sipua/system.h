#ifndef _SYSTEM_H
#define _SYSTEM_H
/****************************************************************************
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
 ****************************************************************************
   Module      : system.h
   Description :
*******************************************************************************/

/* ============================= */
/* Includes                      */
/* ============================= */

#ifdef EASY3332
/** easy3332 includes*/
#include "easy3332_io.h"
#include "lib_easy3332.h"
#endif /* EASY3332 */

#ifdef EASY334
/** easy334 includes*/
#include "easy334_io.h"
#include "lib_easy334.h"
#endif /* EASY334 */

#ifdef YOUR_SYSTEM
/** your system includes*/
#include "your_system_io.h"
#include "your_system_lib.h"
#endif /* YOUR_SYSTEM */

/* ============================= */
/* Global Defines                */
/* ============================= */

/** Generic defines for used system */
#ifdef EASY3332
/* EASY3332 */
#define SYSTEM_DEV                     "/dev/easy3332/0"
#define System_Config(a,b)             easy3332_set_config ((a), (b), \
                                                            EASY3332_CLK_2048_KHZ)
#define System_Init(a,b)               easy3332_init_board ((a),(b))
#define System_SetReset(a,b)           easy3332_chip_setreset((a),(b))
#define System_GetParams(a,b,c,d,e,f)  easy3332_get_chipparam((a),(b),(c),\
                                                              (d),(e),(f))
#define System_GetAccMode(a)           easy3332_b2v_ifce_accessmode((a))
#define System_ClearReset(a,b)         easy3332_chip_clearreset((a),(b))

/* a - fd to sys device,
   b - register offset,
   c - register value to write */
#define System_WriteReg(a, b, c)       easy3332_write_reg((a), (b), (c))
/* a - fd to sys device,
   b - register offset,
   c - pointer to read register value */
#define System_ReadReg(a, b, c)        easy3332_read_reg((a), (b), (c))
#endif /* EASY3332 */

#ifdef EASY334
/* EASY334 */
#define SYSTEM_DEV                     "/dev/easy334/0"
#define System_Config(a,b)             easy334_set_config ((a), (b), \
                                                            EASY334_CLK_2048_KHZ)
#define System_Init(a,b)               easy334_init_board ((a),(b))
#define System_SetReset(a,b)           easy334_chip_setreset((a),(b))
#define System_GetParams(a,b,c,d,e,f)  easy334_get_chipparam((a),(b),(c),\
                                                              (d),(e),(f))
#define System_GetAccMode(a)           easy334_b2v_ifce_accessmode((a))
#define System_ClearReset(a,b)         easy334_chip_clearreset((a),(b))
/* a - fd to sys device,
   b - register offset,
   c - register value to write */
#define System_WriteReg(a, b, c)       easy334_write_reg((a), (b), (c))
/* a - fd to sys device,
   b - register offset,
   c - pointer to read register value */
#define System_ReadReg(a, b, c)        easy334_read_reg((a), (b), (c))
#endif /* EASY334 */

#ifdef VMMC /* For VMMC, this initialization done by BSP. */
/** Generic defines for <your system> */
#define SYSTEM_DEV
#define System_Config(a,b) IFX_SUCCESS
#define System_Init(a,b) vmmc_init_board ((a),(b))
#define System_SetReset(a,b) IFX_SUCCESS
#define System_GetParams(a,b,c,d,e,f) IFX_SUCCESS
#define System_GetAccMode(a) IFX_SUCCESS
#define System_ClearReset(a,b) IFX_SUCCESS
#define SYSTEM_AM_DEFAULT              0
#endif /* VMMC */

#ifdef YOUR_SYSTEM
/** Generic defines for <your system> */
#define SYSTEM_DEV                     "/dev/null"
#define System_Config(a,b)             IFX_ERROR
#define System_Init(a,b)               IFX_ERROR
#define System_SetReset(a,b)           IFX_ERROR
#define System_GetParams(a,b,c,d,e,f)  IFX_ERROR
#define System_GetAccMode(a)           IFX_ERROR
#define System_ClearReset(a,b)         IFX_ERROR
/* a - fd to sys device,
   b - register offset,
   c - register value to write */
#define System_WriteReg(a, b, c)       IFX_ERROR
/* a - fd to sys device,
   b - register offset,
   c - pointer to read register value */
#define System_ReadReg(a, b, c)        IFX_ERROR
#endif /* YOUR_SYSTEM */


/* specify no separator for the last table entry */
#undef SYS_NOSEPARATOR
#undef SYS_SEPARATOR
#undef SYS_EMPTY
#undef sys_am
#define SYS_EMPTY          0
#define SYS_NOSEPARATOR
#define SYS_SEPARATOR      ,
#define sys_am(generic,system,separator) generic=system separator

/** Supported system access modes */

#ifdef EASY3332
/* EASY3332 */
#define SYSTEM_AM_DEFAULT              SYSTEM_AM_8MOT
#define SYSTEM_AM_UNSUPPORTED          SYSTEM_AM_8SPI + 1
#define SYS_ACCESS_MODE \
        sys_am(SYSTEM_AM_8MOT,EASY3332_ACCESS_8BIT_MOTOROLA,SYS_SEPARATOR)\
        sys_am(SYSTEM_AM_8INTMUX,EASY3332_ACCESS_8BIT_INTELMUX,SYS_SEPARATOR)\
        sys_am(SYSTEM_AM_8INTDEMUX,EASY3332_ACCESS_8BIT_INTELDEMUX,SYS_SEPARATOR)\
        sys_am(SYSTEM_AM_8SPI,EASY3332_ACCESS_SPI,SYS_NOSEPARATOR)
#endif /* EASY3332 */

#ifdef EASY334
/* EASY334 */
#define SYSTEM_AM_DEFAULT              SYSTEM_AM_16MOT
#define SYSTEM_AM_UNSUPPORTED          SYSTEM_AM_8SPI + 1
#define SYS_ACCESS_MODE \
        sys_am(SYSTEM_AM_16MOT,EASY334_ACCESS_16BIT_MOTOROLA,SYS_SEPARATOR)\
        sys_am(SYSTEM_AM_16INTMUX,EASY334_ACCESS_16BIT_INTELMUX,SYS_SEPARATOR)\
        sys_am(SYSTEM_AM_16INTDEMUX,EASY334_ACCESS_16BIT_INTELDEMUX,SYS_SEPARATOR)\
        sys_am(SYSTEM_AM_8MOT,EASY334_ACCESS_8BIT_MOTOROLA,SYS_SEPARATOR)\
        sys_am(SYSTEM_AM_8INTMUX,EASY334_ACCESS_8BIT_INTELMUX,SYS_SEPARATOR)\
        sys_am(SYSTEM_AM_8INTDEMUX,EASY334_ACCESS_8BIT_INTELDEMUX,SYS_SEPARATOR)\
        sys_am(SYSTEM_AM_8SPI,EASY334_ACCESS_SPI,SYS_NOSEPARATOR)
#endif /* EASY334 */

#ifdef YOUR_SYSTEM
/* YOUR_SYSTEM */
#define SYSTEM_AM_DEFAULT              SYSTEM_AM_16MOT
#define SYSTEM_AM_UNSUPPORTED          SYSTEM_AM_8SPI + 1
#define SYS_ACCESS_MODE \
        sys_am(SYSTEM_AM_16MOT,SYS_EMPTY,SYS_SEPARATOR)\
        sys_am(SYSTEM_AM_16INTMUX,SYS_EMPTY,SYS_SEPARATOR)\
        sys_am(SYSTEM_AM_16INTDEMUX,SYS_EMPTY,SYS_SEPARATOR)\
        sys_am(SYSTEM_AM_8MOT,SYS_EMPTY,SYS_SEPARATOR)\
        sys_am(SYSTEM_AM_8INTMUX,SYS_EMPTY,SYS_SEPARATOR)\
        sys_am(SYSTEM_AM_8INTDEMUX,SYS_EMPTY,SYS_SEPARATOR)\
        sys_am(SYSTEM_AM_8SPI,SYS_EMPTY,SYS_NOSEPARATOR)
#endif /* YOUR_SYSTEM */

/* access mode types */

typedef enum _sys_acc_mode
{
   SYS_ACCESS_MODE
} sys_acc_mode_t;

extern const IFX_char_t  *access_mode_strings[];
extern const IFX_uint8_t access_mode_val[];

#define PRINT_USED_ACCESS_MODE(am) do {\
   printf("Used access mode : %d => %s\n\r", access_mode_val[(am)],\
           access_mode_strings[(am)]);\
} while(0);

/* ============================= */
/* Global Structures and enums   */
/* ============================= */


/** Supported system registers */

#ifdef EASY3332
/* EASY3332 */

/* offset of EASY3332 register used to setup PCM, cpld cmd 1 register offset */
#define CMDREG1_OFF                    EASY3332_CPLD_CMDREG1_OFF
#define CMDREG2_OFF                    EASY3332_CPLD_CMDREG2_OFF
#define CMDREG3_OFF                    EASY3332_CPLD_CMDREG3_OFF
#define CMDREG4_OFF                    EASY3332_CPLD_CMDREG4_OFF
#define CMDREG5_OFF                    EASY3332_CPLD_CMDREG5_OFF

/* define if master - 1 or slave - 0 */
enum { MASTER = 0x010 }; /* bit 4 */
/* master will have clock OUT - 1, slave will have clock IN - 0 */
enum { PCMEN_MASTER = 0x020}; /* bit 5 */
/* both will have 1 */
enum { PCMON = 0x040}; /* bit 6 */


#endif /* EASY3332 */

#ifdef EASY334
/* EASY334 */


/** offset of EASY334 register used to setup PCM, cpld cmd 3 register offset */
#define CMDREG1_OFF                    EASY334_CPLD_CMDREG3_OFF
#define CMDREG2_OFF                    EASY334_CPLD_CMDREG2_OFF
#define CMDREG3_OFF                    EASY334_CPLD_CMDREG3_OFF
#define CMDREG4_OFF                    EASY334_CPLD_CMDREG4_OFF
#define CMDREG5_OFF                    EASY334_CPLD_CMDREG5_OFF

/** define if master - 1 or slave - 0 */
enum { MASTER = 0x010 }; /* bit 4 of register CMDREG3 */
/** Vinetic reset not active */
enum { VRES = 0x01 };
/** Which highway is selected A or B */
enum
{
   HIGHWAY_A = 0x40,
   HIGHWAY_B = 0x0
};
/** Micro controller via CPLD or MPC ~ VINETIC */
enum
{
   MICRO_CONTROLER_CPLD = 0x0,
   MICRO_CONTROLER_MPC_VIN = 0x20
};
/** Access mode with PCM */
enum
{
   MOTOR_16_BIT_AND_PCM = 0x0C,
   MOTOR_8_BIT_AND_PCM = 0x04,
   INTEL_16_BIT_DEMUX_AND_PCM = 0x0B,
   INTEL_8_BIT_DEMUX_AND_PCM = 0x03,
   INTEL_16_BIT_MUX_AND_PCM = 0x0A,
   INTEL_8_BIT_MUX_AND_PCM = 0x02
};
/** Used to clear bits in register */
enum { ACCESS_MODE_MASK = 0x0F };
/** How MCLK is defined */
enum
{
   MCLK_IS_DCL = 0x0,
   MCLK_SEL_IN_CMDREG5 = 0x80
};
/** Is IOM-2 connector clock input or output */ 
enum
{
   IOM_2_CLK_IN = 0x0,
   IOM_2_CLK_OUT = 0x20
};
/** Frequencies for clock generator */
enum
{
   FREQ_4096_KHZ_8_KHZ = 0x0,
   FREQ_2048_KHZ_8_KHZ = 0x1,
   FREQ_1536_KHZ_8_KHZ = 0x2,
   FREQ_8192_KHZ_8_KHZ = 0x3,
   FREQ_3072_KHZ_8_KHZ = 0x4,
   FREQ_1024_KHZ_8_KHZ = 0x5,
   FREQ_512_KHZ_8_KHZ = 0x6
};
/** Used to clear bits in register */
enum { FREQ_MASK = 0x0F };

/*
Master:
CMDREG1:                  - bit 7..5: Control bits for analog multiplexer
                                      (Line test support)
                                     000 - Voltage on RING channel B is
                                           measured
                                     001 - VBATH is measured
                                     ... - Just measuring
                          - bit 4: always 0
                          - bit 3: always 0
                          - bit 0: 1 - VRES, Vinetic Reset not active
                                   0 - VRES, Vinetic Reset active
CMDREG2: 0x2c (0010 1100) - bit 6: 0 - highway B conn to MPC, A select for
                                       measurments
                                   1 - highway A conn to MPC, B select for
                                       measurments
                            bit 5: 0 - micro controller interface conn via
                                       CPLD
                                   1 -  -||-                      conn 
                                       directly between MPC and VINETIC 
                            bit 3..0: 1100 - 16 bit Motor. mode + PCM
                                      0100 - 8 bit Motor. mode + PCM
                                      1011 - 16 bit Intel Demux. mode + PCM
                                      0011 - 8 bit Intel Demux. mode + PCM
                                      1010 - 16 bit Intel Multip. mode + PCM
                                      0010 - 8 bit Intel Multip. mode + PCM
                                      0001 - SCI + PCM
                                      1111 - IOM-2

CMDREG3: 0x31 (0011 0001) - bit 7: 0 - MCLK = DCL
                                   1 - MCLK selected in CMDREG5
                            bit 5: 0 - IOM-2 Connector is clock input
                                   1 -   -||-                   output
                            bit 4: 0 - Board in slave mode
                                   1 - Board in master mode
                            bit 3..0: Clock generator
                                              PCLK = DCL = PDC, PFS = FSC
                                      0000 -  4.096 MHz, 8kHz
                                      0001 -  2.048 MHz, 8kHz
                                      0010 -  1.536, 8
                                      0011 -  8.192, 8
                                      0100 -  3.072, 8
                                      0101 -  1.024, 8
                                      0110 -  512, 8
                                      ostalo reserved 
CMDREG4: 0x0 - Controls LEDs 0 off, 1 on :-)
CMDREG5: 0x0 - bit 7..5 - Master Clock Selection (CMDREG3, bit 7 must be 1)
                          000 - MCLK = 4.096 MHz
                          ...
               Other bits are used for DEBUG, HDLC, JTAG interfaces.

The configuration I am using for clock master mode:
CMDREG1: 0x01
CMDREG2: 0x2C
CMDREG3: 0x31
CMDREG4: 0x00
CMDREG5: 0x00
Slave:
CMDREG1: 0x01
CMDREG2: 0x2C
CMDREG3: 0x21
CMDREG4: 0x0
CMDREG5: 0x0
*/

#endif /* EASY334 */

#ifdef YOUR_SYSTEM
/* YOUR_SYSTEM */


#endif /* YOUR_SYSTEM */


#endif /* _SYSTEM_H */
