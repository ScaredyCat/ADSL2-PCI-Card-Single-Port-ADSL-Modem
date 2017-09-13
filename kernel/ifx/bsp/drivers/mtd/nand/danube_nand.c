#if 1
/*
 * ########################################################################
 *
 *  This program is free software; you can distribute it and/or modify it
 *  under the terms of the GNU General Public License (Version 2) as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
 *
 * ########################################################################
 *
 * danube_nand.c
 *
 *  Description:
 *    device driver of NAND controller of Danube chip
 *  Author:
 *    Samuels Xu Liang
 *  Created:
 *    6 Sep 2005
 *  History & Modification Tag:
 *  ___________________________________________________________________________
 *  |  Tag   |                  Comments                   | Modifier & Time  |
 *  |--------+---------------------------------------------+------------------|
 *  |  S0.0  | First version of this driver and the tag is | Samuels Xu Liang |
 *  |        | implied.                                    |    6 Jul 2005    |
 *  ---------------------------------------------------------------------------
 *
 */


/*
 * ####################################
 *              Head File
 * ####################################
 */

/*
 *  Common Head File
 */
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/mtd/partitions.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <asm/unistd.h>
#include <asm/io.h>
#include <asm/delay.h>
#include <linux/errno.h>

/*
 *  Chip Specific Head File
 */
#if 0
  #include <asm/danube/danube.h>
  #undef DANUBE_EBU
  #undef DANUBE_EBU_CON
#endif


/*
 * ####################################
 *     Parameters to Configure NAND
 * ####################################
 */
#ifdef MODULE
    static int  ebu_address_region  = 1;    /*  number of address region (0-4)      */
    static int  ebu_config          = 1;    /*  1 to config EBU, else to do nothing */

    MODULE_PARM(ebu_address_region, "i");
    MODULE_PARM_DESC(ebu_address_region, "Number of address region (0-4).");
    MODULE_PARM(ebu_config, "i");
    MODULE_PARM_DESC(ebu_config, "Set 1 to config EBU, else to do nothing.");
#else
    #define ebu_address_region          1
    #define ebu_config                  1
#endif

#if 0
    #define NUM_PARTITIONS              2
    static struct mtd_partition partition_info[NUM_PARTITIONS] = {
        {
            .name       = "kernel image",
            .offset     = MTDPART_OFS_APPEND,
            .size       = 8 * 0x00100000,   /*  8MB (Depends on size of kernel image)   */
            .mask_flags = MTD_WRITEABLE     /*  This partition is NOT writable          */
        },
        {
            .name   = "Root FS (JFFS2)",
            .offset = MTDPART_OFS_NXTBLK,   /*  Start address of next zone              */
            .size   = MTDPART_SIZ_FULL
        },
    };
#else
    #define NUM_PARTITIONS              3
    static struct mtd_partition partition_info[NUM_PARTITIONS] = {
        {
            .name   = "Danube NAND flash partition 0 (16K)",
            .offset = 0x00,
            .size   = 0x4000
        },
        {
            .name   = "Danube NAND flash partition 1 (2.3M)",
            .offset = 8 * 0x00100000 - 2 * 0x00100000,
            .size   = 0x050000 + 2 * 0x00100000
//            .size   = 4 * 0x00100000        /*  4MB (Depends on size of kernel image)   */
        },
        {
            .name   = "Danube NAND flash partition 2 (8M)",
            .offset = MTDPART_OFS_NXTBLK,   /*  Start address of next zone              */
            .size   = MTDPART_SIZ_FULL
        },
    };
#endif


/*
 * ####################################
 *              Definition
 * ####################################
 */

#define LINUX_2_4_31                    1

#define DEBUG_ON_AMAZON                 0

#define DEBUG_ON_VENUS                  0

#define DEBUG_WRITE_GPIO_REGISTER       1

#define FORCE_ECC_ON                    1

#define PANIC_WHEN_REACH_MAX_LOOP_COUNT 1

#define MAX_LOOP_COUNT                  4000

#if (defined(DEBUG_ON_AMAZON) && DEBUG_ON_AMAZON) || (defined(DEBUG_ON_VENUS) && DEBUG_ON_VENUS)
    #undef dbg
    #define dbg(format, arg...)         printk(KERN_DEBUG __FILE__ ":" format "\n", ##arg)

    #define INLINE
#else
    #ifndef dbg
        #define dbg(format, arg...)     do {} while (0)
    #endif

    #define INLINE                      inline
#endif  //  (defined(DEBUG_ON_AMAZON) && DEBUG_ON_AMAZON) || (defined(DEBUG_ON_VENUS) && DEBUG_ON_VENUS)

/*
 *  EBU Regions' Address
 */
#define DANUBE_EBU_ADDR_REGION0         0x10000000
#define DANUBE_EBU_ADDR_REGION1         0x14000000
#define DANUBE_EBU_ADDR_REGION2         0x19000000
#define DANUBE_EBU_ADDR_REGION3         0x1C000000

/*
 *  Bits Operation
 */
#define GET_BITS(x, msb, lsb)           (((x) & ((1 << ((msb) + 1)) - 1)) >> (lsb))
#define SET_BITS(x, msb, lsb, value)    (((x) & ~(((1 << ((msb) + 1)) - 1) ^ ((1 << (lsb)) - 1))) | (((value) & ((1 << (1 + (msb) - (lsb))) - 1)) << (lsb)))

/*
 *  EBU Registers Mapping
 */
#define DANUBE_EBU                      (KSEG1 + 0x1E105300)

#define DANUBE_EBU_ADDR_SEL             ((volatile u32*)(DANUBE_EBU + 0x20 + (ebu_address_region * 4)))
#define DANUBE_EBU_CON                  ((volatile u32*)(DANUBE_EBU + 0x60 + (ebu_address_region * 4)))

#define DANUBE_EBU_ADDR_SELn(n)         ((volatile u32*)(DANUBE_EBU + 0x20 + (((u32)(n) & 0x03) * 4)))

/*
 *  NAND Registers Mapping
 */
#define DANUBE_NAND                     DANUBE_EBU

#define DANUBE_NAND_CON                 ((volatile u32*)(DANUBE_NAND + 0x00B0))
#define DANUBE_NAND_WAIT                ((volatile u32*)(DANUBE_NAND + 0x00B4))
#define DANUBE_NAND_ECC0                ((volatile u32*)(DANUBE_NAND + 0x00B8))
#define DANUBE_NAND_ECC_AC              ((volatile u32*)(DANUBE_NAND + 0x00BC))

/*
 *  NAND IO Port Mapping
 */
#define NAND_CMD_DECODER_ALE(x)         ((x) ? (1 << 2) : 0)
#define NAND_CMD_DECODER_CLE(x)         ((x) ? (1 << 3) : 0)
#define NAND_CMD_DECODER_CS(x)          ((x) ? (1 << 4) : 0)
#define NAND_CMD_DECODER_SE(x)          ((x) ? (1 << 5) : 0)
#define NAND_CMD_DECODER_WP(x)          ((x) ? (1 << 6) : 0)
#define NAND_CMD_DECODER_PRE(x)         ((x) ? (1 << 7) : 0)

#define DANUBE_NAND_CMD                 ((volatile u8*)((u32)ebu_base_address + NAND_CMD_DECODER_ALE(0) + NAND_CMD_DECODER_CLE(1) + NAND_CMD_DECODER_CS(1) + NAND_CMD_DECODER_SE(0) + NAND_CMD_DECODER_WP(0) + NAND_CMD_DECODER_PRE(0)))
#define DANUBE_NAND_WRITE               ((volatile u8*)((u32)ebu_base_address + NAND_CMD_DECODER_ALE(0) + NAND_CMD_DECODER_CLE(0) + NAND_CMD_DECODER_CS(1) + NAND_CMD_DECODER_SE(0) + NAND_CMD_DECODER_WP(0) + NAND_CMD_DECODER_PRE(0)))
#define DANUBE_NAND_ADDR                ((volatile u8*)((u32)ebu_base_address + NAND_CMD_DECODER_ALE(1) + NAND_CMD_DECODER_CLE(0) + NAND_CMD_DECODER_CS(1) + NAND_CMD_DECODER_SE(0) + NAND_CMD_DECODER_WP(0) + NAND_CMD_DECODER_PRE(0)))
#define DANUBE_NAND_READ                ((volatile u8*)((u32)ebu_base_address + NAND_CMD_DECODER_ALE(0) + NAND_CMD_DECODER_CLE(0) + NAND_CMD_DECODER_CS(1) + NAND_CMD_DECODER_SE(0) + NAND_CMD_DECODER_WP(0) + NAND_CMD_DECODER_PRE(0)))

//#define DANUBE_NAND_WRITE_OVER          ((volatile u8*)((u32)ebu_base_address + NAND_CMD_DECODER_ALE(0) + NAND_CMD_DECODER_CLE(0) + NAND_CMD_DECODER_CS(0) + NAND_CMD_DECODER_SE(0) + NAND_CMD_DECODER_WP(0) + NAND_CMD_DECODER_PRE(0)))
#define DANUBE_NAND_ADDR_OVER           ((volatile u8*)((u32)ebu_base_address + NAND_CMD_DECODER_ALE(0) + NAND_CMD_DECODER_CLE(0) + NAND_CMD_DECODER_CS(1) + NAND_CMD_DECODER_SE(0) + NAND_CMD_DECODER_WP(0) + NAND_CMD_DECODER_PRE(0)))
//#define DANUBE_NAND_READ_OVER           ((volatile u8*)((u32)ebu_base_address + NAND_CMD_DECODER_ALE(0) + NAND_CMD_DECODER_CLE(0) + NAND_CMD_DECODER_CS(0) + NAND_CMD_DECODER_SE(0) + NAND_CMD_DECODER_WP(0) + NAND_CMD_DECODER_PRE(0)))

