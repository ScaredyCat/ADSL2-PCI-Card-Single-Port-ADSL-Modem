/*
 * ########################################################################
 *
 *  This program is free softwavre; you can distribute it and/or modify it
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
 * ifx_adsl_device - header files
 *
 */
/* Modification history */
/* */
/* 12-july-2005 Jin-Sze.Sow@infineon.com */


#ifndef		_IFX_ADSL_BASIC_H
#define        	_IFX_ADSL_BASIC_H
/////////////////////////////////////////////////////////////////////////////////////////////////////////////


// enable debug printouts
#define IFX_ADSL_DEBUG
// enable WINHOST debug ioctls
#define IFX_ADSL_CMV_EXTRA

// do autoboot handling inside driver
#define IFX_ADSL_INCLUDE_AUTOBOOT

#define IFX_ADSL_DEV_MAJOR	106

#define MSG_LENGTH		16     	// x16 bits
#define YES_REPLY      	 	1
#define NO_REPLY         	0


#define IFX_POP_EOC_DONE	0
#define IFX_POP_EOC_FAIL	-1

#define OMB_CLEAREOC_INTERRUPT_CODE	(0x00020000) 

#if defined(__KERNEL__) || defined (IFX_ADSL_PORT_RTEMS)

#ifdef __LINUX__
#ifdef CONFIG_PROC_FS
typedef struct reg_entry {
	int * flag;
	char name[30];          // big enough to hold names
	char description[100];      // big enough to hold description
	unsigned short low_ino;
} reg_entry_t;
#endif
#endif //__LINUX__
/*
 * external variables from ifx_adsl_basic.c
 */
extern MEI_mutex_t		mei_sema;
extern int			showtime;
extern u32			adsl_mode,
				adsl_mode_extend;
extern int			loop_diagnostics_mode;
extern int			loop_diagnostics_completed;
extern wait_queue_head_t	wait_queue_loop_diagnostic;



/*
**	Native size for the Stratiphy interface is 32-bits. All reads and writes
**	MUST be aligned on 32-bit boundaries. Trickery must be invoked to read word and/or
**	byte data. Read routines are provided. Write routines are probably a bad idea, as the
**	Arc has unrestrained, unseen access to the same memory, so a read-modify-write cycle
**	could very well have unintended results.
*/
extern MEI_ERROR meiCMV(u16 *, int, u16 *);                         // first arg is CMV to ARC, second to indicate whether need reply
extern void makeCMV(u8 opcode, u8 group, u16 address, u16 index, int size, u16 * data,u16 *CMVMSG);

extern int IFX_ADSL_ReadHdlc(char *hdlc_pkt,int max_hdlc_pkt_len);
extern int IFX_ADSL_SendHdlc(unsigned char *hdlc_pkt,int hdlc_pkt_len);

extern int IFX_ADSL_AdslStart(void);
extern int IFX_ADSL_AdslShowtime(void);
extern int IFX_ADSL_ReadAdslMode(void);
extern int IFX_ADSL_AdslReset(void);

extern ifx_adsl_device_t *IFX_ADSL_GetAdslDevice(void);
extern int IFX_ADSL_SendCmv(u8 opcode, u8 group, u16 address, u16 index, int size, u16 * data, u16 *CMVMSG);
extern int IFX_ADSL_IsModemReady(ifx_adsl_device_t *pDev);

extern int ifx_adsl_autoboot_thread_start(void);
extern int ifx_adsl_autoboot_thread_restart(void);
extern int ifx_adsl_autoboot_thread_stop(void);

extern int mei_mib_ioctl(MEI_inode_t * ino, MEI_file_t * fil, unsigned int command, unsigned long lon);
extern int mei_mib_adsl_link_up(void);
extern int mei_mib_adsl_link_down(void);
extern int ifx_adsl_mib_init(void);
extern void ifx_adsl_mib_cleanup(void);

#endif

#endif //_IFX_ADSL_BASIC_H

