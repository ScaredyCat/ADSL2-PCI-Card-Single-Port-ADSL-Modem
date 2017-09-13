/******************************************************************************
**
** FILE NAME    : danube_cgu.c
** PROJECT      : Danube
** MODULES     	: CGU
**
** DATE         : 19 JUL 2005
** AUTHOR       : Xu Liang
** DESCRIPTION  : Clock Generation Unit (CGU) Driver
** COPYRIGHT    : 	Copyright (c) 2006
**			Infineon Technologies AG
**			Am Campeon 1-12, 85579 Neubiberg, Germany
**
**    This program is free software; you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation; either version 2 of the License, or
**    (at your option) any later version.
**
** HISTORY
** $Date        $Author         $Comment
** 19 JUL 2005  Xu Liang        Initiate Version
** 21 AUG 2006  Xu Liang        Work around to fix calculation error for 36M
**                              crystal.
** 23 OCT 2006  Xu Liang        Add GPL header.
*******************************************************************************/



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
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <asm/unistd.h>
#include <asm/irq.h>
#include <linux/errno.h>
#include <linux/proc_fs.h>

/*
 *  Chip Specific Head File
 */
#include <asm/danube/danube_cgu.h>


/*
 * ####################################
 *              Definition
 * ####################################
 */

#define FIX_FOR_36M_CRYSTAL             1

#define DEBUG_ON_AMAZON                 0

#define DEBUG_ON_VENUS                  0

#define DEBUG_PRINT_INFO                0

/*
 *  Frequency of Clock Direct Feed from The Analog Line Driver Chip
 */
#define BASIC_INPUT_CLOCK_FREQUENCY_1   35328000
#define BASIC_INPUT_CLOCK_FREQUENCY_2   36000000

#define BASIS_INPUT_CRYSTAL_USB         12000000

/*
 *  Bits Operation
 */
#define GET_BITS(x, msb, lsb)           (((x) & ((1 << ((msb) + 1)) - 1)) >> (lsb))
#define SET_BITS(x, msb, lsb, value)    (((x) & ~(((1 << ((msb) + 1)) - 1) ^ ((1 << (lsb)) - 1))) | (((value) & ((1 << (1 + (msb) - (lsb))) - 1)) << (lsb)))

/*
 *  CGU Register Mapping
 */
#define DANUBE_CGU                      (KSEG1 + 0x1F103000)
#define DANUBE_CGU_PLL0_CFG             ((volatile u32*)(DANUBE_CGU + 0x0004))
#define DANUBE_CGU_PLL1_CFG             ((volatile u32*)(DANUBE_CGU + 0x0008))
#define DANUBE_CGU_PLL2_CFG             ((volatile u32*)(DANUBE_CGU + 0x000C))
#define DANUBE_CGU_SYS                  ((volatile u32*)(DANUBE_CGU + 0x0010))
#define DANUBE_CGU_UPDATE               ((volatile u32*)(DANUBE_CGU + 0x0014))
#define DANUBE_CGU_IF_CLK               ((volatile u32*)(DANUBE_CGU + 0x0018))
#define DANUBE_CGU_OSC_CON              ((volatile u32*)(DANUBE_CGU + 0x001C))
#define DANUBE_CGU_SMD                  ((volatile u32*)(DANUBE_CGU + 0x0020))
#define DANUBE_CGU_CT1SR                ((volatile u32*)(DANUBE_CGU + 0x0028))
#define DANUBE_CGU_CT2SR                ((volatile u32*)(DANUBE_CGU + 0x002C))
#define DANUBE_CGU_PCMCR                ((volatile u32*)(DANUBE_CGU + 0x0030))
#define DANUBE_CGU_PCI_CR               ((volatile u32*)(DANUBE_CGU + 0x0034))
#define DANUBE_CGU_PD_PC                ((volatile u32*)(DANUBE_CGU + 0x0038))
#define DANUBE_CGU_FMR                  ((volatile u32*)(DANUBE_CGU + 0x003C))

/*
 *  CGU PLL0 Configure Register
 */
#define CGU_PLL0_PHASE_DIVIDER_ENABLE   (*DANUBE_CGU_PLL0_CFG & (1 << 31))
#define CGU_PLL0_BYPASS                 (*DANUBE_CGU_PLL0_CFG & (1 << 30))
#define CGU_PLL0_SRC                    (*DANUBE_CGU_PLL0_CFG & (1 << 29))
#define CGU_PLL0_CFG_DSMSEL             (*DANUBE_CGU_PLL0_CFG & (1 << 28))
#define CGU_PLL0_CFG_FRAC_EN            (*DANUBE_CGU_PLL0_CFG & (1 << 27))
#define CGU_PLL0_CFG_PLLK               GET_BITS(*DANUBE_CGU_PLL0_CFG, 26, 17)
//#define CGU_PLL0_CFG_PLLD               GET_BITS(*DANUBE_CGU_PLL0_CFG, 16, 13)
#define CGU_PLL0_CFG_PLLN               GET_BITS(*DANUBE_CGU_PLL0_CFG, 12, 6)
#define CGU_PLL0_CFG_PLLM               GET_BITS(*DANUBE_CGU_PLL0_CFG, 5, 2)
#define CGU_PLL0_CFG_PLLL               (*DANUBE_CGU_PLL0_CFG & (1 << 1))
#define CGU_PLL0_CFG_PLLEN              (*DANUBE_CGU_PLL0_CFG & (1 << 0))

/*
 *  CGU PLL1 Configure Register
 */
#define CGU_PLL1_SRC                    (*DANUBE_CGU_PLL1_CFG & (1 << 31))
#define CGU_PLL1_BYPASS                 (*DANUBE_CGU_PLL1_CFG & (1 << 30))
#define CGU_PLL1_CFG_CTEN               (*DANUBE_CGU_PLL1_CFG & (1 << 29))
#define CGU_PLL1_CFG_DSMSEL             (*DANUBE_CGU_PLL1_CFG & (1 << 28))
#define CGU_PLL1_CFG_FRAC_EN            (*DANUBE_CGU_PLL1_CFG & (1 << 27))
#define CGU_PLL1_CFG_PLLK               GET_BITS(*DANUBE_CGU_PLL1_CFG, 26, 17)
//#define CGU_PLL1_CFG_PLLD               GET_BITS(*DANUBE_CGU_PLL1_CFG, 16, 13)
#define CGU_PLL1_CFG_PLLN               GET_BITS(*DANUBE_CGU_PLL1_CFG, 12, 6)
#define CGU_PLL1_CFG_PLLM               GET_BITS(*DANUBE_CGU_PLL1_CFG, 5, 2)
#define CGU_PLL1_CFG_PLLL               (*DANUBE_CGU_PLL1_CFG & (1 << 1))
#define CGU_PLL1_CFG_PLLEN              (*DANUBE_CGU_PLL1_CFG & (1 << 0))