/*
 *  EBU Address Select Register
 */
#define EBU_ADDR_SEL_BASE_SET(value)    (((value) & ((1 << (1 + 31 - 12)) - 1)) << 12)
#define EBU_ADDR_SEL_MASK_SET(value)    SET_BITS(0, 7, 4, (value))
#define EBU_ADDR_SEL_MRME_SET(value)    ((value) ? (1 << 1) : 0)
#define EBU_ADDR_SEL_REGEN_SET(value)   ((value) ? (1 << 0) : 0)

/*
 *  EBU Configuration Register
 */
#define EBU_CON_WRDIS_SET(value)        ((value) ? (1 << 31) : 0)
#define EBU_CON_ADSWP_SET(value)        ((value) ? (1 << 30) : 0)
#define EBU_CON_AGEN_SET(value)         SET_BITS(0, 26, 24, (value))
#define EBU_CON_SETUP_SET(value)        ((value) ? (1 << 22) : 0)
#define EBU_CON_WAIT_SET(value)         SET_BITS(0, 21, 20, (value))
#define EBU_CON_WINV_SET(value)         ((value) ? (1 << 19) : 0)
#define EBU_CON_PW_SET(value)           SET_BITS(0, 17, 16, (value))
#define EBU_CON_ALEC_SET(value)         SET_BITS(0, 15, 14, (value))
#define EBU_CON_BCGEN_SET(value)        SET_BITS(0, 13, 12, (value))
#define EBU_CON_WAITWRC_SET(value)      SET_BITS(0, 10, 8, (value))
#define EBU_CON_WAITRDC_SET(value)      SET_BITS(0, 7, 6, (value))
#define EBU_CON_HOLDC_SET(value)        SET_BITS(0, 5, 4, (value))
#define EBU_CON_RECOVC_SET(value)       SET_BITS(0, 3, 2, (value))
#define EBU_CON_CMULT_SET(value)        SET_BITS(0, 1, 0, (value))

//#define NAND_CE_SET                     *DANUBE_EBU_CON = 0x00F405F3
//#define NAND_CE_CLEAR                   *DANUBE_EBU_CON = 0x000005F3

/*
 *  NAND Flash Controller Control Register
 */
#define NAND_CON_LAT_EN                 GET_BITS(*DANUBE_NAND_CON, 23, 18)

#define NAND_CON_ECC_ON_SET(value)      ((value) ? (1 << 31) : 0)
#define NAND_CON_PRE_LAT_EN_SET(value)  ((value) ? (1 << 23) : 0)
#define NAND_CON_WP_LAT_EN_SET(value)   ((value) ? (1 << 22) : 0)
#define NAND_CON_SE_LAT_EN_SET(value)   ((value) ? (1 << 21) : 0)
#define NAND_CON_CS_LAT_EN_SET(value)   ((value) ? (1 << 20) : 0)
#define NAND_CON_CLE_LAT_EN_SET(value)  ((value) ? (1 << 19) : 0)
#define NAND_CON_ALE_LAT_EN_SET(value)  ((value) ? (1 << 18) : 0)
#define NAND_CON_OUT_CS_S_SET(value)    SET_BITS(0, 11, 10, (value))
#define NAND_CON_IN_CS_S_SET(value)     SET_BITS(0, 9, 8, (value))
#define NAND_CON_PRE_P_SET(value)       ((value) ? (1 << 7) : 0)
#define NAND_CON_WP_P_SET(value)        ((value) ? (1 << 6) : 0)
#define NAND_CON_SE_P_SET(value)        ((value) ? (1 << 5) : 0)
#define NAND_CON_CS_P_SET(value)        ((value) ? (1 << 4) : 0)
#define NAND_CON_CLE_P_SET(value)       ((value) ? (1 << 3) : 0)
#define NAND_CON_ALE_P_SET(value)       ((value) ? (1 << 2) : 0)
#define NAND_CON_CSMUX_E_SET(value)     ((value) ? (1 << 1) : 0)
#define NAND_CON_NANDM_SET(value)       ((value) ? (1 << 0) : 0)

/*
 *  NAND Flash Device RD/BY State Register
 */
#define NAND_WAIT_WR_C                  (*DANUBE_NAND_WAIT & (1 << 3))
//#define NAND_WAIT_RD                    (*DANUBE_NAND_WAIT & (1 << 0))
#define NAND_WAIT_RD                    ((*DANUBE_NAND_WAIT & 0x07) == 0x07)

#define NAND_READY_CLEAR()              do { *DANUBE_NAND_WAIT = 0x00; } while ( 0 )

/*
 *  NAND Flash ECC Register
 */
#define NAND_ECC0_B2                    GET_BITS(*DANUBE_NAND_ECC0, 23, 16)
#define NAND_ECC0_B1                    GET_BITS(*DANUBE_NAND_ECC0, 15, 8)
#define NAND_ECC0_B0                    GET_BITS(*DANUBE_NAND_ECC0, 7, 0)

#define NAND_ECC0_CLEAR()               do { *DANUBE_NAND_ECC0 = 0; } while ( 0 )


/*
 * ####################################
 * Preparation of Debug on Amazon Chip
 * ####################################
 */

/*
 *  If try module on Amazon chip, prepare some tricks to prevent invalid memory write.
 */
#if defined(DEBUG_ON_AMAZON) && DEBUG_ON_AMAZON
#endif  //  defined(DEBUG_ON_AMAZON) && DEBUG_ON_AMAZON


/*
 * ####################################
 *             Declaration
 * ####################################
 */

/*
 *  MTD Device Operations to Replace Default Routines
 */
#if defined(LINUX_2_4_31) && LINUX_2_4_31
  static void danube_hwcontrol(int);
  static int danube_dev_ready(void);
  static void danube_cmdfunc(struct mtd_info *, unsigned, int, int);
  static void danube_calculate_ecc(const u_char *, u_char *);
  static void danube_enable_hwecc(int);
#else
  static void danube_write_byte(struct mtd_info *, u_char);
  static void danube_write_buf(struct mtd_info *, const u_char *, int);
  static void danube_select_chip(struct mtd_info *, int);
  static int danube_dev_ready(struct mtd_info *);
  static void danube_cmdfunc(struct mtd_info *, unsigned, int, int);
  static void danube_calculate_ecc(struct mtd_info *, const u_char *, u_char *);
  static void danube_enable_hwecc(struct mtd_info *, int);
#endif

/*
 *  Init Functions
 */
static INLINE void check_parameter(void);
static INLINE void config_ebu(void);
static INLINE void config_nand(void);

/*
 *  Internal Help Functions
 */
static INLINE void nand_ce_latch(void);
static INLINE void nand_ce_pulse(void);
static INLINE int nand_dev_ready(void);
static INLINE void nand_wait_write_complete(void);
static INLINE void nand_wait_dev_ready(void);


/*
 * ####################################
 *            Local Variable
 * ####################################
 */

static struct mtd_info mtd = {0};
static struct nand_chip chip = {0};
static u_char data_buf[512 + 16];

static int f_mtd_partitions = 0;

static int f_ecc_write = 0;


/*
 * ####################################
 *           Global Variable
 * ####################################
 */

static volatile u8 *ebu_base_address = (volatile u8*)(KSEG1 + DANUBE_EBU_ADDR_REGION1);


/*
 * ####################################
 *            Local Function
 * ####################################
 */

#if defined(LINUX_2_4_31) && LINUX_2_4_31

static void danube_hwcontrol(int cmd)
{
    if ( cmd == NAND_CTL_CLRNCE )
        nand_ce_pulse();
    else if ( cmd == NAND_CTL_SETNCE )
        nand_ce_latch();
}

#else

static void danube_write_byte(struct mtd_info *mtd, u_char byte)
{
  #if defined(MAX_LOOP_COUNT) && MAX_LOOP_COUNT
    int j;
  #endif

    *DANUBE_NAND_WRITE = byte;
  #if defined(MAX_LOOP_COUNT) && MAX_LOOP_COUNT
    for ( j = 0; j < MAX_LOOP_COUNT && !NAND_WAIT_WR_C; j++ );
    #if defined(PANIC_WHEN_REACH_MAX_LOOP_COUNT) && PANIC_WHEN_REACH_MAX_LOOP_COUNT
    if ( j >= MAX_LOOP_COUNT )
        panic("danube_nand: loop is over threshold (danube_write_byte)\n");
    #endif
  #else
    while ( !NAND_WAIT_WR_C );
  #endif

  #if 0
    {
        static int count = 0;

        if ( (count & 0x0F) == 0 )
            printk("  ");
        printk(" %02X", (u32)byte);
        if ( (++count & 0x0F) == 0 )
            printk("\n");
    }
  #endif
}

