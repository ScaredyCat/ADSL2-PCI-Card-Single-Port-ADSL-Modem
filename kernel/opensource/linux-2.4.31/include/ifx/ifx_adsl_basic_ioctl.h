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
 * ifx_adsl
 * 25-July-2005: Jin-Sze.Sow@infineon.com
 *
 *
 *
 */
#ifndef       	_IFX_ADSL_APP_H
#define        	_IFX_ADSL_APP_H

/** \defgroup IFX_ADSL_INTERFACE MEI Driver Interface
 * Lists the interface of the MEI Driver
 */

/** \defgroup IFX_ADSL_TYPES Type definitions for Infineon ADSL driver interface
 * \ingroup	IFX_ADSL_INTERFACE
 * @{
 */

/** This enumeration type contains the possible ADSL mode selections.
 */
typedef enum {
	/** Depending on firmware-image:
	    - Annex A
	      - ANSI T.1413,
	      - G.992.1 Annex A (ADSL)
	      - G.992.3 Annex A (ADSL2)
	      - G.992.3 Annex L (ADSL2)
	      - G.992.5 Annex A (ADSL2+)
	    - Annex B
	      - G.992.1 Annex B (ADSL)
	      - G.992.3 Annex B (ADSL2)
	      - G.992.5 Annex B (ADSL2+)
	*/
	AUTOBOOT_DSL_MODE_ALL,
	/** - G.992.1 Annex A (ADSL)
	*/
	AUTOBOOT_DSL_MODE_992_1A,
	/** - G.992.1 Annex B (ADSL)
	*/
	AUTOBOOT_DSL_MODE_992_1B,
	/** - G.992.3 Annex A (ADSL2)
	*/
	AUTOBOOT_DSL_MODE_992_3A,
	/** - G.992.3 Annex B (ADSL2)
	*/
	AUTOBOOT_DSL_MODE_992_3B,
	/** - G.992.3 Annex A (ADSL2)
	    - G.992.5 Annex A (ADSL2+)
	*/
	AUTOBOOT_DSL_MODE_992_5A,
	/** - G.992.3 Annex B (ADSL2)
	- G.992.5 Annex B (ADSL2+)
	*/
	AUTOBOOT_DSL_MODE_992_5B,
	/** - G.992.3 Annex I (ADSL2)
	*/
	AUTOBOOT_DSL_MODE_992_3I,
	/** - G.992.3 Annex J(ADSL2)
	*/
	AUTOBOOT_DSL_MODE_992_3J,
	/** - G.992.3 Annex M (ADSL2)
	*/
	AUTOBOOT_DSL_MODE_992_3M,
	/** - G.992.5 Annex I (ADSL2+)
	*/
	AUTOBOOT_DSL_MODE_992_5I,
	/** - G.992.5 Annex J (ADSL2+)
	*/
	AUTOBOOT_DSL_MODE_992_5J,
	/** - G.992.5 Annex M (ADSL2+)
	*/
	AUTOBOOT_DSL_MODE_992_5M,
	/** All Annex M modes:
	    - G.992.3 Annex M (ADSL2)
	    - G.992.5 Annex M (ADSL2+)
	*/
	AUTOBOOT_DSL_MODE_M_ALL,
	/** All Annex B modes:
	    - G.992.1 Annex B (ADSL)
	    - G.992.3 Annex B (ADSL2)
	    - G.992.5 Annex B (ADSL2+)
	*/
	AUTOBOOT_DSL_MODE_B_ALL,
	/** All Annex B & M modes:
	    - G.992.1 Annex B (ADSL)
	    - G.992.3 Annex B (ADSL2)
	    - G.992.3 Annex M (ADSL2)
	    - G.992.5 Annex B (ADSL2+)
	    - G.992.5 Annex M (ADSL2+)
	*/
	AUTOBOOT_DSL_MODE_M_B_ALL
} autoboot_adsl_mode_t;

/** Defines the maximum number of DSL modes */
#define AUTOBOOT_DSL_MODE_MAX (AUTOBOOT_DSL_MODE_M_B_ALL+1)

/** List of line states */
typedef enum {
	/** Init state. Line is in initialization phase. */
	AUTOBOOT_INIT,
	/** Training state. Line is in handshake/training phase. */
	AUTOBOOT_TRAIN,
	/** Showtime state. Line is in showtime, data trafic possible. */
	AUTOBOOT_SHOWTIME,
	/** Diagnostic state. Line is in diagnostic mode. */
	AUTOBOOT_DIAGNOSTIC,
	/** Restart state. Line is going to be restarted. (This is an intermediate state!) */
	AUTOBOOT_RESTART
} autoboot_line_states_t;