/*
 *  CGU PLL2 Configure/Status Register
 */
//#define CGU_PLL2_PHASE_DIVIDER_ENABLE   (*DANUBE_CGU_PLL2_CFG & (1 << 31))    //  Write bit 31, Read from bit 20
#define CGU_PLL2_PHASE_DIVIDER_ENABLE   (*DANUBE_CGU_PLL2_CFG & (1 << 20))
#define CGU_PLL2_BYPASS                 (*DANUBE_CGU_PLL2_CFG & (1 << 19))
#define CGU_PLL2_SRC                    GET_BITS(*DANUBE_CGU_PLL2_CFG, 18, 17)
#define CGU_PLL2_CFG_INPUT_DIV          GET_BITS(*DANUBE_CGU_PLL2_CFG, 16, 13)
#define CGU_PLL2_CFG_PLLN               GET_BITS(*DANUBE_CGU_PLL2_CFG, 12, 6)
#define CGU_PLL2_CFG_PLLM               GET_BITS(*DANUBE_CGU_PLL2_CFG, 5, 2)
#define CGU_PLL2_CFG_PLLL               (*DANUBE_CGU_PLL2_CFG & (1 << 1))
#define CGU_PLL2_CFG_PLLEN              (*DANUBE_CGU_PLL2_CFG & (1 << 0))

/*
 *  CGU Clock Sys Mux Register
 */
#define CGU_SYS_PPESEL                  GET_BITS(*DANUBE_CGU_SYS, 8, 7)
#define CGU_SYS_FPI_SEL                 (*DANUBE_CGU_SYS & (1 << 6))
#define CGU_SYS_CPU1SEL                 GET_BITS(*DANUBE_CGU_SYS, 5, 4)
#define CGU_SYS_CPU0SEL                 GET_BITS(*DANUBE_CGU_SYS, 3, 2)
#define CGU_SYS_DDR_SEL                 GET_BITS(*DANUBE_CGU_SYS, 1, 0)

/*
 *  CGU Update Register
 */
#define CGU_UPDATE_UPDATE               (*DANUBE_CGU_UPDATE & (1 << 0))

/*
 *  CGU Interface Clock Register
 */
#define CGU_IF_CLK_O_RMII1              (*DANUBE_CGU_IF_CLK & (1 << 25))
#define CGU_IF_CLK_O_RMII0              (*DANUBE_CGU_IF_CLK & (1 << 24))
#define CGU_IF_CLK_PCI_CLK              GET_BITS(*DANUBE_CGU_IF_CLK, 23, 20)
#define CGU_IF_CLK_PDA                  (*DANUBE_CGU_IF_CLK & (1 << 19))
#define CGU_IF_CLK_PCI_B                (*DANUBE_CGU_IF_CLK & (1 << 18))
#define CGU_IF_CLK_PCIBM                (*DANUBE_CGU_IF_CLK & (1 << 17))
#define CGU_IF_CLK_PCIS                 (*DANUBE_CGU_IF_CLK & (1 << 16))
#define CGU_IF_CLK_CLKOD0               GET_BITS(*DANUBE_CGU_IF_CLK, 15, 14)
#define CGU_IF_CLK_CLKOD1               GET_BITS(*DANUBE_CGU_IF_CLK, 13, 12)
#define CGU_IF_CLK_CLKOD2               GET_BITS(*DANUBE_CGU_IF_CLK, 11, 10)
#define CGU_IF_CLK_CLKOD3               GET_BITS(*DANUBE_CGU_IF_CLK, 9, 8)
//#define CGU_IF_CLK_PPESEL               GET_BITS(*DANUBE_CGU_IF_CLK, 7, 6)
#define CGU_IF_CLK_USBSEL               GET_BITS(*DANUBE_CGU_IF_CLK, 5, 4)
#define CGU_IF_CLK_MDCSEL               GET_BITS(*DANUBE_CGU_IF_CLK, 3, 2)
#define CGU_IF_CLK_MIISEL               GET_BITS(*DANUBE_CGU_IF_CLK, 1, 0)

/*
 *  CGU SDRAM Memory Delay Register
 */
//#define CGU_SMD_CLKI                    (*DANUBE_CGU_SMD & (1 << 31))
#define CGU_SMD_CLK_IN_S                (*DANUBE_CGU_SMD & (1 << 22))
#define CGU_SMD_DDR_PRG                 (*DANUBE_CGU_SMD & (1 << 21))
#define CGU_SMD_DDR_CQ                  (*DANUBE_CGU_SMD & (1 << 20))
#define CGU_SMD_DDR_EQ                  (*DANUBE_CGU_SMD & (1 << 19))
#define CGU_SMD_SDR_CLKS                (*DANUBE_CGU_SMD & (1 << 18))
#define CGU_SMD_MIDS                    GET_BITS(*DANUBE_CGU_SMD, 17, 12)
#define CGU_SMD_MODS                    GET_BITS(*DANUBE_CGU_SMD, 11, 6)
#define CGU_SMD_MDSEL                   GET_BITS(*DANUBE_CGU_SMD, 5, 0)

/*
 *  CGU CT Status Register 1
 */
#define CGU_CT1SR_PDOUT                 GET_BITS(*DANUBE_CGU_CT1SR, 14, 0)

/*
 *  CGU CT Status Register 2
 */
#define CGU_CT1SR_PLL1K                 GET_BITS(*DANUBE_CGU_CT2SR, 9, 0)

/*
 *  CGU PCM Control Register
 */
#define CGU_PCMCR_DCL1                  GET_BITS(*DANUBE_CGU_PCMCR, 27, 25)
#define CGU_PCMCR_MUXDCL                (*DANUBE_CGU_MUXDCL & (1 << 22))
#define CGU_PCMCR_MUXFSC                (*DANUBE_CGU_MUXDCL & (1 << 18))
#define CGU_PCMCR_PCM_SL                (*DANUBE_CGU_MUXDCL & (1 << 13))
#define CGU_PCMCR_DNTR                  GET_BITS(*DANUBE_CGU_PCMCR, 12, 11)
#define CGU_PCMCR_NTRS                  (*DANUBE_CGU_MUXDCL & (1 << 10))
#define CGU_PCMCR_AC97_EN               (*DANUBE_CGU_MUXDCL & (1 << 9))
#define CGU_PCMCR_CTTMUX                (*DANUBE_CGU_MUXDCL & (1 << 8))

/*
 *  PCI Clock Control Register
 */
#define CGU_PCI_CR_PADSEL               (*DANUBE_CGU_PCI_CR & (1 << 31))
#define CGU_PCI_CR_RESSEL               (*DANUBE_CGU_PCI_CR & (1 << 30))
#define CGU_PCI_CR_PCID_H               GET_BITS(*DANUBE_CGU_PCI_CR, 23, 21)
#define CGU_PCI_CR_PCID_L               GET_BITS(*DANUBE_CGU_PCI_CR, 20, 18)