static void danube_write_buf(struct mtd_info *mtd, const u_char *buf, int len)
{
    int i;
  #if defined(MAX_LOOP_COUNT) && MAX_LOOP_COUNT
    int j;
  #endif

    for (i=0; i<len; i++)
    {
        *DANUBE_NAND_WRITE = buf[i];
  #if defined(MAX_LOOP_COUNT) && MAX_LOOP_COUNT
        for ( j = 0; j < MAX_LOOP_COUNT && !NAND_WAIT_WR_C; j++ );
    #if defined(PANIC_WHEN_REACH_MAX_LOOP_COUNT) && PANIC_WHEN_REACH_MAX_LOOP_COUNT
        if ( j >= MAX_LOOP_COUNT )
            panic("danube_nand: loop is over threshold (danube_write_buf)\n");
    #endif
  #else
        while ( !NAND_WAIT_WR_C );
  #endif

  #if 0
        {
            static int count = 0;

            if ( (count & 0x0F) == 0 )
                printk("  ");
            printk(" %02X", (u32)buf[i]);
            if ( (++count & 0x0F) == 0 )
                printk("\n");
        }
  #endif
    }
}

/*
 *  Description:
 *    Enable or disable NAND flash chip. This is a dummy routine, because the
 *    CE pin is controlled by address bit 4 (A4).
 *  Input:
 *    mtd  --- struct mtd_info *, MTD device to change CE (chip enable) pin.
 *    chip --- int, flag indicate enable or disable chip.
 *  Output:
 *    none
 */
static void danube_select_chip(struct mtd_info *mtd, int chip)
{
//    printk("danube_select_chip(chip = %d)\n", chip);
  #if 1
    if ( chip == -1 )
        nand_ce_pulse();
    else
        nand_ce_latch();
//    printk("DANUBE_NAND_CON = %08X\n", *DANUBE_NAND_CON);
  #else
    /*
     *  Set CS to pulse mode to pull the level and then restore latch mode.
     */
    if ( chip == -1 )
    {
        *DANUBE_NAND_CON &= ~NAND_CON_CS_LAT_EN_SET(1);
        *DANUBE_NAND_CON |= NAND_CON_CS_LAT_EN_SET(1);
    }
  #endif
}

#endif

/*
 *  Description:
 *    Check if the MTD device is ready to accesss.
 *  Input:
 *    mtd --- struct mtd_info *, MTD device to check.
 *  Output:
 *    none
 */
#if defined(LINUX_2_4_31) && LINUX_2_4_31
  static int danube_dev_ready(void)
#else
  static int danube_dev_ready(struct mtd_info *mtd)
#endif
{
//    printk("danube_dev_ready()\n");
    return nand_dev_ready();
}

/*
 *  Description:
 *    Send NAND flash command.
 *  Input:
 *    mtd       --- struct mtd_info *, MTD device to send command to.
 *    command   --- unsigned, command to be sent.
 *    column    --- int, low 8 bit address (A0-A7)
 *    page_addr --- int, high bits of address (A9-A25)
 *  Output:
 *    none
 */