/** error code definitions */
typedef enum mei_error
{
	MEI_SUCCESS = 0,
	MEI_FAILURE = -1,
	MEI_MAILBOX_FULL = -2,
	MEI_MAILBOX_EMPTY = -3,
	MEI_MAILBOX_TIMEOUT = -4,
} MEI_ERROR;


typedef struct meireg{
	unsigned long iAddress;
	unsigned long iData;
}meireg;

#define MEIDEBUG_BUFFER_SIZES 512
typedef struct meidebug{
	unsigned long iAddress;
	unsigned long iCount;
	unsigned long buffer[MEIDEBUG_BUFFER_SIZES];
}meidebug;



/** @} */


/** \defgroup IFX_ADSL_IOCTL Ioctl definitions for Infineon ADSL driver
 * \ingroup	IFX_ADSL_INTERFACE
 * @{
 */


/**
 * Start (and restart) the ARC.
 *
 * \param int	The parameter is not used.
 *
 * \return 0 if successful, otherwise -1 in case of an error
 */
#define IFX_ADSL_IOC_MIB_BASE				0
#define IFX_ADSL_IOC_BASIC_BASE				3000
#define IFX_ADSL_IOC_CEOC_BASE				4000
#define IFX_ADSL_IOC_AUTOBOOT_BASE			5000
#define IFX_ADSL_IOC_END				10000

#define IFX_ADSL_IOC_START				( 0 + IFX_ADSL_IOC_BASIC_BASE)
#define IFX_ADSL_IOC_RUN				( 1 + IFX_ADSL_IOC_BASIC_BASE)
#define IFX_ADSL_IOC_RESET				( 3 + IFX_ADSL_IOC_BASIC_BASE)
#define IFX_ADSL_IOC_REBOOT				( 4 + IFX_ADSL_IOC_BASIC_BASE)
#define IFX_ADSL_IOC_HALT				( 5 + IFX_ADSL_IOC_BASIC_BASE)
#define IFX_ADSL_IOC_CMV_WINHOST			( 6 + IFX_ADSL_IOC_BASIC_BASE)
#define IFX_ADSL_IOC_CMV_READ				( 7 + IFX_ADSL_IOC_BASIC_BASE)
#define IFX_ADSL_IOC_CMV_WRITE				( 8 + IFX_ADSL_IOC_BASIC_BASE)

#define IFX_ADSL_IOC_GET_BASE_ADDRESS			( 9 + IFX_ADSL_IOC_BASIC_BASE)
/**
 * When the link is up and has entered showtime, call this command to inform driver that it is showtime now.
 *
 * \deprecated
 *	Not necessary anymore, as this is handled by the autoboot daemon in kernel!
 */
#define IFX_ADSL_IOC_SHOWTIME				(10 + IFX_ADSL_IOC_BASIC_BASE)
#define IFX_ADSL_IOC_REMOTE				(11 + IFX_ADSL_IOC_BASIC_BASE)
#define IFX_ADSL_IOC_READDEBUG				(12 + IFX_ADSL_IOC_BASIC_BASE)
#define IFX_ADSL_IOC_WRITEDEBUG				(13 + IFX_ADSL_IOC_BASIC_BASE)
#define IFX_ADSL_IOC_DOWNLOAD				(15 + IFX_ADSL_IOC_BASIC_BASE)
#define IFX_ADSL_IOC_JTAG_ENABLE			(16 + IFX_ADSL_IOC_BASIC_BASE)

/* Loop diagnostics mode of the ADSL line related constants */
/** @{ \name loop diagnostic mode
 */
#define IFX_ADSL_IOC_SET_LOOP_DIAGNOSTICS_MODE 		(19 + IFX_ADSL_IOC_BASIC_BASE)
#define IFX_ADSL_IOC_GET_LOOP_DIAGNOSTICS_MODE 		(20 + IFX_ADSL_IOC_BASIC_BASE)
/**
 * \deprecated
 *	Not necessary anymore, as this is handled by the autoboot daemon in kernel!
 */
#define IFX_ADSL_IOC_LOOP_DIAGNOSTIC_MODE_COMPLETE	(21 + IFX_ADSL_IOC_BASIC_BASE)
#define IFX_ADSL_IOC_IS_LOOP_DIAGNOSTICS_MODE_COMPLETE	(22 + IFX_ADSL_IOC_BASIC_BASE)
/** @} */