/*
 * ####################################
 * Preparation of Debug on Amazon Chip
 * ####################################
 */

/*
 *  If try module on Amazon chip, prepare some tricks to prevent invalid memory write.
 */
#if defined(DEBUG_ON_AMAZON) && DEBUG_ON_AMAZON
    u32 g_pFakeRegisters[0x0100];

    #undef  DANUBE_CGU
    #define DANUBE_CGU                  ((u32)g_pFakeRegisters)
#endif  //  defined(DEBUG_ON_AMAZON) && DEBUG_ON_AMAZON

#if defined(DEBUG_ON_VENUS) && DEBUG_ON_VENUS
    #define VENUS_SYS_CLK               5000000
#endif  //  defined(DEBUG_ON_VENUS) && DEBUG_ON_VENUS


/*
 * ####################################
 *              Data Type
 * ####################################
 */


/*
 * ####################################
 *             Declaration
 * ####################################
 */

/*
 *  Pre-declaration of File Operations
 */
static ssize_t cgu_read(struct file *, char *, size_t, loff_t *);
static ssize_t cgu_write(struct file *, const char *, size_t, loff_t *);
static int cgu_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
static int cgu_open(struct inode *, struct file *);
static int cgu_release(struct inode *, struct file *);

/*
 *  Pre-declaration of 64-bit Unsigned Integer Operation
 */
static inline void uint64_multiply(unsigned int, unsigned int, unsigned int *);
static inline void uint64_divide(unsigned int *, unsigned int, unsigned int *, unsigned int *);

/*
 *  Calculate PLL Frequency
 */
static inline u32 get_input_clock(int pll);
static inline u32 cal_dsm(int, u32, u32);
static inline u32 mash_dsm(int, u32, u32, u32);
static inline u32 ssff_dsm_1(int, u32, u32, u32);
static inline u32 ssff_dsm_2(int, u32, u32, u32);
static inline u32 dsm(int, u32 M, u32, u32, u32, u32);
static inline u32 cgu_get_pll0_fosc(void);
static inline u32 cgu_get_pll0_fps(int);
static inline u32 cgu_get_pll0_fdiv(void);
static inline u32 cgu_get_pll1_fosc(void);
static inline u32 cgu_get_pll1_fps(void);
static inline u32 cgu_get_pll1_fdiv(void);
static inline u32 cgu_get_pll2_fosc(void);
static inline u32 cgu_get_pll2_fps(int);
static inline u32 cgu_get_pll2_fdiv(void);

/*
 *  Export Functions
 */
u32 cgu_get_mips_clock(int);
u32 cgu_get_cpu_clock(void);
u32 cgu_get_io_region_clock(void);
u32 cgu_get_fpi_bus_clock(int);
u32 cgu_get_pp32_clock(void);
u32 cgu_get_pci_clock(void);
u32 cgu_get_ethernet_clock(int);
u32 cgu_get_usb_clock(void);
u32 cgu_get_clockout(int);
int danube_cgu_info(char* sysbuf, char** p_mybuf, off_t offset, int l_sysbuf, int zero);


/*
 * ####################################
 *            Local Variable
 * ####################################
 */

static struct file_operations cgu_fops = {
    owner:      THIS_MODULE,
    llseek:     no_llseek,
    read:       cgu_read,
    write:      cgu_write,
    ioctl:      cgu_ioctl,
    open:       cgu_open,
    release:    cgu_release
};

static struct miscdevice cgu_miscdev = {
    MISC_DYNAMIC_MINOR,
    "danube_cgu_dev",
    &cgu_fops
};


/*
 * ####################################
 *           Global Variable
 * ####################################
 */


/*
 * ####################################
 *            Local Function
 * ####################################
 */

static ssize_t cgu_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
    return -EPERM;
}

static ssize_t cgu_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
    return -EPERM;
}

static int cgu_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    struct cgu_clock_rates rates;

    if ( _IOC_TYPE(cmd) != CGU_IOC_MAGIC
        || _IOC_NR(cmd) >= CGU_IOC_MAXNR )
        return -ENOTTY;

    if ( _IOC_DIR(cmd) & _IOC_READ )
        ret = !access_ok(VERIFY_WRITE, arg, _IOC_SIZE(cmd));
    else if ( _IOC_DIR(cmd) & _IOC_WRITE )
        ret = !access_ok(VERIFY_READ, arg, _IOC_SIZE(cmd));
    if ( ret )
        return -EFAULT;

    switch ( cmd )
    {
    case CGU_GET_CLOCK_RATES:
        /*  Calculate Clock Rates   */
        rates.mips0     = cgu_get_mips_clock(0);
        rates.mips1     = cgu_get_mips_clock(1);
        rates.cpu       = cgu_get_cpu_clock();
        rates.io_region = cgu_get_io_region_clock();
        rates.fpi_bus1  = cgu_get_fpi_bus_clock(1);
        rates.fpi_bus2  = cgu_get_fpi_bus_clock(2);
        rates.pp32      = cgu_get_pp32_clock();
        rates.pci       = cgu_get_pci_clock();
        rates.mii0      = cgu_get_ethernet_clock(0);
        rates.mii1      = cgu_get_ethernet_clock(1);
        rates.usb       = cgu_get_usb_clock();
        rates.clockout0 = cgu_get_clockout(0);
        rates.clockout1 = cgu_get_clockout(1);
        rates.clockout2 = cgu_get_clockout(2);
        rates.clockout3 = cgu_get_clockout(3);
        /*  Copy to User Space      */
        copy_to_user((char*)arg, (char*)&rates, sizeof(rates));

        ret = 0;
        break;
    default:
        ret = -ENOTTY;
    }

    return ret;
}

static int cgu_open(struct inode *inode, struct file *file)
{
    return 0;
}

static int cgu_release(struct inode *inode, struct file *file)
{
    return 0;
}

/*
 *  Description:
 *    calculate 64-bit multiplication result of two 32-bit unsigned integer
 *  Input:
 *    u32Multiplier1 --- u32 (32-bit), one of the multipliers
 *    u32Multiplier2 --- u32 (32-bit), the other multiplier
 *    u32Result      --- u32[2], array to retrieve the multiplication result,
 *                       index 0 is high word, index 1 is low word
 *  Output:
 *    none
 */