static void danube_cmdfunc(struct mtd_info *mtd, unsigned command, int column, int page_addr)
{
#if 0
    if ( command == NAND_CMD_READID )
    {
        u8 buf[512];
        int i;

        for ( i = 0; i < 256; i++ )
        {
            buf[i] = (u8)i;
            buf[i + 256] = 255;
        }

        mtd->oobblock = 512;
        danube_select_chip(mtd, 0);
  #if 1
        danube_cmdfunc(mtd, 0x60, -1, 0);
  #else
        *DANUBE_NAND_CMD = 0x60;
        while ( !NAND_WAIT_WR_C );
        *DANUBE_NAND_ADDR = 0x00;
        while ( !NAND_WAIT_WR_C );
        *DANUBE_NAND_ADDR = 0x00;
        while ( !NAND_WAIT_WR_C );
        *DANUBE_NAND_ADDR_OVER = 0x00;
        while ( !NAND_WAIT_WR_C );
  #endif
  #if 1
        danube_cmdfunc(mtd, 0xD0, -1, -1);
  #else
        *DANUBE_NAND_CMD = 0xD0;
        while ( !NAND_WAIT_WR_C );
  #endif
        while ( !nand_dev_ready() );
  #if 1
        danube_cmdfunc(mtd, 0x80, 0, 0);
  #else
        *DANUBE_NAND_CMD = 0x80;
        while ( !NAND_WAIT_WR_C );
        *DANUBE_NAND_ADDR = 0x00;
        while ( !NAND_WAIT_WR_C );
        *DANUBE_NAND_ADDR = 0x00;
        while ( !NAND_WAIT_WR_C );
        *DANUBE_NAND_ADDR = 0x00;
        while ( !NAND_WAIT_WR_C );
        *DANUBE_NAND_ADDR_OVER = 0x00;
        while ( !NAND_WAIT_WR_C );
  #endif
  #if 1
        danube_write_buf(mtd, buf, 256);
  #else
        for ( i = 0; i < 528; i++ )
        {
            *DANUBE_NAND_WRITE = (u8)(i & 0xFF);
            while ( !NAND_WAIT_WR_C );
        }
  #endif
  #if 1
        danube_cmdfunc(mtd, 0x10, -1, -1);
  #else

        *DANUBE_NAND_CMD = 0x10;
        while ( !NAND_WAIT_WR_C );
  #endif
        while ( !nand_dev_ready() );
        danube_select_chip(mtd, -1);
    }
#endif

#if 0
    if ( command == NAND_CMD_READID )
    {
        u8 id1, id2;
//        u8 id3, id4;
        u8 buf[1024];
        int i;

//        *DANUBE_NAND_CMD = 0xFF;
//        udelay(1);
//        while ( !danube_dev_ready(NULL) );

//        nand_ce_latch();
//        printk("DANUBE_NAND_CON = %08X\n", *DANUBE_NAND_CON);
        *DANUBE_NAND_CMD = 0x90;
//        __asm__("sync;");
        while ( !NAND_WAIT_WR_C );
//        *DANUBE_NAND_CON &= ~NAND_CON_ALE_LAT_EN_SET(1);
//        printk("DANUBE_NAND_CON = %08X\n", *DANUBE_NAND_CON);
//        udelay(1000);
        *DANUBE_NAND_ADDR = 0x00;
//        __asm__("sync;");
        while ( !NAND_WAIT_WR_C );
//        *DANUBE_NAND_CON &= ~NAND_CON_ALE_LAT_EN_SET(1);
//        *DANUBE_NAND_CON |= NAND_CON_ALE_LAT_EN_SET(1);
//        printk("DANUBE_NAND_CON = %08X\n", *DANUBE_NAND_CON);
//        udelay(1000);
//        nand_ce_latch();
        id1 = *DANUBE_NAND_READ;
//        __asm__("sync;");
        id2 = *DANUBE_NAND_READ;
//        __asm__("sync;");
//        id3 = *DANUBE_NAND_READ;
//        id4 = *DANUBE_NAND_READ;
//        nand_ce_pulse();
        printk("id1 = %02X, id2 = %02X\n", (u32)id1, (u32)id2);
//        printk("id3 = %02X, id4 = %02X\n", (u32)id3, (u32)id4);

        NAND_READY_CLEAR();

        printk("first read\n");
//        NAND_CE_SET;
        nand_ce_latch();
        *DANUBE_NAND_CMD = 0x00;
//        __asm__("sync;");
        while ( !NAND_WAIT_WR_C );
        *DANUBE_NAND_ADDR = 0x00;
//        __asm__("sync;");
        while ( !NAND_WAIT_WR_C );
        *DANUBE_NAND_ADDR = 0x00;
//        __asm__("sync;");
        while ( !NAND_WAIT_WR_C );
        *DANUBE_NAND_ADDR = 0x00;
//        __asm__("sync;");
        while ( !NAND_WAIT_WR_C );
        *DANUBE_NAND_ADDR_OVER = 0x00;
//        __asm__("sync;");
        while ( !NAND_WAIT_WR_C );
//        while ( (*DANUBE_NAND_WAIT & 0x07) != 0x07 );
//        NAND_READY_CLEAR;
        while ( !nand_dev_ready() );
        for ( i = 0; i < 528; i++ )
        {
            buf[i] = *DANUBE_NAND_READ;
//            __asm__("sync;");
        }
//        NAND_CE_CLEAR;
        nand_ce_pulse();
        for ( i = 0; i < 528; i++ )
        {
            printk(" %02X", (u32)buf[i]);
            if ( (i & 0x0F) == 0x0F )
                printk("\n");
        }

        printk("erase\n");
//        NAND_CE_SET;
        nand_ce_latch();
        *DANUBE_NAND_CMD = 0x60;
//        __asm__("sync;");
        while ( !NAND_WAIT_WR_C );
        *DANUBE_NAND_ADDR = 0x00;
//        __asm__("sync;");
        while ( !NAND_WAIT_WR_C );
        *DANUBE_NAND_ADDR = 0x00;
//        __asm__("sync;");
        while ( !NAND_WAIT_WR_C );
        *DANUBE_NAND_ADDR_OVER = 0x00;
//        __asm__("sync;");
        while ( !NAND_WAIT_WR_C );
        *DANUBE_NAND_CMD = 0xD0;
//        __asm__("sync;");
        while ( !NAND_WAIT_WR_C );
//        while ( (*DANUBE_NAND_WAIT & 0x07) != 0x07 );
//        NAND_READY_CLEAR;
        while ( !nand_dev_ready() );
//        udelay(1);
//        while ( !danube_dev_ready(NULL) );
//        NAND_CE_CLEAR;
        nand_ce_pulse();

        printk("second read\n");
//        NAND_CE_SET;
        nand_ce_latch();
        *DANUBE_NAND_CMD = 0x00;
//        __asm__("sync;");
        while ( !NAND_WAIT_WR_C );
        *DANUBE_NAND_ADDR = 0x00;
//        __asm__("sync;");
        while ( !NAND_WAIT_WR_C );
        *DANUBE_NAND_ADDR = 0x00;
//        __asm__("sync;");
        while ( !NAND_WAIT_WR_C );
        *DANUBE_NAND_ADDR = 0x00;
//        __asm__("sync;");
        while ( !NAND_WAIT_WR_C );
        *DANUBE_NAND_ADDR_OVER = 0x00;
//        __asm__("sync;");
        while ( !NAND_WAIT_WR_C );
//        while ( (*DANUBE_NAND_WAIT & 0x07) != 0x07 );
//        NAND_READY_CLEAR;
        while ( !nand_dev_ready() );
        for ( i = 0; i < 528; i++ )
        {
            buf[i] = *DANUBE_NAND_READ;
//            __asm__("sync;");
        }
//        NAND_CE_CLEAR;
        nand_ce_pulse();
        for ( i = 0; i < 528; i++ )
        {
            printk(" %02X", (u32)buf[i]);
            if ( (i & 0x0F) == 0x0F )
                printk("\n");
        }

        printk("program\n");
//        NAND_CE_SET;
        nand_ce_latch();
        *DANUBE_NAND_CMD = 0x80;
//        __asm__("sync;");
        while ( !NAND_WAIT_WR_C );
        *DANUBE_NAND_ADDR = 0x00;
//        __asm__("sync;");
        while ( !NAND_WAIT_WR_C );
        *DANUBE_NAND_ADDR = 0x00;
//        __asm__("sync;");
        while ( !NAND_WAIT_WR_C );
        *DANUBE_NAND_ADDR = 0x00;
//        __asm__("sync;");
        while ( !NAND_WAIT_WR_C );
        *DANUBE_NAND_ADDR_OVER = 0x00;
//        __asm__("sync;");
        while ( !NAND_WAIT_WR_C );
        for ( i = 0; i < 528; i++ )
        {
            *DANUBE_NAND_WRITE = (u8)(i & 0xFF);
//            __asm__("sync;");
            while ( !NAND_WAIT_WR_C );
        }
        *DANUBE_NAND_CMD = 0x10;
//        __asm__("sync;");
        while ( !NAND_WAIT_WR_C );
//        while ( (*DANUBE_NAND_WAIT & 0x07) != 0x07 );
//        NAND_READY_CLEAR;
        while ( !nand_dev_ready() );
//        udelay(1);
//        while ( !danube_dev_ready(NULL) );
//        NAND_CE_CLEAR;
        nand_ce_pulse();
        for ( i = 0; i < 528; i++ )
        {
            printk(" %02X", i & 0xFF);
            if ( (i & 0x0F) == 0x0F )
                printk("\n");
        }

        printk("third read\n");
//        NAND_CE_SET;
        nand_ce_latch();
        *DANUBE_NAND_CMD = 0x00;
//        __asm__("sync;");
        while ( !NAND_WAIT_WR_C );
        *DANUBE_NAND_ADDR = 0x00;
//        __asm__("sync;");
        while ( !NAND_WAIT_WR_C );
        *DANUBE_NAND_ADDR = 0x00;
//        __asm__("sync;");
        while ( !NAND_WAIT_WR_C );
        *DANUBE_NAND_ADDR = 0x00;
//        __asm__("sync;");
        while ( !NAND_WAIT_WR_C );
        *DANUBE_NAND_ADDR_OVER = 0x00;
//        __asm__("sync;");
        while ( !NAND_WAIT_WR_C );
//        while ( (*DANUBE_NAND_WAIT & 0x07) != 0x07 );
//        NAND_READY_CLEAR;
        while ( !nand_dev_ready() );
        for ( i = 0; i < 528; i++ )
        {
            buf[i] = *DANUBE_NAND_READ;
//            __asm__("sync;");
        }
//        NAND_CE_CLEAR;
        nand_ce_pulse();
        for ( i = 0; i < 528; i++ )
        {
            printk(" %02X", (u32)buf[i]);
            if ( (i & 0x0F) == 0x0F )
                printk("\n");
        }
    }
#endif

//    printk("danube_cmdfunc(command = %02X, column = %d, page_addr = %d)\n", command, column, page_addr);

    if ( command == NAND_CMD_READID )
    {
        nand_ce_pulse();
        *DANUBE_NAND_CMD = command;
//        while ( !NAND_WAIT_WR_C );
        nand_wait_write_complete();
        *DANUBE_NAND_ADDR = column;
//        while ( !NAND_WAIT_WR_C );
        nand_wait_write_complete();
    }
    else
    {
        /*
         *  Write out the command to the device.
         *  These are copied from "nand.c".
         *  I think original data need be read into internal buffer,
         *  before write new data.
         */
#if 0
        if ( command == NAND_CMD_SEQIN )
        {
            int readcolumn = column;
            int readcmd;

            if ( column >= mtd->oobblock )
            {
                /*  OOB area    */
                readcolumn -= mtd->oobblock;
                readcmd = NAND_CMD_READOOB;
            }
            else if ( column < 256 )
            {
                /*  First 256 bytes ---> READ0  */
                readcmd = NAND_CMD_READ0;
            }
            else
            {
                readcolumn -= 256;
                readcmd = NAND_CMD_READ1;
            }
            *DANUBE_NAND_CMD = readcmd;
//            while ( !NAND_WAIT_WR_C );
            nand_wait_write_complete();
            *DANUBE_NAND_ADDR = readcolumn;
//            while ( !NAND_WAIT_WR_C );
            nand_wait_write_complete();
            *DANUBE_NAND_ADDR = (u8)(page_addr & 0xFF);
//            while ( !NAND_WAIT_WR_C );
            nand_wait_write_complete();
            *DANUBE_NAND_ADDR = (u8)((page_addr >> 8) & 0xFF);
//            while ( !NAND_WAIT_WR_C );
            nand_wait_write_complete();
            *DANUBE_NAND_ADDR_OVER = (u8)((page_addr >> 16) & 0xFF);
//            while ( !NAND_WAIT_WR_C );
            nand_wait_write_complete();
//            while ( !nand_dev_ready() );
            nand_wait_dev_ready();
        }
#endif
        *DANUBE_NAND_CMD = command;
//        while ( !NAND_WAIT_WR_C );
        nand_wait_write_complete();

        /*  Serial address output   */
        if ( column != -1 )
        {
            *DANUBE_NAND_ADDR = column;
//            while ( !NAND_WAIT_WR_C );
            nand_wait_write_complete();
        }
        if ( page_addr != -1 )
        {
            *DANUBE_NAND_ADDR = (u8)(page_addr & 0xFF);
//            while ( !NAND_WAIT_WR_C );
            nand_wait_write_complete();
            *DANUBE_NAND_ADDR = (u8)((page_addr >> 8) & 0xFF);
//            while ( !NAND_WAIT_WR_C );
            nand_wait_write_complete();
            *DANUBE_NAND_ADDR_OVER = (u8)((page_addr >> 16) & 0xFF);
//            while ( !NAND_WAIT_WR_C );
            nand_wait_write_complete();
        }

        /*
         *  program and erase have their own busy handlers
         *  status and sequential in needs no delay
         */
        switch ( command )
        {
#if 1
        case NAND_CMD_PAGEPROG:
#endif
        case NAND_CMD_ERASE1:
        case NAND_CMD_ERASE2:
        case NAND_CMD_SEQIN:
        case NAND_CMD_STATUS:
            return;

        case NAND_CMD_RESET:
//            while ( !nand_dev_ready() );
            nand_wait_dev_ready();
            *DANUBE_NAND_CMD = NAND_CMD_STATUS;
//            while ( !NAND_WAIT_WR_C );
            nand_wait_write_complete();
            if ( (*DANUBE_NAND_READ & 0x41) != 0x40 )
                panic("Reseting NAND chip failed!\n");
            return;

        default:
//            while ( !nand_dev_ready() );
            nand_wait_dev_ready();
        }
    }
}

/*
 *  Description:
 *    Caculate ECC of a page. For read, ECC is calculated by hardware, however,
 *    for write, a standard function implemented in source file "nand_ecc.c" is
 *    called to produce the ECC.
 *  Input:
 *    mtd      --- struct mtd_info *, MTD device to do ECC calculation.
 *    dat      --- const u_char *, page data.
 *    ecc_code --- u_char *, buffer to put ECC.
 *  Output:
 *    none
 */
#if defined(LINUX_2_4_31) && LINUX_2_4_31
  static void danube_calculate_ecc(const u_char *dat, u_char *ecc_code)
#else
  static void danube_calculate_ecc(struct mtd_info *mtd, const u_char *dat, u_char *ecc_code)
#endif
{
    if ( f_ecc_write )
#if defined(LINUX_2_4_31) && LINUX_2_4_31
        nand_calculate_ecc(dat, ecc_code);
#else
        nand_calculate_ecc(mtd, dat, ecc_code);
#endif
    else
    {
        ecc_code[0] = NAND_ECC0_B0;
        ecc_code[1] = NAND_ECC0_B1;
        ecc_code[2] = NAND_ECC0_B2;
        *DANUBE_NAND_CON &= ~NAND_CON_ECC_ON_SET(1);
    }
}