/* L3 Power Mode */
/* Get current Power Moaagement Mode Status*/
#define IFX_ADSL_IOC_GET_POWER_MANAGEMENT_MODE		(23 + IFX_ADSL_IOC_BASIC_BASE)
/* Set L3 Power Mode /disable L3 power mode */
#define IFX_ADSL_IOC_SET_L3_POWER_MODE			(24 + IFX_ADSL_IOC_BASIC_BASE)

/* get current dual latency configuration */
#define IFX_ADSL_IOC_GET_DUAL_LATENCY			(25 + IFX_ADSL_IOC_BASIC_BASE)
/* enable/disable dual latency path */
#define IFX_ADSL_IOC_SET_DUAL_LATENCY			(26 + IFX_ADSL_IOC_BASIC_BASE)

/* Enable/Disable autoboot mode. */
/* When the autoboot mode is disabled, the driver will excute some cmv
   commands for led control and dual latency when DSL startup.*/
#define AUTOBOOT_ENABLE_SET				(27+ IFX_ADSL_IOC_BASIC_BASE)

#define L0_POWER_MODE 0;
#define L2_POWER_MODE 2;
#define L3_POWER_MODE 3;

#define DUAL_LATENCY_US_DS_DISABLE			0
#define DUAL_LATENCY_US_ENABLE				(1<<0)
#define DUAL_LATENCY_DS_ENABLE				(1<<1)
#define DUAL_LATENCY_US_DS_ENABLE			(DUAL_LATENCY_US_ENABLE|DUAL_LATENCY_DS_ENABLE)


#define ME_HDLC_IDLE 0
#define ME_HDLC_INVALID_MSG 1
#define ME_HDLC_MSG_QUEUED 2
#define ME_HDLC_MSG_SENT 3
#define ME_HDLC_RESP_RCVD 4
#define ME_HDLC_RESP_TIMEOUT 5
#define ME_HDLC_RX_BUF_OVERFLOW 6
#define ME_HDLC_UNRESOLVED 1
#define ME_HDLC_RESOLVED 2

/** @{ \name kernel based autoboot daemon
 */

/**
 * Selecting the accepted ADSL modes in the firmware
 *
 * \deprecated
 *	interim solution to provide old autoboot interface, may change for final SW API
 *
 * \param autoboot_adsl_mode_t
 *	The parameter is a value of the type \ref autoboot_adsl_mode_t
 *
 * \retval 0 if successful
 * \retval -1 in case of an error
 *
 * \remarks
 *	- If the firmware does not support the selected mode, a fallback to "all" is done
 *	- This setting should be done before starting the firmware or another
 *	  restart of the firmware with \ref IFX_ADSL_IOC_START should  be triggered
 */
#define AUTOBOOT_ADSL_MODE_SET				(0 + IFX_ADSL_IOC_AUTOBOOT_BASE)

/**
 * Possibility for applications to read out the line state
 * This is done by mapping the firmware states to the ADSL standard.
 * \param autoboot_line_states_t*
 *		The parameter is a pointer to \ref autoboot_line_states_t value.
 *
 * \retval 0 if successful
 * \retval -1 in case of an error
 */
#define AUTOBOOT_LINE_STATE_GET				(1 + IFX_ADSL_IOC_AUTOBOOT_BASE)

/**
 * Give the possibility to enable/disable the autoboot task.
 *
 * \remarks This is mainly intended for debugging/testing purpose; therefore the
 * task is enabled by default.
 *
 * \param int	The parameter has the following values:
 *
 * \arg 0: disabled
 * \arg 1: enabled
 *
 * \retval 0 if successful
 * \retval -1 in case of an error
 */
#define AUTOBOOT_CONTROL_SET				(2 + IFX_ADSL_IOC_AUTOBOOT_BASE)

/**
 * Controls the showtime behavior of the autoboot task in showtime.
 * If this setting is enabled, the line stay in showtime, even if no data
 * traffic is posssible anymore because of LOS or other error signals.
 *
 * \remarks This is mainly intended for maesuring/testing purpose.
 *
 * \param int	The parameter has the following values:
 *
 * \arg 0: disabled
 * \arg 1: enabled
 *
 * \retval 0 if successful
 * \retval -1 in case of an error
 */
#define AUTOBOOT_SHOWTIME_LOCK_SET			(3 + IFX_ADSL_IOC_AUTOBOOT_BASE)

/** @} */

/** @} */

#define IFX_ADSL_IOC_CEOC_SEND				(0 + IFX_ADSL_IOC_CEOC_BASE)

#include <ifx/ifx_adsl_mib_ioctl_old.h>

#ifdef __KERNEL__
int IFX_ADSL_Ioctls(MEI_inode_t * ino, MEI_file_t * fil, unsigned int command, unsigned long lon);
#endif

#endif //_IFX_ADSL_APP_H