static inline void uint64_multiply(u32 u32Multiplier1, u32 u32Multiplier2, u32 u32Result[2])
{
	u32 u32Multiplier1LowWord = u32Multiplier1 & 0xFFFF;
	u32 u32Multiplier1HighWord = u32Multiplier1 >> 16;
	u32 u32Multiplier2LowWord = u32Multiplier2 & 0xFFFF;
	u32 u32Multiplier2HighWord = u32Multiplier2 >> 16;
	u32 u32Combo1, u32Combo2, u32Combo3, u32Combo4;
	u32 u32Word1, u32Word2, u32Word3, u32Word4;

	u32Combo1 = u32Multiplier1LowWord * u32Multiplier2LowWord;
	u32Combo2 = u32Multiplier1HighWord * u32Multiplier2LowWord;
	u32Combo3 = u32Multiplier1LowWord * u32Multiplier2HighWord;
	u32Combo4 = u32Multiplier1HighWord * u32Multiplier2HighWord;

	u32Word1 = u32Combo1 & 0xFFFF;
	u32Word2 = (u32Combo1 >> 16) + (u32Combo2 & 0xFFFF) + (u32Combo3 & 0xFFFF);
	u32Word3 = (u32Combo2 >> 16) + (u32Combo3 >> 16) + (u32Combo4 & 0xFFFF) + (u32Word2 >> 16);
	u32Word4 = (u32Combo4 >> 16) + (u32Word3 >> 16);

	u32Result[0] = (u32Word4 << 16) | u32Word3;
	u32Result[1] = (u32Word2 << 16) | u32Word1;
}

/*
 *  Description:
 *    divide 64-bit unsigned integer with 32-bit unsigned integer
 *  Input:
 *    u32Numerator   --- u32[2], index 0 is high word of numerator, while
 *                       index 1 is low word of numerator
 *    u32Denominator --- u32 (32-bit), the denominator in division, this
 *                       parameter can not be zero, or lead to unpredictable
 *                       result
 *    pu32Quotient   --- u32 *, the pointer to retrieve 32-bit quotient, null
 *                       pointer means ignore quotient
 *    pu32Residue    --- u32 *, the pointer to retrieve 32-bit residue null
 *                       pointer means ignore residue
 *  Output:
 *    none
 */
static inline void uint64_divide(u32 u32Numerator[2], u32 u32Denominator, u32 *pu32Quotient, u32 *pu32Residue)
{
	u32 u32DWord1, u32DWord2, u32DWord3;
	u32 u32Quotient;
	int i;

	u32DWord3 = 0;
	u32DWord2 = u32Numerator[0];
	u32DWord1 = u32Numerator[1];

	u32Quotient = 0;

	for ( i = 0; i < 64; i++ )
	{
		u32DWord3 = (u32DWord3 << 1) | (u32DWord2 >> 31);
		u32DWord2 = (u32DWord2 << 1) | (u32DWord1 >> 31);
		u32DWord1 <<= 1;
		u32Quotient <<= 1;
		if ( u32DWord3 >= u32Denominator )
		{
			u32DWord3 -= u32Denominator;
			u32Quotient |= 1;
		}
	}
	if ( pu32Quotient )
	    *pu32Quotient = u32Quotient;
	if ( pu32Residue )
	    *pu32Residue = u32DWord3;
}

/*
 *  Description:
 *    get input clock frequency according to GPIO config
 *  Input:
 *    none
 *  Output:
 *    u32 --- frequency of input clock
 */
static inline u32 get_input_clock(int pll)
{
    //  according to Spec
    //    gpio4 is used as pin strapping
    //    0: 35.328MHz
    //    1: 36MHz
    //  however, don't know which port it is, port 0 or port 1
    //  a fake value is used now
    switch ( pll )
    {
    case 0:
        if ( CGU_PLL0_SRC )
            return BASIS_INPUT_CRYSTAL_USB;
        else if ( CGU_PLL0_PHASE_DIVIDER_ENABLE )   //  This bit should be set correctly by ROM code
            return BASIC_INPUT_CLOCK_FREQUENCY_1;
        else
            return BASIC_INPUT_CLOCK_FREQUENCY_2;
    case 1:
        if ( CGU_PLL1_SRC )
            return BASIS_INPUT_CRYSTAL_USB;
        else if ( CGU_PLL0_PHASE_DIVIDER_ENABLE )   //  Same source as PLL0
            return BASIC_INPUT_CLOCK_FREQUENCY_1;
        else
            return BASIC_INPUT_CLOCK_FREQUENCY_2;
    case 2:
        switch ( CGU_PLL2_SRC )
        {
        case 0:
            return cgu_get_pll0_fdiv();
        case 1:
            return CGU_PLL2_PHASE_DIVIDER_ENABLE ? BASIC_INPUT_CLOCK_FREQUENCY_1 : BASIC_INPUT_CLOCK_FREQUENCY_2;
        case 2:
            return BASIS_INPUT_CRYSTAL_USB;
        }
    default:
        return 0;
    }
}

/*
 *  Description:
 *    common routine to calculate PLL frequency
 *  Input:
 *    num --- u32, numerator
 *    den --- u32, denominator
 *  Output:
 *    u32 --- frequency the PLL output
 */
static inline u32 cal_dsm(int pll, u32 num, u32 den)
{
    u32 ret;
    u32 temp[2];
    u32 residue;

//    den <<= 1;  //  [(n+1)*freq] / [(m+1)*2], den = (m+1)*2
    uint64_multiply(num, get_input_clock(pll), temp);
    uint64_divide(temp, den, &ret, &residue);
    if ( (residue << 1) >= den )
        ret++;

    return ret;
}

/*
 *  Description:
 *    calculate PLL frequency following MASH-DSM
 *  Input:
 *    M   --- u32, denominator coefficient
 *    N   --- u32, numerator integer coefficient
 *    K   --- u32, numerator fraction coefficient
 *  Output:
 *    u32 --- frequency the PLL output
 */
static inline u32 mash_dsm(int pll, u32 M, u32 N, u32 K)
{
    u32 num = ((N + 1) << 10) + K;
    u32 den = (M + 1) << 10;

    return cal_dsm(pll, num, den);
}

/*
 *  Description:
 *    calculate PLL frequency following SSFF-DSM (0.25 < fraction < 0.75)
 *  Input:
 *    M   --- u32, denominator coefficient
 *    N   --- u32, numerator integer coefficient
 *    K   --- u32, numerator fraction coefficient
 *  Output:
 *    u32 --- frequency the PLL output
 */
static inline u32 ssff_dsm_1(int pll, u32 M, u32 N, u32 K)
{
    u32 num = ((N + 1) << 11) + K + 512;
    u32 den = (M + 1) << 11;

    return cal_dsm(pll, num, den);
}

/*
 *  Description:
 *    calculate PLL frequency following SSFF-DSM
 *    (fraction < 0.125 || fraction > 0.875)
 *  Input:
 *    M   --- u32, denominator coefficient
 *    N   --- u32, numerator integer coefficient
 *    K   --- u32, numerator fraction coefficient
 *  Output:
 *    u32 --- frequency the PLL output
 */