/*
 *  Description:
 *    Send signal to prepare for ECC calculation. For read, need to enable
 *    ECC hardware.
 *  Input:
 *    mtd  --- struct mtd_info *, MTD device to enable hardware ECC calculation.
 *    mode --- u_char *, signal ECC calculation for read or write.
 *  Output:
 *    none
 */
#if defined(LINUX_2_4_31) && LINUX_2_4_31
  static void danube_enable_hwecc(int mode)
#else
  static void danube_enable_hwecc(struct mtd_info *mtd, int mode)
#endif
{
    switch ( mode )
    {
    case NAND_ECC_READ:
        f_ecc_write = 0;
        NAND_ECC0_CLEAR();
        *DANUBE_NAND_CON |= NAND_CON_ECC_ON_SET(1);
        break;
    case NAND_ECC_WRITE:
        f_ecc_write = 1;
        break;
    }
}

/*
 *  Description:
 *    Check and adjust parameters passed in by command "insmod".
 *  Input:
 *    none
 *  Output:
 *    none
 */
#ifdef MODULE
    static INLINE void check_parameter(void)
    {
        if ( ebu_address_region < 0 || ebu_address_region > 4 )
            ebu_address_region = 1;
        else if ( ebu_address_region == 4 )
            ebu_address_region = 3;

        if ( ebu_config != 1 )
           ebu_config = 0;

        switch ( ebu_address_region )
        {
        case 0:
            ebu_base_address = (volatile u8*)(KSEG1 + DANUBE_EBU_ADDR_REGION0); break;
        case 1:
            ebu_base_address = (volatile u8*)(KSEG1 + DANUBE_EBU_ADDR_REGION1); break;
        case 2:
            ebu_base_address = (volatile u8*)(KSEG1 + DANUBE_EBU_ADDR_REGION2); break;
        case 3:
            ebu_base_address = (volatile u8*)(KSEG1 + DANUBE_EBU_ADDR_REGION3); break;
        }
    }
#else
    static INLINE void check_parameter(void) {}
#endif

/*
 *  Description:
 *    Config EBU region.
 *  Input:
 *    none
 *  Output:
 *    none
 */
static INLINE void config_ebu(void)
{
//    if ( !((*DANUBE_EBU_ADDR_SELn(0) ^ *DANUBE_EBU_ADDR_SEL) & ((((1 << (15 - GET_BITS(*DANUBE_EBU_ADDR_SELn(0), 7, 4))) - 1) << 12) | 0xF8000000)) )
//        *DANUBE_EBU_ADDR_SELn(0) ^= 0x01;
    *DANUBE_EBU_ADDR_SELn(0) = 0x11000000;

    *DANUBE_EBU_ADDR_SEL = EBU_ADDR_SEL_BASE_SET(((u32)ebu_base_address & 0x1FFFFFFF) >> 12)
                         | EBU_ADDR_SEL_MASK_SET(15)
                         | EBU_ADDR_SEL_MRME_SET(0)
                         | EBU_ADDR_SEL_REGEN_SET(1);

    *DANUBE_EBU_CON = EBU_CON_WRDIS_SET(0) | EBU_CON_ADSWP_SET(0) | EBU_CON_AGEN_SET(0x00)
                    | EBU_CON_SETUP_SET(1) | EBU_CON_WAIT_SET(0x00) | EBU_CON_WINV_SET(0)
                    | EBU_CON_PW_SET(0x00) | EBU_CON_ALEC_SET(3) | EBU_CON_BCGEN_SET(0x01)
                    | EBU_CON_WAITWRC_SET(1) | EBU_CON_WAITRDC_SET(1) | EBU_CON_HOLDC_SET(1)
                    | EBU_CON_RECOVC_SET(1) | EBU_CON_CMULT_SET(0x00);

#if 0
    printk("DANUBE_EBU_ADDR_SELn(0)     = 0x%08X\n", (u32)*DANUBE_EBU_ADDR_SELn(0));
    printk("DANUBE_EBU_ADDR_SEL(0x%08X) = 0x%08X\n", (u32)DANUBE_EBU_ADDR_SEL, (u32)*DANUBE_EBU_ADDR_SEL);
    printk("DANUBE_EBU_CON(0x%08X)      = 0x%08X\n", (u32)DANUBE_EBU_CON, (u32)*DANUBE_EBU_CON);
#endif
}

/*
 *  Description:
 *    Config NAND controller.
 *  Input:
 *    none
 *  Output:
 *    none
 */
static INLINE void config_nand(void)
{
    nand_ce_pulse();

#if 0
    printk("DANUBE_NAND_CON(0x%08X)     = 0x%08X\n", (u32)DANUBE_NAND_CON, (u32)*DANUBE_NAND_CON);
#endif
}

static INLINE void nand_ce_latch(void)
{
    /*
     *  ECC disabled
     *  PRE, WP, SE and CLE are pulse, CS and ALE are latch based
     *  CLE and ALE are active high, PRE, WP, SE and CS/CE are active low
     *  OUT_CS_S is enabled
     *  NAND mode is enabled
     */
    *DANUBE_NAND_CON = NAND_CON_ECC_ON_SET(0)
                     | NAND_CON_PRE_LAT_EN_SET(0) | NAND_CON_WP_LAT_EN_SET(0)
                     | NAND_CON_SE_LAT_EN_SET(0) | NAND_CON_CS_LAT_EN_SET(1)
                     | NAND_CON_CLE_LAT_EN_SET(0) | NAND_CON_ALE_LAT_EN_SET(1)
                     | NAND_CON_OUT_CS_S_SET(ebu_address_region)
                     | NAND_CON_IN_CS_S_SET(ebu_address_region)
                     | NAND_CON_PRE_P_SET(1) | NAND_CON_WP_P_SET(1)
                     | NAND_CON_SE_P_SET(1) | NAND_CON_CS_P_SET(1)
                     | NAND_CON_CLE_P_SET(0) | NAND_CON_ALE_P_SET(0)
                     | NAND_CON_CSMUX_E_SET(1)
                     | NAND_CON_NANDM_SET(1);
}

static INLINE void nand_ce_pulse(void)
{
    /*
     *  ECC disabled
     *  PRE, WP, SE CS, CLE and ALE are pulse, no signal is latch based
     *  CLE and ALE are active high, PRE, WP, SE and CS/CE are active low
     *  OUT_CS_S is enabled
     *  NAND mode is enabled
     */
    *DANUBE_NAND_CON = NAND_CON_ECC_ON_SET(0)
                     | NAND_CON_PRE_LAT_EN_SET(0) | NAND_CON_WP_LAT_EN_SET(0)
                     | NAND_CON_SE_LAT_EN_SET(0) | NAND_CON_CS_LAT_EN_SET(0)
                     | NAND_CON_CLE_LAT_EN_SET(0) | NAND_CON_ALE_LAT_EN_SET(0)
                     | NAND_CON_OUT_CS_S_SET(ebu_address_region)
                     | NAND_CON_IN_CS_S_SET(ebu_address_region)
                     | NAND_CON_PRE_P_SET(1) | NAND_CON_WP_P_SET(1)
                     | NAND_CON_SE_P_SET(1) | NAND_CON_CS_P_SET(1)
                     | NAND_CON_CLE_P_SET(0) | NAND_CON_ALE_P_SET(0)
                     | NAND_CON_CSMUX_E_SET(1)
                     | NAND_CON_NANDM_SET(1);
}

static INLINE int nand_dev_ready(void)
{
    if ( NAND_WAIT_RD )
    {
        NAND_READY_CLEAR();
        return 1;
    }
    else
        return 0;
}

static INLINE void nand_wait_write_complete(void)
{
#if defined(MAX_LOOP_COUNT) && MAX_LOOP_COUNT
    int j;

    for ( j = 0; j < MAX_LOOP_COUNT && !NAND_WAIT_WR_C; j++ );
  #if defined(PANIC_WHEN_REACH_MAX_LOOP_COUNT) && PANIC_WHEN_REACH_MAX_LOOP_COUNT
    if ( j >= MAX_LOOP_COUNT )
        panic("danube_nand: loop is over threshold (nand_wait_write_complete: danube_cmdfunc or danube_nand_init)\n");
  #endif
#else
    while ( !NAND_WAIT_WR_C );
#endif
}

static INLINE void nand_wait_dev_ready(void)
{
#if defined(MAX_LOOP_COUNT) && MAX_LOOP_COUNT
    int j;

    for ( j = 0; j < MAX_LOOP_COUNT && !nand_dev_ready(); j++ );
  #if defined(PANIC_WHEN_REACH_MAX_LOOP_COUNT) && PANIC_WHEN_REACH_MAX_LOOP_COUNT
    if ( j >= MAX_LOOP_COUNT )
        panic("danube_nand: loop is over threshold (nand_wait_dev_ready)\n");
  #endif
#else
    while ( !nand_dev_ready() );
#endif
}


/*
 * ####################################
 *           Global Function
 * ####################################
 */


/*
 * ####################################
 *           Init/Cleanup API
 * ####################################
 */

/*
 *  Description:
 *    Register MTD partitions or device.
 *  Input:
 *    none
 *  Output:
 *    0    --- successful
 *    else --- failure, usually it is negative value of error code
 */