static inline u32 ssff_dsm_2(int pll, u32 M, u32 N, u32 K)
{
    u32 num = K >= 512 ? ((N + 1) << 12) + K - 512 : ((N + 1) << 12) + K + 3584;
    u32 den = (M + 1) << 12;

    return cal_dsm(pll, num, den);
}

/*
 *  Description:
 *    calculate PLL frequency
 *  Input:
 *    M            --- u32, denominator coefficient
 *    N            --- u32, numerator integer coefficient
 *    K            --- u32, numerator fraction coefficient
 *    dsmsel       --- int, 0: MASH-DSM, 1: SSFF-DSM
 *    phase_div_en --- int, 0: 0.25 < fraction < 0.75
 *                          1: fraction < 0.125 || fraction > 0.875
 *  Output:
 *    u32          --- frequency the PLL output
 */
static inline u32 dsm(int pll, u32 M, u32 N, u32 K, u32 dsmsel, u32 phase_div_en)
{
    if ( !dsmsel )
        return mash_dsm(pll, M, N, K);
    else
#if !defined(FIX_FOR_36M_CRYSTAL) || !FIX_FOR_36M_CRYSTAL
        if ( !phase_div_en )
            return ssff_dsm_1(pll, M, N, K);
        else
            return ssff_dsm_2(pll, M, N, K);
#else
        if ( !phase_div_en )
            return mash_dsm(pll, M, N, K);
        else
            return ssff_dsm_2(pll, M, N, K);
#endif
}

/*
 *  Description:
 *    get oscillate frequency of PLL0
 *  Input:
 *    none
 *  Output:
 *    u32 --- frequency of PLL0 Fosc
 */
static inline u32 cgu_get_pll0_fosc(void)
{
//    printk("get_input_clock(0) = %d, CGU_PLL0_CFG_PLLN = %d, CGU_PLL0_CFG_PLLM = %d, CGU_PLL0_CFG_PLLK = %d, CGU_PLL0_CFG_DSMSEL = %d\n", get_input_clock(0), CGU_PLL0_CFG_PLLN, CGU_PLL0_CFG_PLLM, CGU_PLL0_CFG_PLLK, CGU_PLL0_CFG_DSMSEL >> 28);

    if ( CGU_PLL0_BYPASS )
        get_input_clock(0);
    else
        return !CGU_PLL0_CFG_FRAC_EN
               ? dsm(0, CGU_PLL0_CFG_PLLM, CGU_PLL0_CFG_PLLN, 0, CGU_PLL0_CFG_DSMSEL, CGU_PLL0_PHASE_DIVIDER_ENABLE)
               : dsm(0, CGU_PLL0_CFG_PLLM, CGU_PLL0_CFG_PLLN, CGU_PLL0_CFG_PLLK, CGU_PLL0_CFG_DSMSEL, CGU_PLL0_PHASE_DIVIDER_ENABLE);
}

/*
 *  Description:
 *    get output frequency of PLL0 phase shifter
 *  Input:
 *    phase --- int, 1: 1.25 divider, 2: 1.5 divider
 *  Output:
 *    u32   --- frequency of PLL0 Fps
 */
static inline u32 cgu_get_pll0_fps(int phase)
{
    register u32 fps = cgu_get_pll0_fosc();

    switch ( phase )
    {
    case 1:
        /*  1.25    */
        fps = ((fps << 2) + 2) / 5; break;
    case 2:
        /*  1.5     */
        fps = ((fps << 1) + 1) / 3; break;
    }
    return fps;
}

/*
 *  Description:
 *    get output frequency of PLL0 output divider
 *  Input:
 *    none
 *  Output:
 *    u32 --- frequency of PLL0 Fdiv
 */
static inline u32 cgu_get_pll0_fdiv(void)
{
#if 1
    register u32 div = CGU_PLL2_CFG_INPUT_DIV + 1;

    return (cgu_get_pll0_fosc() + (div >> 1)) / div;
#else
    return (cgu_get_pll0_fosc() + 2) / 5;
#endif
}

/*
 *  Description:
 *    get oscillate frequency of PLL1
 *  Input:
 *    none
 *  Output:
 *    u32 --- frequency of PLL1 Fosc
 */
static inline u32 cgu_get_pll1_fosc(void)
{
    if ( CGU_PLL1_BYPASS )
        get_input_clock(1);
    else
        return !CGU_PLL1_CFG_FRAC_EN
               ? dsm(1, CGU_PLL1_CFG_PLLM, CGU_PLL1_CFG_PLLN, 0, CGU_PLL1_CFG_DSMSEL, 0)
               : dsm(1, CGU_PLL1_CFG_PLLM, CGU_PLL1_CFG_PLLN, CGU_PLL1_CFG_PLLK, CGU_PLL1_CFG_DSMSEL, 0);
}

/*
 *  Description:
 *    get output frequency of PLL1 phase shifter
 *  Input:
 *    none
 *  Output:
 *    u32 --- frequency of PLL1 Fps
 */
static inline u32 cgu_get_pll1_fps(void)
{
    register u32 fps = cgu_get_pll1_fosc();

    switch ( 1 )
    {
    case 1:
        /*  1.5     */
        fps = ((fps << 1) + 1) / 3; break;
    case 2:
        /*  1.25    */
        fps = ((fps << 2) + 2) / 5; break;
    case 3:
        /*  3.5     */
        fps = ((fps << 1) + 3) / 7;
    }
    return fps;
}

/*
 *  Description:
 *    get output frequency of PLL1 output divider
 *  Input:
 *    none
 *  Output:
 *    u32 --- frequency of PLL1 Fdiv
 */
static inline u32 cgu_get_pll1_fdiv(void)
{
#if 0
    register u32 div = CGU_PLL1_CFG_PLLD + 1;

    return (cgu_get_pll1_fosc() + (div >> 1)) / div;
#else
    return cgu_get_pll1_fosc();
#endif
}

/*
 *  Description:
 *    get oscillate frequency of PLL2
 *  Input:
 *    none
 *  Output:
 *    u32 --- frequency of PLL2 Fosc
 */
static inline u32 cgu_get_pll2_fosc(void)
{
    u32 ret;
    u32 temp[2];
    u32 residue;

    if ( CGU_PLL2_BYPASS )
        return get_input_clock(2);

//    printk("get_input_clock(2) = %d, CGU_PLL2_CFG_PLLN = %d, CGU_PLL2_CFG_PLLM = %d\n", (u32)get_input_clock(2), (u32)CGU_PLL2_CFG_PLLN, (u32)CGU_PLL2_CFG_PLLM);

    uint64_multiply(CGU_PLL2_CFG_PLLN + 1, get_input_clock(2), temp);
    uint64_divide(temp, CGU_PLL2_CFG_PLLM + 1, &ret, &residue);
    if ( (residue << 1) > CGU_PLL2_CFG_PLLM )
        ret++;

    return ret;
}