int __init danube_nand_init(void)
{
//    printk("danube_nand_init start......\n");

    /*
     *  Remap to uncached address
     */
//    ebu_base_address = (volatile u8 *)ioremap(ebu_base_address, 4096);
//    if ( !ebu_base_address )
//        return -EIO;
//    printk("danube_nand_init: ioremap\n");

    /*
     *  Configure hardware
     */
    check_parameter();

#if 0
    printk("DANUBE_NAND_CMD(0x%08X)\n",         (u32)DANUBE_NAND_CMD);
    printk("DANUBE_NAND_WRITE(0x%08X)\n",       (u32)DANUBE_NAND_WRITE);
    printk("DANUBE_NAND_ADDR(0x%08X)\n",        (u32)DANUBE_NAND_ADDR);
    printk("DANUBE_NAND_READ(0x%08X)\n",        (u32)DANUBE_NAND_READ);
    printk("DANUBE_NAND_WRITE_OVER(0x%08X)\n",  (u32)DANUBE_NAND_WRITE_OVER);
    printk("DANUBE_NAND_ADDR_OVER(0x%08X)\n",   (u32)DANUBE_NAND_ADDR_OVER);
    printk("DANUBE_NAND_READ_OVER(0x%08X)\n",   (u32)DANUBE_NAND_READ_OVER);
#endif

#if defined(DEBUG_WRITE_GPIO_REGISTER) && DEBUG_WRITE_GPIO_REGISTER
  #if 0
    *DANUBE_GPIO_P0_ALTSEL0 |=  0x2000;
    *DANUBE_GPIO_P0_ALTSEL1 &= ~0x2000;
    *DANUBE_GPIO_P0_DIR     |=  0x2000;
    *DANUBE_GPIO_P0_OD      |=  0x2000;
    *DANUBE_GPIO_P1_ALTSEL0 |=  0x0100;
    *DANUBE_GPIO_P1_ALTSEL1 &= ~0x0100;
    *DANUBE_GPIO_P1_DIR     |=  0x0100;
    *DANUBE_GPIO_P1_OD      |=  0x0100;

   #if 0
    printk("DANUBE_GPIO_P0_ALTSEL0(0x%08X) & 0x2000 = 0x%08X\n", (u32)DANUBE_GPIO_P0_ALTSEL0, (u32)*DANUBE_GPIO_P0_ALTSEL0 & 0x2000);
    printk("DANUBE_GPIO_P0_ALTSEL1(0x%08X) & 0x2000 = 0x%08X\n", (u32)DANUBE_GPIO_P0_ALTSEL1, (u32)*DANUBE_GPIO_P0_ALTSEL1 & 0x2000);
    printk("DANUBE_GPIO_P0_DIR(0x%08X)     & 0x2000 = 0x%08X\n", (u32)DANUBE_GPIO_P0_DIR,     (u32)*DANUBE_GPIO_P0_DIR     & 0x2000);
    printk("DANUBE_GPIO_P1_ALTSEL0(0x%08X) & 0x0100 = 0x%08X\n", (u32)DANUBE_GPIO_P1_ALTSEL0, (u32)*DANUBE_GPIO_P1_ALTSEL0 & 0x0100);
    printk("DANUBE_GPIO_P1_ALTSEL1(0x%08X) & 0x0100 = 0x%08X\n", (u32)DANUBE_GPIO_P1_ALTSEL1, (u32)*DANUBE_GPIO_P1_ALTSEL1 & 0x0100);
    printk("DANUBE_GPIO_P1_DIR(0x%08X)     & 0x0100 = 0x%08X\n", (u32)DANUBE_GPIO_P1_DIR,     (u32)*DANUBE_GPIO_P1_DIR     & 0x0100);
   #endif
  #else
    *(u32*)0xBE100B1C |=  0x2000;
    *(u32*)0xBE100B20 &= ~0x2000;
    *(u32*)0xBE100B18 |=  0x2000;
    *(u32*)0xBE100B24 |=  0x2000;
    *(u32*)0xBE100B4C |=  0x0100;
    *(u32*)0xBE100B50 &= ~0x0100;
    *(u32*)0xBE100B48 |=  0x0100;
    *(u32*)0xBE100B54 |=  0x0100;

   #if 0
    printk("DANUBE_GPIO_P0_ALTSEL0(0xBE100B1C) & 0x2000 = 0x%08X\n", *(u32*)0xBE100B1C & 0x2000);
    printk("DANUBE_GPIO_P0_ALTSEL1(0xBE100B20) & 0x2000 = 0x%08X\n", *(u32*)0xBE100B20 & 0x2000);
    printk("DANUBE_GPIO_P0_DIR(0xBE100B18)     & 0x2000 = 0x%08X\n", *(u32*)0xBE100B18 & 0x2000);
    printk("DANUBE_GPIO_P0_OD(0xBE100B24)      & 0x2000 = 0x%08X\n", *(u32*)0xBE100B24 & 0x2000);
    printk("DANUBE_GPIO_P1_ALTSEL0(0xBE100B4C) & 0x0100 = 0x%08X\n", *(u32*)0xBE100B4C & 0x0100);
    printk("DANUBE_GPIO_P1_ALTSEL1(0xBE100B50) & 0x0100 = 0x%08X\n", *(u32*)0xBE100B50 & 0x0100);
    printk("DANUBE_GPIO_P1_DIR(0xBE100B48)     & 0x0100 = 0x%08X\n", *(u32*)0xBE100B48 & 0x0100);
    printk("DANUBE_GPIO_P1_OD(0xBE100B54)      & 0x2000 = 0x%08X\n", *(u32*)0xBE100B54 & 0x2000);
   #endif
  #endif
#else
  #error Must configure GPIO
#endif

    if ( ebu_config )
        config_ebu();

    config_nand();

    NAND_READY_CLEAR();
    *DANUBE_NAND_CMD = NAND_CMD_RESET;
//    while ( !NAND_WAIT_WR_C );
    nand_wait_write_complete();
//    printk("After write reset\n");
//    while ( !nand_dev_ready() );
    nand_wait_dev_ready();
    *DANUBE_NAND_CMD = NAND_CMD_STATUS;
//    while ( !NAND_WAIT_WR_C );
    nand_wait_write_complete();
    if ( (*DANUBE_NAND_READ & 0x41) != 0x40 )
        panic("Reseting NAND chip failed!\n");
//    printk("After reset\n");

    /*
     *  Setup structures
     */
    chip.IO_ADDR_R      = (unsigned long)DANUBE_NAND_READ;
    chip.IO_ADDR_W      = (unsigned long)DANUBE_NAND_WRITE;
#if defined(LINUX_2_4_31) && LINUX_2_4_31
    chip.hwcontrol      = danube_hwcontrol;
#else
    chip.write_byte     = danube_write_byte;
    chip.write_buf      = danube_write_buf;
    chip.select_chip    = danube_select_chip;
#endif
    chip.dev_ready      = danube_dev_ready;
    chip.cmdfunc        = danube_cmdfunc;
    chip.calculate_ecc  = danube_calculate_ecc;
    chip.correct_data   = nand_correct_data;
    chip.enable_hwecc   = danube_enable_hwecc;
//    chip.eccmode        = NAND_ECC_HW3_256;   //  hardware ECC is disabled
    chip.eccmode        = NAND_ECC_SOFT;
    chip.data_buf       = data_buf;
//    chip.options        |= NAND_SKIP_BBTSCAN;   //  for linux 2.6

    mtd.name = "Danube Nand Controller";
    mtd.priv = &chip;
#if (!defined(LINUX_2_4_31) || !LINUX_2_4_31) && (defined(FORCE_ECC_ON) && FORCE_ECC_ON)
    mtd.oobinfo.useecc = 1;
    mtd.oobinfo.eccpos[0] = 0;
    mtd.oobinfo.eccpos[1] = 1;
    mtd.oobinfo.eccpos[2] = 2;
    mtd.oobinfo.eccpos[3] = 3;
    mtd.oobinfo.eccpos[4] = 6;
    mtd.oobinfo.eccpos[5] = 7;
#endif

    /*
     *  Scan to find existance of the device
     */
//    printk("before scan\n");
#if defined(LINUX_2_4_31) && LINUX_2_4_31
    if ( nand_scan(&mtd) )
#else
    if ( nand_scan(&mtd, 1) )
#endif
    {
        printk("danube_nand_init: nand_scan failed\n");
//        iounmap(ebu_base_address);
        return -ENXIO;
    }
//    printk("danube_nand_init: nand_scan\n");

#if 0
    printk("mtd:\n");
    printk("  type:      0x%02X\n", (u32)mtd.type);
    printk("  flags:     0x%08X\n", (u32)mtd.flags);
    printk("  size:      %d\n",     (u32)mtd.size);
    printk("  erasesize: %d\n",     (u32)mtd.erasesize);
    printk("  oobblock:  %d\n",     (u32)mtd.oobblock);
    printk("  oobsize:   %d\n",     (u32)mtd.oobsize);
    printk("  ecctype:   %d\n",     (u32)mtd.ecctype);
    printk("  eccsize:   %d\n",     (u32)mtd.eccsize);
    printk("  name:      %s\n",     mtd.name);
    printk("  index:     %d\n",     mtd.index);
    printk("  numeraseregions: %d\n", mtd.numeraseregions);
#endif

#if 1
    /*
     *  Register MTD partitions or device
     */
#ifdef CONFIG_MTD_CMDLINE_PARTS
    {
        int mtd_parts_nb = 0;
        struct mtd_partition *mtd_parts = NULL;

        mtd_parts_nb = parse_cmdline_partitions(&mtd, &mtd_parts, "danube nand controller");
        if ( mtd_parts_nb > 0 )
        {
            add_mtd_partitions(&mtd, mtd_parts, mtd_parts_nb);
            f_mtd_partitions = 1;
        }
        else
            add_mtd_device(&mtd);
//        printk("danube_nand_init: parse_cmdline_partitions\n");
    }
#else
  #if defined(NUM_PARTITIONS) && NUM_PARTITIONS > 0
    add_mtd_partitions(&mtd, partition_info, NUM_PARTITIONS);
    f_mtd_partitions = 1;
//    printk("danube_nand_init: add_mtd_partitions\n");
  #else
    add_mtd_device(&mtd);
//    printk("danube_nand_init: add_mtd_device\n");
  #endif
#endif
#endif

    printk("Init NAND controller succeeded!\n");
    return 0;
}

/*
 *  Description:
 *    Deregister MTD partitions or device.
 *  Input:
 *    none
 *  Output:
 *    none
 */
void __exit danube_nand_exit(void)
{
    if ( f_mtd_partitions )
        del_mtd_partitions(&mtd);
    else
        del_mtd_device(&mtd);

//    iounmap(ebu_base_address);
}

module_init(danube_nand_init);
module_exit(danube_nand_exit);

#else


#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/mtd/partitions.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <asm/unistd.h>
#include <asm/io.h>
#include <asm/delay.h>
#include <linux/errno.h>
#include <asm/danube/danube.h>


//#define PROVIDE_IMG
#ifdef PROVIDE_IMG
  #include "nand_img.h"
#endif

//#define TEST_DANUBE
#define DEBUG_NAND_OUTPUT
#if defined(TEST_DANUBE) && defined(DEBUG_NAND_OUTPUT)
  #define DEBUG_NAND(string)	asc_puts(string)
#else
  #define DEBUG_NAND(string)
#endif

/***********************************************************************/
/*  Module      :  EBU register address and bits                       */
/***********************************************************************/

#define DANUBE_EBU                          (0xBE105300)
#define EBU_ADDR_SEL_0     (volatile u32*)(DANUBE_EBU + 0x20)
#define EBU_ADDR_SEL_1     (volatile u32*)(DANUBE_EBU + 0x24)
#define EBU_CON_0          (volatile u32*)(DANUBE_EBU + 0x60)
#define EBU_CON_1          (volatile u32*)(DANUBE_EBU + 0x64)
#define EBU_NAND_CON       (volatile u32*)(DANUBE_EBU + 0xB0)
#define EBU_NAND_WAIT      (volatile u32*)(DANUBE_EBU + 0xB4)
#define EBU_NAND_ECC0      (volatile u32*)(DANUBE_EBU + 0xB8)
#define EBU_NAND_ECC_AC    (volatile u32*)(DANUBE_EBU + 0xBC)

#define NAND_BASE_ADDRESS  0xB4000000

#define NAND_WRITE(addr, val)     *((u8*)(NAND_BASE_ADDRESS | (addr))) = val;while((*EBU_NAND_WAIT & 0x08) == 0);
#define NAND_READ(addr, val)      val = *((u8*)(NAND_BASE_ADDRESS | (addr)))
#define NAND_CE_SET  		*EBU_NAND_CON   = 0x00F405F3
#define NAND_CE_CLEAR 		*EBU_NAND_CON   = 0x000005F3
#define NAND_READY       ( ((*EBU_NAND_WAIT)&0x07) == 7)
#define NAND_READY_CLEAR  *EBU_NAND_WAIT = 0;
#define WRITE_CMD    0x18
#define WRITE_ADDR   0x14
#define WRITE_LADDR  0x10
#define WRITE_DATA  0x10
#define READ_DATA    0x10
#define READ_LDATA   0x00
#define ACCESS_WAIT

#define NAND_OK              0x00000000    /* Bootstrap succesful, start address in BOOT_RVEC */
#define NAND_ERR             0x80000000
#define NAND_ACC_TIMEOUT     (NAND_ERR | 0x00000001)
#define NAND_ACC_ERR         (NAND_ERR | 0x00000002)

#define _ADDR_MAX   5               /* Number of address cycles currently supported */
#define _CMD_MAX    _ADDR_MAX + 2   /* Length of commands */

typedef struct _nand_cmd
{
   u32 size;
   u32 addr_len;
   u8 addr[_CMD_MAX];
   u8 data[_CMD_MAX];
} nand_cmd;

typedef struct _nand_acc
{
   u32 cmd_index;
   u32 page;
   u32 page_size;
   u32 col_bytes;
} nand_acc;

/* threshold for error pages */
#define MAX_ERR_PAGES   0
/*****************************************************************************
 * Local definitions
 *****************************************************************************/
#define IFX_ATC_NAND 0xc176
#define IFX_BTC_NAND 0xc166
#define ST_512WB2_NAND 0x2076
#define NAND_PAGE_SIZE 512
typedef struct _nand_t
{
  u8 erasecmd1;
  u8 erasecmd2;
  u8 prgcmd;
  u8 ridcmd;
  u8 writecmd;
  u8 readcmd;
  u8 rstscmd;
  u8 rstcmd;
  u8 addr[_ADDR_MAX];
  u8 col_addr_byte;
  u8 total_addr_byte;
  u32 pages_per_block;
} nand_t;

nand_t g_nand;

/*****************************************************************************
 * Local functions
 *****************************************************************************/
static u32 nand_read_chip_id(void);
static u8 nand_status(void);
static void nand_reset(void);
static void check_nand_status_done(void);
static void nand_read(char * buf, u32 len);
static void nand_write(char * buf, u32 len);
static void nand_erase_block(void);
static u32 nand_get_page_num(void);
static int _nand_wait_ready(void);
static void nand_write_addr(void);
static int _nand_init(void);

#ifdef PROVIDE_IMG
  const static u32 _img[] = TEST_IMG;
#else
  const static u32 _img[] = {0x00000000,0x11111111,0x22222222,0x33333333};
#endif


#define asc_puts        printk
void print_u32(u32 n)
{
    printk("%08X", n);
}

void print_u16(u16 n)
{
    printk("%04X", (u32)n);
}

void print_u8(u8 n)
{
    printk("%02X", (u32)n);
}


void START_ECC(void) {
  *(EBU_NAND_CON)   = *(EBU_NAND_CON) | 0x80000000;
  *(EBU_NAND_ECC0)    = 0;
  *(EBU_NAND_ECC_AC)    = 0;
};

void STOP_ECC(void)
{
*(EBU_NAND_CON)   = *(EBU_NAND_CON) & 0x7FFFFFFF;
}

void RESUME_ECC(void)
{
*(EBU_NAND_CON)   = *(EBU_NAND_CON) | 0x80000000;
}

u32 GET_ECC_RESULT(void)
{
    return *(EBU_NAND_ECC0);
}

#if 0
static u32 nand_get_ecc(void)
{
	int i;
    u8 tmp;
	NAND_CE_SET;
	NAND_WRITE(WRITE_CMD,g_nand.readcmd);
    nand_write_addr();
#ifndef ON_IKOS
	_nand_wait_ready();
#endif
    START_ECC();  // clear ECC register
	for(i=0;i<256;i++){
		NAND_READ(READ_DATA,tmp);
	}
	NAND_CE_CLEAR;
    STOP_ECC();
	NAND_CE_SET;
	NAND_WRITE(WRITE_CMD,g_nand.readcmd);
    nand_write_addr();
#ifndef ON_IKOS
	_nand_wait_ready();
#endif
    RESUME_ECC();
	for(i=0;i<256;i++){
		NAND_READ(READ_DATA,tmp);
	}
	NAND_CE_CLEAR;
    return GET_ECC_RESULT();
}
#endif