/*
 *  Description:
 *    get output frequency of PLL2 phase shifter
 *  Input:
 *    phase --- int, 1: 1.125 divider, 2: 1.25 divider
 *  Output:
 *    u32   --- frequency of PLL2 Fps
 */
static inline u32 cgu_get_pll2_fps(int phase)
{
    register u32 fps = cgu_get_pll2_fosc();

    switch ( phase )
    {
    case 1:
        /*  1.125   */
        fps = ((fps << 2) + 2) / 5; break;
    case 2:
        /*  1.25    */
        fps = ((fps << 3) + 4) / 9;
    }

    return fps;
}

/*
 *  Description:
 *    get output frequency of PLL2 output divider (used for PCI clock)
 *  Input:
 *    none
 *  Output:
 *    u32   --- frequency of PLL2 Fdiv
 */
static inline u32 cgu_get_pll2_fdiv(void)
{
    register u32 div = CGU_IF_CLK_PCI_CLK + 1;

//    printk("*DANUBE_CGU_PLL0_CFG = 0x%08X, CGU_PLL0_BYPASS = %d, CGU_PLL0_CFG_PLLD = %d\n", (u32)*DANUBE_CGU_PLL0_CFG, (u32)CGU_PLL0_BYPASS, (u32)CGU_PLL0_CFG_PLLD);
//    printk("*DANUBE_CGU_PLL1_CFG = 0x%08X, CGU_PLL1_BYPASS = %d, CGU_PLL1_CFG_PLLD = %d\n", (u32)*DANUBE_CGU_PLL1_CFG, (u32)CGU_PLL1_BYPASS, (u32)CGU_PLL1_CFG_PLLD);
//    printk("*DANUBE_CGU_PLL2_CFG = 0x%08X, CGU_PLL2_CFG_PCIDIV = %d\n", (u32)*DANUBE_CGU_PLL2_CFG, (u32)CGU_PLL2_CFG_PCIDIV);

    return (cgu_get_pll2_fosc() + (div >> 1)) / div;
}


/*
 * ####################################
 *           Global Function
 * ####################################
 */

/*
 *  Description:
 *    get frequency of MIPS (0: core, 1: DSP)
 *  Input:
 *    cpu --- int, 0: core, 1: DSP
 *  Output:
 *    u32 --- frequency of MIPS coprocessor (0: core, 1: DSP)
 */
u32 cgu_get_mips_clock(int cpu)
{
#if !defined(DEBUG_ON_VENUS) || !DEBUG_ON_VENUS
    register u32 ret = cgu_get_pll0_fosc();
    register u32 cpusel = cpu == 0 ? CGU_SYS_CPU0SEL : CGU_SYS_CPU1SEL;

	printk(KERN_INFO "%s %s %d: pll0_fosc = %d, cpusel = %#x\n", __FILE__, __func__, __LINE__, ret, cpusel);
    if ( cpusel == 0 )
        return ret;
    else if ( cpusel == 2 )
        ret <<= 1;

    switch ( CGU_SYS_DDR_SEL )
    {
    default:
    case 0:
        return (ret + 1) / 2;
    case 1:
        return (ret * 2 + 2) / 5;
    case 2:
        return (ret + 1) / 3;
    case 3:
        return (ret + 2) / 4;
    }
#else
    return VENUS_SYS_CLK / 2;
#endif
}

/*
 *  Description:
 *    get frequency of MIPS core
 *  Input:
 *    none
 *  Output:
 *    u32 --- frequency of MIPS core
 */
u32 cgu_get_cpu_clock(void)
{
	printk(KERN_INFO "%s %s %d\n", __FILE__, __func__, __LINE__);
	return cgu_get_mips_clock(0);
}

/*
 *  Description:
 *    get frequency of sub-system and memory controller
 *  Input:
 *    none
 *  Output:
 *    u32 --- frequency of sub-system and memory controller
 */
u32 cgu_get_io_region_clock(void)
{
#if !defined(DEBUG_ON_VENUS) || !DEBUG_ON_VENUS
    register u32 ret = cgu_get_pll0_fosc();

    switch ( CGU_SYS_DDR_SEL )
    {
    default:
    case 0:
        return (ret + 1) / 2;
    case 1:
        return (ret * 2 + 2) / 5;
    case 2:
        return (ret + 1) / 3;
    case 3:
        return (ret + 2) / 4;
    }
#else
    return cgu_get_mips_clock(0) / 2;
#endif
}

/*
 *  Description:
 *    get frequency of FPI bus
 *  Input:
 *    fpi --- int, 1: FPI bus 1 (FBS1/Fast FPI Bus), 2: FPI bus 2 (FBS2)
 *  Output:
 *    u32 --- frequency of FPI bus
 */
u32 cgu_get_fpi_bus_clock(int fpi)
{
    register u32 ret = cgu_get_io_region_clock();

    if ( fpi == 2 )
        if ( CGU_SYS_FPI_SEL )
            ret >>= 1;

    return ret;
}

/*
 *  Description:
 *    get frequency of PP32 processor
 *  Input:
 *    none
 *  Output:
 *    u32 --- frequency of PP32 processor
 */
u32 cgu_get_pp32_clock(void)
{
#if !defined(DEBUG_ON_VENUS) || !DEBUG_ON_VENUS
    switch ( CGU_SYS_PPESEL )
    {
    default:
    case 0:
        return cgu_get_pll2_fps(1);
    case 1:
        return cgu_get_pll2_fps(2);
    case 2:
        return (cgu_get_pll2_fps(1) + 1) >> 1;
    case 3:
        return (cgu_get_pll2_fps(2) + 1) >> 1;
    }
#else
    return cgu_get_fpi_bus_clock(1);
#endif
}

/*
 *  Description:
 *    get frequency of PCI bus
 *  Input:
 *    none
 *  Output:
 *    u32 --- frequency of PCI bus
 */
u32 cgu_get_pci_clock(void)
{
#if !defined(DEBUG_ON_VENUS) || !DEBUG_ON_VENUS
  #if 0
    switch ( CGU_IF_CLK_PCI_CLK )
    {
    default:
    case 0x04: return 60000000;
    case 0x08: return 33333333;
    case 0x0C: return 23077000;
    }
  #else
    return cgu_get_pll2_fdiv();
  #endif
#else
    return VENUS_SYS_CLK / 4 / 5;
#endif
}

/*
 *  Description:
 *    get frequency of ethernet module (MII)
 *  Input:
 *    mii --- int, 0: mii0, 1: mii1
 *  Output:
 *    u32 --- frequency of ethernet module
 */
u32 cgu_get_ethernet_clock(int mii)
{
#if !defined(DEBUG_ON_VENUS) || !DEBUG_ON_VENUS
    switch ( CGU_IF_CLK_MIISEL )
    {
    case 0:
        return (cgu_get_pll2_fosc() + 3) / 12;
    case 1:
        return (cgu_get_pll2_fosc() + 3) / 6;
    case 2:
        return 50000000;
    case 3:
        return 25000000;
    }
    return 0;
#else
    return VENUS_SYS_CLK / 4 / 12;
#endif
}

/*
 *  Description:
 *    get frequency of USB
 *  Input:
 *    none
 *  Output:
 *    u32 --- frequency of USB
 */
u32 cgu_get_usb_clock(void)
{
#if !defined(DEBUG_ON_VENUS) || !DEBUG_ON_VENUS
    switch ( CGU_IF_CLK_USBSEL )
    {
    case 0:
        return (cgu_get_pll2_fosc() + 12) / 25;
    case 1:
        return 12000000;
    case 2:
        return 12000000 / 4;
    case 3:
        return 12000000;
    }
    return 0;
#else
    return VENUS_SYS_CLK / 25;
#endif
}

/*
 *  Description:
 *    get frequency of CLK_OUT pin
 *  Input:
 *    clkout --- int, clock out pin number
 *  Output:
 *    u32    --- frequency of CLK_OUT pin
 */
u32 cgu_get_clockout(int clkout)
{
#if !defined(DEBUG_ON_VENUS) || !DEBUG_ON_VENUS
    u32 fosc1 = cgu_get_pll1_fosc();
    u32 fosc2 = cgu_get_pll2_fosc();

    if ( clkout > 3 || clkout < 0 )
        return 0;

    switch ( ((u32)clkout << 2) | GET_BITS(*DANUBE_CGU_IF_CLK, 15 - clkout * 2, 14 - clkout * 2) )
    {
    case 0: /*  32.768KHz   */
    case 15:
        return (fosc1 + 6000) / 12000;
    case 1: /*  1.536MHz    */
        return (fosc1 + 128) / 256;
    case 2: /*  2.5MHz      */
        return (fosc2 + 60) / 120;
    case 3: /*  12MHz       */
    case 5:
    case 12:
        return (fosc2 + 12) / 25;
    case 4: /*  40MHz       */
        return (cgu_get_pll2_fps(2) + 3) / 6;
    case 6: /*  24MHz       */
        return (cgu_get_pll2_fps(2) + 5) / 10;
    case 7: /*  48MHz       */
        return (cgu_get_pll2_fps(2) + 2) / 5;
    case 8: /*  25MHz       */
    case 14:
        return (fosc2 + 6) / 12;
    case 9: /*  50MHz       */
    case 13:
        return (fosc2 + 3) / 6;
    case 10:/*  30MHz       */
        return (fosc2 + 5) / 10;
    case 11:/*  60MHz       */
        return (fosc2 + 2) / 5;
    }

    return 0;
#else
    return 0;
#endif
}


/*
 * ####################################
 *           Init/Cleanup API
 * ####################################
 */
#define CGU_PROC "danube_cgu"
struct proc_dir_entry *cgu_proc = NULL;
/*
 *  Description:
*    register device
 *  Input:
 *    none
 *  Output:
 *    0    --- successful
 *    else --- failure, usually it is negative value of error code
 */