static u8 nand_status(void)
{
	u8 status;
	//NAND_CE_SET;
	NAND_WRITE(WRITE_CMD,0x70);
	NAND_READ(READ_DATA,status);
	//NAND_CE_CLEAR;
	return status;
}
static int _nand_wait_ready(void)
{
   static int wait;
   wait = 0;
#if 0
/* RD/BY signal may not be connected yet*/
	while ((nand_status() & 1) == 1){
#else
  while(!NAND_READY) {
#endif
      if((wait++) == 0x4000)
         return NAND_ACC_TIMEOUT;
   }
   NAND_READY_CLEAR;
   return NAND_OK;
}
static void nand_reset(void)
{
	NAND_CE_SET;
	NAND_WRITE(WRITE_CMD,0xff);
	NAND_CE_CLEAR;
}
static void check_nand_status_done(void)
{
	u8 status;
	status = nand_status();
	while ((status & 1) == 1){
		DEBUG_NAND(".");
	}
}
static u32 nand_get_page_num(void)
{
  u32 page=0;
  int i;
  for(i=g_nand.total_addr_byte-1;i>=g_nand.col_addr_byte;i--){
    page = (page<<8) + g_nand.addr[i];
  }
  asc_puts("page:");
  print_u32(page);
  asc_puts("\n");
  return page;
}
static void nand_write_addr()
{
	int i;
  asc_puts("[");
  for(i=0;i<g_nand.total_addr_byte-1;i++){
    print_u8(g_nand.addr[i]);
	  NAND_WRITE(WRITE_ADDR,g_nand.addr[i]);
  }
  print_u8(g_nand.addr[i]);
  asc_puts("]\n");
  /* Last address */
	NAND_WRITE(WRITE_LADDR,g_nand.addr[i]);
}
static void nand_write_page_addr(void)
{
	int i;
  asc_puts("[");
  for(i=g_nand.col_addr_byte;i<g_nand.total_addr_byte-1;i++){
    print_u8(g_nand.addr[i]);
	  NAND_WRITE(WRITE_ADDR,g_nand.addr[i]);
  }
  print_u8(g_nand.addr[i]);
  asc_puts("]\n");
  /* Last address */
	NAND_WRITE(WRITE_LADDR,g_nand.addr[i]);
}
static void nand_read(char * buf, u32 len)
{
	int i;
	NAND_CE_SET;
	NAND_WRITE(WRITE_CMD,g_nand.readcmd);
  nand_write_addr();
#ifndef ON_IKOS
	_nand_wait_ready();
#endif
	for(i=0;i<len;i++){
		NAND_READ(READ_DATA,buf[i]);
	}
	NAND_CE_CLEAR;
}
/* Brief: erase 32 pages
 */
static void nand_erase_block(void)
{
	/*Erase block*/
	NAND_CE_SET;
	NAND_WRITE(WRITE_CMD,g_nand.erasecmd1);
    nand_write_page_addr();
	NAND_WRITE(WRITE_CMD,g_nand.erasecmd2);
	_nand_wait_ready();
	NAND_CE_CLEAR;
}
/* Brief: program one page
 */
static void nand_write(char * buf, u32 len)
{
	int i;
	/*Data into buffer*/
	NAND_CE_SET;
	NAND_WRITE(WRITE_CMD,g_nand.writecmd);
    nand_write_addr();
	for(i=0;i<len;i++){
		NAND_WRITE(WRITE_DATA,buf[i]);
	}
	/*Program*/
	NAND_WRITE(WRITE_CMD,g_nand.prgcmd);
	_nand_wait_ready();
	NAND_CE_CLEAR;
}
/* Read manufacturer and device ID */
static u32 nand_read_chip_id(void)
{
   volatile u8 tmp[2];
   u32 id;

   //NAND_CE_SET;
   /* use latch doesn't work for one-cycle address */

   /* Read ID: always use command 0x90 */
   NAND_WRITE(WRITE_CMD, 0x90);
   NAND_WRITE(WRITE_ADDR, 0x00);

   NAND_READ(READ_DATA, tmp[0]);
   NAND_READ(READ_DATA, tmp[1]);

   DEBUG_NAND("FLASH ID: ");
   print_u8(tmp[0]);
   print_u8(tmp[1]);
   DEBUG_NAND("\n");
   //NAND_CE_CLEAR;

   id = tmp[0]*256 + tmp[1];
   return (id);
}

void nand_increase_page_num(void)
{
    int i;
    for (i=g_nand.col_addr_byte; i < g_nand.total_addr_byte; i++) {
        if ( g_nand.addr[i] != 0xff){
            g_nand.addr[i]++;
            break;
        }else{
            g_nand.addr[i]=0;
        }
    }
}

#ifndef TEST_IMG
#define TEST_SIZE 128
#endif
/* Brief:   read/write/program test for NAND chip
 * Return:
 ** 0 OK
 ** 1 fails
 ** 2 unsupported NAND
 */
int test_nand(void)
{
#ifdef TEST_IMG
  char *wr_buf;
  u32 len=sizeof(_img)/sizeof(char);
#else
  char wr_buf[NAND_PAGE_SIZE];
  u32 len=TEST_SIZE;
#endif
  u32 eff_len=0;
  char rd_buf[NAND_PAGE_SIZE];
  int i,err=0;
  u32 devtype;
//  u8 status;
  u32 err_pages=0;
  int fails=0;

  _nand_init();

  nand_reset();
  check_nand_status_done();

  devtype=nand_read_chip_id();
  if(devtype==IFX_ATC_NAND) {
    asc_puts("Infineon nand flash (ATC) found\n");
    g_nand.readcmd=0x0;
    g_nand.writecmd=0x80;
    g_nand.erasecmd1=0x60;
    g_nand.erasecmd2=0xd0;
    g_nand.prgcmd=0x10;
    g_nand.rstscmd=0x70;
    g_nand.rstcmd=0xff;
    g_nand.col_addr_byte=1;
    g_nand.total_addr_byte=4;
    g_nand.pages_per_block=32;
  } else if(devtype==IFX_BTC_NAND) {
    asc_puts("Infineon nand flash (BTC) found\n");
    g_nand.readcmd=0x0;
    g_nand.writecmd=0x80;
    g_nand.erasecmd1=0x60;
    g_nand.erasecmd2=0xd0;
    g_nand.prgcmd=0x10;
    g_nand.rstscmd=0x70;
    g_nand.rstcmd=0xff;
    g_nand.col_addr_byte=1;
    g_nand.total_addr_byte=4;
    g_nand.pages_per_block=32;
  }else if (devtype == ST_512WB2_NAND){
    asc_puts("ST 512WB2 found\n");
    g_nand.readcmd=0x0;
    g_nand.writecmd=0x80;
    g_nand.erasecmd1=0x60;
    g_nand.erasecmd2=0xd0;
    g_nand.prgcmd=0x10;
    g_nand.rstscmd=0x70;
    g_nand.rstcmd=0xff;
    g_nand.col_addr_byte=1;
    g_nand.total_addr_byte=4;
    g_nand.pages_per_block=32;
  }else{
    asc_puts("unsupported nand ID:");
    print_u32(devtype);
    asc_puts("\n");
    return 2;
  }
  /* initialize addre bytes*/
  for(i=0;i<g_nand.total_addr_byte;i++){
    g_nand.addr[i]=0;
  }
/* intialize Test data */
#ifdef TEST_IMG
    wr_buf = (char *) &_img[0];
#else
    for(i=0;i<NAND_PAGE_SIZE;i++){
      wr_buf[i]=i;
    }
#endif
  NAND_READY_CLEAR;
  while(1){
    eff_len = (len>NAND_PAGE_SIZE)?(NAND_PAGE_SIZE):len;
    if (nand_get_page_num() % g_nand.pages_per_block == 0){
      /* erase block */
      nand_erase_block();
    }
    /*TODO: testing error page */
  /* TODO use second page */
#if 0
    nand_increase_page_num();
    nand_increase_page_num();
#endif
    nand_write(wr_buf,eff_len);
    nand_read(rd_buf,eff_len);
    for(i=0;i<eff_len;i++){
//      print_u8(rd_buf[i]);
      if (rd_buf[i] != wr_buf[i]){
        err=1;
      }
    }
    print_u32(len);
    asc_puts("\n");
    if (err){
      err=0;
      err_pages++;
      if (err_pages > MAX_ERR_PAGES){
        fails=1;
        break;
      }
    }else{
      len -= eff_len;
#ifdef TEST_IMG
      wr_buf += eff_len;
#endif
      if ((len == 0 )){
        break;
      }
    }
    nand_increase_page_num();
  }
  nand_get_page_num();
  asc_puts(" error pages:");
  print_u32(err_pages);
  asc_puts("\n");
  if (fails){
    DEBUG_NAND("NAND r/w Fail\n");
  }else{
    DEBUG_NAND("NAND r/w Pass\n");
  }
  nand_reset();
  return err;
}

static int _nand_init(void)
{
#ifdef ON_EMULATOR
/* TODO: to resolve PCI reset floating problem, remove it !!!!
 * PCI reset signal GPIO21 is not pull high internally
 */
  *DANUBE_GPIO_P1_DIR |=  0x20 ;
  *DANUBE_GPIO_P1_OUT =  0x20 ;
#endif
  /* set GPIO pins for NAND */
  /* P0.13 FL_A24 01:output*/
  /* P1.8 FL_A23 01:output*/
  *DANUBE_GPIO_P0_ALTSEL0 |= 0x2000;
  *DANUBE_GPIO_P0_ALTSEL1 &= (~0x2000);
  *DANUBE_GPIO_P0_DIR |= (0x2000);
  *DANUBE_GPIO_P1_ALTSEL0 |= 0x100;
  *DANUBE_GPIO_P1_ALTSEL1 &= (~0x100);
  *DANUBE_GPIO_P1_DIR |= (0x100);
	/* Configure EBU */
	*EBU_ADDR_SEL_0 = 0x11000031;
	*EBU_ADDR_SEL_1 = (NAND_BASE_ADDRESS&0x1fffff00)|0xf1;
	/* no byte swap;medium delay*/
#ifdef ON_EMULATOR
  /* use faster parameter */
	*EBU_CON_1      = 0x40D154;
#else
  /* SETUP:1 (must) ALEC:3 BCGEN:1 WAITWRC:2 WAITRDC:2 HOLDC:1 RECOVC:1 CMULT:1 */
	*EBU_CON_1      = 0x40D295;
#endif
	*EBU_NAND_CON   = 0x000005F3;

	/* Set bus signals to inactive */
	NAND_READY_CLEAR;
	NAND_CE_CLEAR;
	return NAND_OK;
}


int __init danube_nand_init(void)
{
    asc_puts("Danube Test build on "__DATE__","__TIME__"\n");
    if ( test_nand() )
        panic("nand failed\n");

    return -1;
}

void __exit danube_nand_exit(void)
{
}

module_init(danube_nand_init);
module_exit(danube_nand_exit);

#endif