int __init danube_cgu_init(void)
{
    int ret;

    ret = misc_register(&cgu_miscdev);
    if ( ret )
    {
        printk(KERN_ERR "cgu: can't misc_register\n");
        return ret;
    }
    else
        printk(KERN_INFO "cgu: misc_register on minor = %d\n", cgu_miscdev.minor);

    /*
     *  initialize fake registers to do testing on Amazon
     */
#if defined(DEBUG_ON_AMAZON) && DEBUG_ON_AMAZON
    #ifdef  DEBUG_PRINT_INFO
    #undef  DEBUG_PRINT_INFO
    #endif
    #define DEBUG_PRINT_INFO    1

    *DANUBE_CGU_PLL0_CFG    = 0x00000000;
    *DANUBE_CGU_PLL1_CFG    = 0x00000000;
    *DANUBE_CGU_PLL2_CFG    = 0x00000000;
    *DANUBE_CGU_SYS         = 0x00000000;
    *DANUBE_CGU_UPDATE      = 0x00000000;
    *DANUBE_CGU_IF_CLK      = 0x00000000;
    *DANUBE_CGU_SMD         = 0x00000000;
    *DANUBE_CGU_CT1SR       = 0x00000000;
    *DANUBE_CGU_CT2SR       = 0x00000000;
    *DANUBE_CGU_PCMCR       = 0x00000000;
    *DANUBE_CGU_PCI_CR      = 0x00000000;
#endif  //  defined(DEBUG_ON_AMAZON) && DEBUG_ON_AMAZON

    /*
     *  for testing only
     */
#if defined(DEBUG_PRINT_INFO) && DEBUG_PRINT_INFO
//    printk("pll0 N = %d, M = %d, K = %d, DIV = %d\n", CGU_PLL0_CFG_PLLN, CGU_PLL0_CFG_PLLM, CGU_PLL0_CFG_PLLK, CGU_PLL0_CFG_PLLD);
    printk("pll0 N = %d, M = %d, K = %d\n", CGU_PLL0_CFG_PLLN, CGU_PLL0_CFG_PLLM, CGU_PLL0_CFG_PLLK);
//    printk("pll1 N = %d, M = %d, K = %d, DIV = %d\n", CGU_PLL1_CFG_PLLN, CGU_PLL1_CFG_PLLM, CGU_PLL1_CFG_PLLK, CGU_PLL1_CFG_PLLD);
    printk("pll1 N = %d, M = %d, K = %d\n", CGU_PLL1_CFG_PLLN, CGU_PLL1_CFG_PLLM, CGU_PLL1_CFG_PLLK);
//    printk("pll2 N = %d, M = %d, DIV = %d\n", CGU_PLL2_CFG_PLLN, CGU_PLL2_CFG_PLLM, CGU_PLL2_CFG_PCIDIV);
    printk("pll2 N = %d, M = %d, INPUT DIV = %d\n", CGU_PLL2_CFG_PLLN, CGU_PLL2_CFG_PLLM, CGU_PLL2_CFG_INPUT_DIV);
    printk("pll0_fosc    = %d\n", cgu_get_pll0_fosc());
    printk("pll0_fps(1)  = %d\n", cgu_get_pll0_fps(1));
    printk("pll0_fps(2)  = %d\n", cgu_get_pll0_fps(2));
    printk("pll0_fdiv    = %d\n", cgu_get_pll0_fdiv());
    printk("pll1_fosc    = %d\n", cgu_get_pll1_fosc());
    printk("pll1_fps     = %d\n", cgu_get_pll1_fps());
    printk("pll1_fdiv    = %d\n", cgu_get_pll1_fdiv());
    printk("pll2_fosc    = %d\n", cgu_get_pll2_fosc());
    printk("pll2_fps(1)  = %d\n", cgu_get_pll2_fps(1));
    printk("pll2_fps(2)  = %d\n", cgu_get_pll2_fps(2));
    printk("mips0 clock  = %d\n", cgu_get_mips_clock(0));
    printk("mips1 clock  = %d\n", cgu_get_mips_clock(1));
    printk("cpu clock    = %d\n", cgu_get_cpu_clock());
    printk("IO region    = %d\n", cgu_get_io_region_clock());
    printk("FPI bus 1    = %d\n", cgu_get_fpi_bus_clock(1));
    printk("FPI bus 2    = %d\n", cgu_get_fpi_bus_clock(2));
    printk("PP32 clock   = %d\n", cgu_get_pp32_clock());
    printk("PCI clock    = %d\n", cgu_get_pci_clock());
    printk("Ethernet MII0= %d\n", cgu_get_ethernet_clock(0));
    printk("Ethernet MII1= %d\n", cgu_get_ethernet_clock(1));
    printk("USB clock    = %d\n", cgu_get_usb_clock());
    printk("Clockout0    = %d\n", cgu_get_clockout(0));
    printk("Clockout1    = %d\n", cgu_get_clockout(1));
    printk("Clockout2    = %d\n", cgu_get_clockout(2));
    printk("Clockout3    = %d\n", cgu_get_clockout(3));
#endif  //  defined(DEBUG_PRINT_INFO) && DEBUG_PRINT_INFO
	cgu_proc = create_proc_entry(CGU_PROC, S_IFREG|S_IRUGO,NULL);
	if(cgu_proc) {
		cgu_proc->nlink = 1;
		cgu_proc->read_proc = danube_cgu_info;
		ret = 0;
	}
	else ret = -1;

    return ret;
}
/* the /proc function: allocate everything to allow concurrency */
int danube_cgu_info(char* sysbuf, char** p_mybuf, off_t offset, int l_sysbuf, int zero)
{
        static  int len=0;
	if(offset>0) {
		len=0;
		return 0;
	}

    len += sprintf(sysbuf + len, "pll0          : N = %d, M = %d, K = %d\n", CGU_PLL0_CFG_PLLN, CGU_PLL0_CFG_PLLM, CGU_PLL0_CFG_PLLK);
    len += sprintf(sysbuf + len, "pll1          : N = %d, M = %d, K = %d\n", CGU_PLL1_CFG_PLLN, CGU_PLL1_CFG_PLLM, CGU_PLL1_CFG_PLLK);
    len += sprintf(sysbuf + len, "pll2          : N = %d, M = %d, INPUT DIV = %d\n", CGU_PLL2_CFG_PLLN, CGU_PLL2_CFG_PLLM, CGU_PLL2_CFG_INPUT_DIV);
    len += sprintf(sysbuf + len, "pll0_fosc     = %d\n", cgu_get_pll0_fosc());
    len += sprintf(sysbuf + len, "pll0_fps(1)   = %d\n", cgu_get_pll0_fps(1));
    len += sprintf(sysbuf + len, "pll0_fps(2)   = %d\n", cgu_get_pll0_fps(2));
    len += sprintf(sysbuf + len, "pll0_fdiv     = %d\n", cgu_get_pll0_fdiv());
    len += sprintf(sysbuf + len, "pll1_fosc     = %d\n", cgu_get_pll1_fosc());
    len += sprintf(sysbuf + len, "pll1_fps      = %d\n", cgu_get_pll1_fps());
    len += sprintf(sysbuf + len, "pll1_fdiv     = %d\n", cgu_get_pll1_fdiv());
    len += sprintf(sysbuf + len, "pll2_fosc     = %d\n", cgu_get_pll2_fosc());
    len += sprintf(sysbuf + len, "pll2_fps(1)   = %d\n", cgu_get_pll2_fps(1));
    len += sprintf(sysbuf + len, "pll2_fps(2)   = %d\n", cgu_get_pll2_fps(2));
    len += sprintf(sysbuf + len, "mips0 clock   = %d\n", cgu_get_mips_clock(0));
    len += sprintf(sysbuf + len, "mips1 clock   = %d\n", cgu_get_mips_clock(1));
    len += sprintf(sysbuf + len, "cpu clock     = %d\n", cgu_get_cpu_clock());
    len += sprintf(sysbuf + len, "IO region     = %d\n", cgu_get_io_region_clock());
    len += sprintf(sysbuf + len, "FPI bus 1     = %d\n", cgu_get_fpi_bus_clock(1));
    len += sprintf(sysbuf + len, "FPI bus 2     = %d\n", cgu_get_fpi_bus_clock(2));
    len += sprintf(sysbuf + len, "PP32 clock    = %d\n", cgu_get_pp32_clock());
    len += sprintf(sysbuf + len, "PCI clock     = %d\n", cgu_get_pci_clock());
    len += sprintf(sysbuf + len, "Ethernet MII0 = %d\n", cgu_get_ethernet_clock(0));
    len += sprintf(sysbuf + len, "Ethernet MII1 = %d\n", cgu_get_ethernet_clock(1));
    len += sprintf(sysbuf + len, "USB clock     = %d\n", cgu_get_usb_clock());
    len += sprintf(sysbuf + len, "Clockout0     = %d\n", cgu_get_clockout(0));
    len += sprintf(sysbuf + len, "Clockout1     = %d\n", cgu_get_clockout(1));
    len += sprintf(sysbuf + len, "Clockout2     = %d\n", cgu_get_clockout(2));
    len += sprintf(sysbuf + len, "Clockout3     = %d\n", cgu_get_clockout(3));
	return len;
}

/*
 *  Description:
 *    deregister device
 *  Input:
 *    none
 *  Output:
 *    none
 */
void __exit danube_cgu_exit(void)
{
    int ret;

    ret = misc_deregister(&cgu_miscdev);
    if ( ret )
        printk(KERN_ERR "cgu: can't misc_deregister, get error number %d\n", -ret);
    else
        printk(KERN_INFO "cgu: misc_deregister successfully\n");
}

EXPORT_SYMBOL(cgu_get_mips_clock);
EXPORT_SYMBOL(cgu_get_cpu_clock);
EXPORT_SYMBOL(cgu_get_io_region_clock);
EXPORT_SYMBOL(cgu_get_fpi_bus_clock);
EXPORT_SYMBOL(cgu_get_pp32_clock);
EXPORT_SYMBOL(cgu_get_pci_clock);
EXPORT_SYMBOL(cgu_get_ethernet_clock);
EXPORT_SYMBOL(cgu_get_usb_clock);
EXPORT_SYMBOL(cgu_get_clockout);

module_init(danube_cgu_init);
module_exit(danube_cgu_exit);
