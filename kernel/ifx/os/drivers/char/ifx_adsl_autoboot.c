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
 *
 */
/* Change log:
*/

/*
 * ===========================================================================
 *                           INCLUDE FILES
 * ===========================================================================
 */

/*#define IFX_ADSL_PORT_RTEMS 1*/

#if defined(IFX_ADSL_PORT_RTEMS)
#include "ifx_asdl_rtems.h"
#else
#ifndef EXPORT_SYMTAB
#define EXPORT_SYMTAB
#endif

#include <ifx/ifx_adsl_cmvs.h>
#include <ifx/ifx_adsl_linux.h>

#endif /* !defined(IFX_ADSL_PORT_RTEMS) */

#ifdef IFX_ADSL_DEBUG
#define IFX_ADSL_DMSG(fmt, args...) printk( KERN_INFO  "%s: " fmt,__FUNCTION__, ## args)
#else
#define IFX_ADSL_DMSG(fmt, args...) do { } while(0)
#endif /* ifdef IFX_ADSL_DEBUG */

#define IFX_ADSL_EMSG(fmt, args...) printk( KERN_ERR  "%s: " fmt,__FUNCTION__, ## args)


#ifdef IFX_ADSL_INCLUDE_AUTOBOOT

/* to be enabled if the firmware does not need the restarts anymore */
#define IFX_ADSL_NO_FW_RESTART	0

#define DSL_FW_ANNEX_A 0x1
#define DSL_FW_ANNEX_B 0x2

/************************************************************************
 *  variable declaration
 ************************************************************************/

wait_queue_head_t wait_queue_autoboot;
int autoboot_shutdown = 0;
int autoboot_running = 0;
struct completion autoboot_thread_exit;
autoboot_line_states_t autoboot_linestate = AUTOBOOT_INIT;
int autoboot_showtime_lock = 0;
autoboot_adsl_mode_t autoboot_adsl_mode = AUTOBOOT_DSL_MODE_ALL;

/************************************************************************
 *  local variables
 ************************************************************************/

/**
   List of supported ADSL transmission modes according to ITU-T standards.
*/
typedef enum
{
   DSL_ADSLMODE_UNKNOWN = 0,
   DSL_ADSLMODE_G_992_1,
   DSL_ADSLMODE_T1_413,
   DSL_ADSLMODE_G_992_3,
   DSL_ADSLMODE_G_992_5,
   DSL_ADSLMODE_LAST
} DSL_AdslMode_t;

/**
   Defines all possible annex types
*/
typedef enum
{
   DSL_ANNEX_UNKNOWN = 0,
   DSL_ANNEX_A,
   DSL_ANNEX_B,
   DSL_ANNEX_I,
   DSL_ANNEX_J,
   DSL_ANNEX_L,
   DSL_ANNEX_M,
   DSL_ANNEX_LAST
} DSL_Annex_t;

static const char *const group_name[] =
{
	"??", "CNTL", "STAT", "INFO", "TEST", "OPTN", "RATE", "PLAM", "CNFG"
};

struct mode_info
{
	u16 optn0;
	u16 optn7;
	u16 annex_support;
};

static const struct mode_info optn_mode[AUTOBOOT_DSL_MODE_MAX] =
{
   /*             optn 0  optn 7  FW Annex support*/
   /* ALL */     {0x0000, 0x0000, DSL_FW_ANNEX_A | DSL_FW_ANNEX_B},
   /* 992_1A */  {0x0004, 0x0000, DSL_FW_ANNEX_A},
   /* 992_1B */  {0x0008, 0x0000, DSL_FW_ANNEX_B},
   /* 992_3A */  {0x0100, 0x0000, DSL_FW_ANNEX_A},
   /* 992_3B */  {0x0200, 0x0000, DSL_FW_ANNEX_B},
   /* 992_5A */  {0x8000, 0x0000, DSL_FW_ANNEX_A},
   /* 992_5B */  {0x4000, 0x0000, DSL_FW_ANNEX_B},
   /* 992_3I */  {0x0400, 0x0000, DSL_FW_ANNEX_A},
   /* 992_3J */  {0x0800, 0x0000, DSL_FW_ANNEX_B},
   /* 992_3M */  {0x2000, 0x0000, DSL_FW_ANNEX_A | DSL_FW_ANNEX_B},
   /* 992_5I */  {0x0000, 0x0001, DSL_FW_ANNEX_A},
   /* 992_5J */  {0x0000, 0x0002, DSL_FW_ANNEX_B},
   /* 992_5M */  {0x0000, 0x0004, DSL_FW_ANNEX_A | DSL_FW_ANNEX_B},
   /* M_ALL */   {0x2000, 0x0004, DSL_FW_ANNEX_A | DSL_FW_ANNEX_B},
   /* B_ALL */   {0x4208, 0x0000, DSL_FW_ANNEX_B},
   /* M_B_ALL */ {0x6208, 0x0000, DSL_FW_ANNEX_A | DSL_FW_ANNEX_B}
};

typedef struct
{
   u16 nStat1;
   u16 nStat17;
   DSL_AdslMode_t eAdslMode;
   DSL_Annex_t eAnnex;
} MEI_AdslStatusEntry_t;

static const MEI_AdslStatusEntry_t g_adslStatusTab[] =
{
   /* STAT1  STAT17  ADSL mode             AnnexType */
   { 0x0001, 0x0000, DSL_ADSLMODE_T1_413,  DSL_ANNEX_A },
   { 0x0004, 0x0000, DSL_ADSLMODE_G_992_1, DSL_ANNEX_A },
   { 0x0008, 0x0000, DSL_ADSLMODE_G_992_1, DSL_ANNEX_B },
   { 0x0100, 0x0000, DSL_ADSLMODE_G_992_3, DSL_ANNEX_A },
   { 0x1100, 0x0000, DSL_ADSLMODE_G_992_3, DSL_ANNEX_L },
   { 0x0200, 0x0000, DSL_ADSLMODE_G_992_3, DSL_ANNEX_B },
   { 0x8000, 0x0000, DSL_ADSLMODE_G_992_5, DSL_ANNEX_A },
   { 0x4000, 0x0000, DSL_ADSLMODE_G_992_5, DSL_ANNEX_B },
   { 0x0400, 0x0000, DSL_ADSLMODE_G_992_3, DSL_ANNEX_I },
   { 0x0800, 0x0000, DSL_ADSLMODE_G_992_3, DSL_ANNEX_J },
   { 0x2000, 0x0000, DSL_ADSLMODE_G_992_3, DSL_ANNEX_M },
   { 0x0000, 0x0001, DSL_ADSLMODE_G_992_5, DSL_ANNEX_I },
   { 0x0000, 0x0002, DSL_ADSLMODE_G_992_5, DSL_ANNEX_J },
   { 0x0000, 0x0004, DSL_ADSLMODE_G_992_5, DSL_ANNEX_M },
   { 0x0000, 0x0000, DSL_ADSLMODE_UNKNOWN, DSL_ANNEX_UNKNOWN },
};


typedef struct
{
   DSL_AdslMode_t eAdslMode;
   DSL_Annex_t eAnnexType;
} MEI_ActAdslStatus_t;

static MEI_ActAdslStatus_t g_actAdslStatus = { 0, 0 };
static int autoboot_polltime = 30;      /* value in sec */
static u16 autoboot_modemstate = 0;
static struct timeval autoboot_starttime = {0,0};
static int autoboot_got_farend_resp=0;
static int autoboot_timout_limit=0;
/**
   Stores the min. SNR marging for downstream given by CO during handshake
   The value is stored in units of 0.1 dB */
static s16 g_nMinSnrmDs = 0;

#if DANUBE_NO_FW_RESTART == 0
/* variables for restoring some counters after firmware reboot */
static u16 first_power_on=1;
static u16 nErrS_L_count[4] = { 0,0,0,0 };
#endif

/************************************************************************
 *  Function declaration
 ************************************************************************/

static MEI_ERROR meiCMVread(u8 group, u16 address, u16 index, int size, u16 * data);
static MEI_ERROR meiCMVwrite(u8 group, u16 address, u16 index, int size, u16 * data);

static MEI_ERROR MEI_ShowtimeStatusUpdate(DSL_boolean_t bReset);
static void ifx_adsl_autoboot_handle_start(void);
static void ifx_adsl_autoboot_handle_restart(void);
static void ifx_adsl_autoboot_handle_training(void);
static void ifx_adsl_autoboot_handle_showtime(void);
static void ifx_adsl_autoboot_statemachine(void);
static int ifx_adsl_autoboot_thread(void *unused);

/************************************************************************
 *  Function implementation
 ************************************************************************/


static MEI_ERROR meiCMVread (
   u8 group,
   u16 address,
   u16 index,
   int size,
   u16 * data)
{
	u16 RxMessage[MSG_LENGTH]__attribute__ ((aligned(4)));
	u16 TxMessage[MSG_LENGTH]__attribute__ ((aligned(4)));
	MEI_ERROR ret = MEI_SUCCESS;
	int i;

	makeCMV(H2D_CMV_READ, group, address, index, size, NULL, TxMessage);
	ret = meiCMV(TxMessage, YES_REPLY, RxMessage);
	if (ret != MEI_SUCCESS)
	{
	      IFX_ADSL_EMSG ("CMV fail, read %s %d %d\n", group_name[group], address,
	         index);
	}
	else
	{
		for (i=0; i<size; i++)
		{
			*(data+i) = RxMessage[i+4];
		}
	}
	return ret;
}

static MEI_ERROR meiCMVwrite (
   u8 group,
   u16 address,
   u16 index,
   int size,
   u16 * data)
{
	u16 RxMessage[MSG_LENGTH]__attribute__ ((aligned(4)));
	u16 TxMessage[MSG_LENGTH]__attribute__ ((aligned(4)));
	MEI_ERROR ret = MEI_SUCCESS;

	makeCMV(H2D_CMV_WRITE, group, address, index, size, data, TxMessage);
	ret = meiCMV(TxMessage, YES_REPLY, RxMessage);
   if (ret != MEI_SUCCESS)
   {
      IFX_ADSL_EMSG ("CMV fail, write %s %d %d\n", group_name[group], address,
         index);
	}

	return ret;
}

/**
   Reads the current ADSL mode status and stores it within global context.

   \param bReset  Updates global context for ADSL mode as follows, [I]
                  DSL_TRUE : Reset the status
                  DSL_FALSE: Read and store current status

   \return
   Return values are defined within the MEI_ERROR definition
   - MEI_SUCCESS (0)
   - MEI_FAILURE (-1)
   - or any other defined value
*/
static MEI_ERROR MEI_ShowtimeStatusUpdate(
   DSL_boolean_t bReset)
{
   u16 nStat1 = 0, nStat17 = 0;
   u16 nMinSnrmDs = 0;
   s16 nVal = 0;
   MEI_AdslStatusEntry_t *pTab = &g_adslStatusTab[0];

   memset (&g_actAdslStatus, 0, sizeof(g_actAdslStatus));

   if (bReset == DSL_FALSE)
   {
      if (meiCMVread(STAT, STAT_Mode, 0, 1, &nStat1) < MEI_SUCCESS) return MEI_FAILURE;
      if (meiCMVread(STAT, STAT_Mode1, 0, 1, &nStat17) < MEI_SUCCESS) return MEI_FAILURE;

      for ( ; (pTab->nStat1 != 0) || (pTab->nStat17 != 0); pTab++)
      {
         if ( (pTab->nStat1 == nStat1) && (pTab->nStat17 == nStat17) )
         {
            g_actAdslStatus.eAdslMode = pTab->eAdslMode;
            g_actAdslStatus.eAnnexType = pTab->eAnnex;
            break;
         }
      }

#ifdef IFX_ADSL_DEBUG
      {
         char sAdslMode[3] = "", sAnnex[2] = "";

         switch (g_actAdslStatus.eAdslMode)
         {
         case DSL_ADSLMODE_G_992_1: sprintf(sAdslMode,"1"); break;
         case DSL_ADSLMODE_T1_413: sprintf(sAdslMode,"T1"); break;
         case DSL_ADSLMODE_G_992_3: sprintf(sAdslMode,"2"); break;
         case DSL_ADSLMODE_G_992_5: sprintf(sAdslMode,"2+"); break;
         default: sprintf(sAdslMode,"?"); break;
         }

         switch (g_actAdslStatus.eAnnexType)
         {
         case DSL_ANNEX_A: sprintf(sAnnex,"A"); break;
         case DSL_ANNEX_B: sprintf(sAnnex,"B"); break;
         case DSL_ANNEX_I: sprintf(sAnnex,"I"); break;
         case DSL_ANNEX_J: sprintf(sAnnex,"J"); break;
         case DSL_ANNEX_M: sprintf(sAnnex,"M"); break;
         case DSL_ANNEX_L: sprintf(sAnnex,"L"); break;
         default: sprintf(sAnnex,"?"); break;
         }

         IFX_ADSL_DMSG("ADSL(%s), Annex(%s)\n", sAdslMode, sAnnex);
      }
#endif /*IFX_ADSL_DEBUG*/

      /* Read MINSNRMds which has been given by CO within handshake */
      switch (g_actAdslStatus.eAdslMode)
      {
      case DSL_ADSLMODE_G_992_1:
      case DSL_ADSLMODE_T1_413:
         if (meiCMVread(INFO, INFO_RCMsgRA, 1, 1, &nMinSnrmDs) >= MEI_SUCCESS)
         {
            /* In case of DMT the value is defined as integer */
            nVal = ((s16)nMinSnrmDs * 10);
         }
         g_nMinSnrmDs = nVal;
         break;
      case DSL_ADSLMODE_G_992_3:
      case DSL_ADSLMODE_G_992_5:
         if (meiCMVread(INFO, INFO_RCMsgs1, 1, 1, &nMinSnrmDs) >= MEI_SUCCESS)
         {
            /* In case of ADSL2/2+ the value is already defined as multiple
               of 0.1 dB */
            nVal = (s16)nMinSnrmDs;
         }
         /* Handling has to be done for DMT only -> set to max. possible value */
         g_nMinSnrmDs = 0;
         break;
      default:
         /* Handling has to be done for DMT only -> set to max. possible value */
         g_nMinSnrmDs = 0;
         break;
      }
      if (g_nMinSnrmDs < 0) g_nMinSnrmDs = 0;
      IFX_ADSL_DMSG ("MINSNRMds=%d/10dB\n", nVal);
   }

   return MEI_SUCCESS;
}

static int ifx_adsl_firmware_startup_init(void)
{	
#if defined(CONFIG_IFX_ADSL_CEOC)
	IFX_ADSL_CEOC_Init();
#endif
#ifdef CONFIG_IFX_ADSL_LED
	IFX_ADSL_LED_Init();
#endif
#ifdef IFX_ADSL_DUAL_LATENCY_SUPPORT
	IFX_ADSL_DualLatency_Init();
#endif
	return 0;	
}

int ifx_adsl_autoboot_ioctl(MEI_inode_t * ino, MEI_file_t * fil, unsigned int command, unsigned long lon)
{
	int meierr=MEI_SUCCESS;
	switch(command)
	{
		case AUTOBOOT_ADSL_MODE_SET:
			if (lon < AUTOBOOT_DSL_MODE_MAX)
				autoboot_adsl_mode = lon;
			else
				meierr = -EINVAL;
			break;
		case AUTOBOOT_LINE_STATE_GET:
			if ((void *)lon != NULL) {
				if (copy_to_user((void *)lon, (int*)&autoboot_linestate, sizeof(int)))
					return -EFAULT;
			}
			else
				meierr = (int)autoboot_linestate;
			break;
		case AUTOBOOT_CONTROL_SET:
			if (lon==0)
				meierr = ifx_adsl_autoboot_thread_stop();
			else
			{
				meierr = ifx_adsl_autoboot_thread_start();
			}
			break;
		case AUTOBOOT_SHOWTIME_LOCK_SET:
			autoboot_showtime_lock = (lon!=0);
			break;
	}
	return meierr;
}

/**
 * Write down the initial configuration to the firmware and activate link
 *
 * \ingroup Internal
 */
static void ifx_adsl_autoboot_handle_start(
	void)
{
	u16 val, annex = 0;
	u16 optn0, optn7;

	ifx_adsl_firmware_startup_init();
	
	if (MEI_MUTEX_LOCK(mei_sema)) 
	{
		IFX_ADSL_EMSG("\ngetting mei_sema failed!");
		return;
	}

	if (meiCMVread(INFO, INFO_Version, 1, 1, &val) != MEI_SUCCESS) 
	{
		goto exit;
	} 
	else 
	{
		annex = (val>>8) & 0x3f;
		switch (annex) 
		{
		case DSL_FW_ANNEX_A:
		case DSL_FW_ANNEX_B:
			if ((autoboot_adsl_mode==AUTOBOOT_DSL_MODE_ALL) ||
				(optn_mode[autoboot_adsl_mode].annex_support & annex)==0) 
			{
            			if (annex == DSL_FW_ANNEX_A)
            			{
               				/* ANNEX A */
					optn0 = 0x9105;
            			}
					else
			        {
			               /* ANNEX B */
					optn0 = 0x4208;
            			}
					optn7 = 0x0000;
		         }
		         else
		         {
					optn0 = optn_mode[autoboot_adsl_mode].optn0;
					optn7 = optn_mode[autoboot_adsl_mode].optn7;
			}
			break;
			default:
				IFX_ADSL_EMSG("Unknown Annex of Firmware (0x%X)!!!\n", annex);
				autoboot_polltime = 30;
				autoboot_linestate = AUTOBOOT_RESTART;
				goto exit;
		}
	}

	meiCMVwrite(OPTN, OPTN_ModeControl, 0, 1, &optn0 );
	meiCMVwrite(OPTN, OPTN_ModeControl1, 0, 1, &optn7);

	if (loop_diagnostics_mode) 
	{
		u16 optn9;
		IFX_ADSL_DMSG("Enable diag mode in Firmware\n");
		if (meiCMVread(OPTN, OPTN_StateMachineCtrl, 0, 1, &optn9) == MEI_SUCCESS) 
		{
			optn9 |= 1<<2;
			meiCMVwrite(OPTN, OPTN_StateMachineCtrl, 0, 1, &optn9);
		}
	}

	/* restore counters */
	if (!first_power_on) 
	{
		int i;
		for (i=0; i<4; i++) 
		{
			meiCMVwrite(PLAM, 6+i, 0, 1, &nErrS_L_count[i]);
		}
	}
	else
	{
		first_power_on = 0;
	}

	autoboot_got_farend_resp = 0;
	do_gettimeofday(&autoboot_starttime);
	autoboot_timout_limit = 60;

	val = CNTL_ModemStart;
	meiCMVwrite(CNTL, CNTL_ModemControl, 0, 1, &val);

	autoboot_linestate = AUTOBOOT_TRAIN;
   /* set poll timeout to 1 sec */
	autoboot_polltime = 1;

exit:
	MEI_MUTEX_UNLOCK(mei_sema);
}

static void ifx_adsl_autoboot_handle_restart(
    void)
{
	u16 val=0;
	int i;

	if (MEI_MUTEX_LOCK(mei_sema)) 
        {
		IFX_ADSL_EMSG("\ngetting mei_sema failed!");
		return;
	}

#if IFX_ADSL_NO_FW_RESTART == 1
	val = CNTL_ModemReset;
	IFX_ADSL_DMSG("CNTL_ModemReset\n");
	meiCMVwrite(CNTL, CNTL_ModemControl, 0, 1, &val);
	autoboot_linestate = AUTOBOOT_INIT;
#else

	/* save the current counters */
	for (i=0; i<4; i++) 
        {
	 	if (meiCMVread(PLAM, 6+i, 0, 1, &val) == MEI_SUCCESS) 
	 	{
			nErrS_L_count[i] = val;
		}
	}

#endif

	MEI_MUTEX_UNLOCK(mei_sema);

#if IFX_ADSL_NO_FW_RESTART == 0
	IFX_ADSL_AdslReset();
	IFX_ADSL_AdslStart();
#endif
}

/**
 * Handle the training phase and restart on timeouts
 *
 * \ingroup Internal
 */
static void ifx_adsl_autoboot_handle_training(
    void)
{
	u16 val=0;
	u16 prev_modemstate = autoboot_modemstate;

	if (MEI_MUTEX_LOCK(mei_sema)) 
        {
		IFX_ADSL_EMSG("\ngetting mei_sema failed!");
		return;
	}

	if (meiCMVread(STAT, STAT_MacroState, 0, 1, 
		&autoboot_modemstate) == MEI_SUCCESS) 
	{
		if (prev_modemstate != autoboot_modemstate) 
		{
			IFX_ADSL_DMSG("modem state: %d -> %d (%ld)\n",prev_modemstate,
				 autoboot_modemstate, jiffies);
			switch (autoboot_modemstate) 
			{
				case STAT_ShowTimeState:
					/*if (autoboot_showtime_lock==0)  IFX_ADSL_DMSG("Showtime 
					  reached, but not reported!!! DEBUG!!!\n"); else*/
					{
						if (meiCMVread(STAT, STAT_Misc, 0, 1, &val) == MEI_SUCCESS) 
						{
							/* wait for "codeswaps complete" */
							if (val & 0x02) 
							{
								autoboot_linestate = AUTOBOOT_SHOWTIME;
								MEI_ShowtimeStatusUpdate(DSL_FALSE);
								IFX_ADSL_AdslShowtime();
							}
						}
					}
					break;
				case STAT_FailState:
            /* case STAT_IdleState: */
					autoboot_linestate = AUTOBOOT_RESTART;
					if (autoboot_modemstate == STAT_IdleState) 
					{
					/* set poll timeout to 5 sec */
						autoboot_polltime = 5;
					}
					break;
				case STAT_GhsState:
				case STAT_FullInitState:
					if (autoboot_got_farend_resp==0) 
					{
						autoboot_got_farend_resp = 1;
						do_gettimeofday(&autoboot_starttime);
						autoboot_timout_limit = 60;
					}
					break;
				case STAT_LoopDiagMode:
					autoboot_linestate = AUTOBOOT_DIAGNOSTIC;
					loop_diagnostics_completed = 0;
					autoboot_timout_limit = 120;
					if (autoboot_got_farend_resp==0) 
					{
						autoboot_got_farend_resp = 1;
						do_gettimeofday(&autoboot_starttime);
					}
					break;
				case STAT_IdleState:
					if (prev_modemstate == STAT_LoopDiagMode) 
					{
						loop_diagnostics_completed = 1;
						MEI_WAKEUP_EVENT(wait_queue_loop_diagnostic);
					}
					IFX_ADSL_ReadAdslMode();
					autoboot_linestate = AUTOBOOT_RESTART;
					/* set poll timeout to 5 sec */
					autoboot_polltime = 5;
					break;
			}
		}
	}
	if ((autoboot_linestate == AUTOBOOT_TRAIN) ||
	    (autoboot_linestate == AUTOBOOT_DIAGNOSTIC)) 
	{
		struct timeval current_time;
		do_gettimeofday(&current_time);
		if ((autoboot_starttime.tv_sec+autoboot_timout_limit) < current_time.tv_sec ) 
		{
			IFX_ADSL_DMSG("Reboot firmware on timeout (%d)!!!\n", 
			    autoboot_timout_limit);
			autoboot_linestate = AUTOBOOT_RESTART;
		}
	}

	MEI_MUTEX_UNLOCK(mei_sema);
}


/**
 * Check for conditions to leave showtime
 *
 * \ingroup Internal
 */
static void ifx_adsl_autoboot_handle_showtime(
	void)
{
	u16 modemstate, val=0;
   s16 nActSnrmDs = 0;

	if (MEI_MUTEX_LOCK(mei_sema)) 
	{
		IFX_ADSL_EMSG("\ngetting mei_sema failed!");
		return;
	}

	if (meiCMVread(STAT, STAT_MacroState, 0, 1, &modemstate) != MEI_SUCCESS) 
	{
		goto exit;
	}

	if (autoboot_modemstate != modemstate) 
	{
		IFX_ADSL_DMSG("modem state: %d -> %d (%ld)\n", autoboot_modemstate,
		  modemstate, jiffies);
		autoboot_modemstate = modemstate;
	}
		switch (autoboot_modemstate) 
		{
			case STAT_ShowTimeState:
			case STAT_IdleState:
				if (meiCMVread(PLAM, 0, 0, 1, &val) == MEI_SUCCESS) 
				{
#ifdef CONFIG_IFX_ADSL_MIB
					mei_mib_ioctl(0,0,IFX_ADSL_MIB_LO_ATUC,val);
#endif /* CONFIG_IFX_ADSL_MIB */
					/* NE LOS */
					if (val&0x1)
					{
						if (autoboot_showtime_lock==0) 
						{
							IFX_ADSL_DMSG("NE LOS, set reboot flag!\n");
							autoboot_linestate = AUTOBOOT_RESTART;
						} 
						else 
						{
							IFX_ADSL_DMSG("NE LOS, but showtime_lock!!!\n");
						}
					}
				}
#ifdef CONFIG_IFX_ADSL_MIB
				/* this value is only required if the MIB is enabled */
				if (meiCMVread(PLAM, 1, 0, 1, &val) == MEI_SUCCESS) 
				{
					mei_mib_ioctl(0,0,IFX_ADSL_MIB_LO_ATUR,val);
				}
#endif /* CONFIG_IFX_ADSL_MIB */

				/* Read current (showtime) value for SNRMds */
				if (meiCMVread(PLAM, 46, 0, 1, &val) == MEI_SUCCESS) 
				{
         /* SNRMds is defined in units of 0.5dB but has to be compared with
            global MINSNRM value in units of 0.1dB */
         nActSnrmDs = (((s16)val) * 10) / 2;
         if (nActSnrmDs < g_nMinSnrmDs)
         {
            if (autoboot_showtime_lock == 0)
            {
               IFX_ADSL_DMSG ("SNR Margin DS (%d/10dB) < MINSNRM (%d/10dB), "
                  "set reboot flag!\n", nActSnrmDs, g_nMinSnrmDs);
							autoboot_linestate = AUTOBOOT_RESTART;
            }
            else
            {
               IFX_ADSL_DMSG ("SNR Margin DS (%d/10dB) < MINSNRM (%d/10dB), "
                  "but showtime_lock!!!\n", nActSnrmDs, g_nMinSnrmDs);
						}
					}
				}
				break;
			default:
				IFX_ADSL_DMSG("set reboot flag!\n");
				autoboot_linestate = AUTOBOOT_RESTART;
				break;
		}

exit:
	MEI_MUTEX_UNLOCK(mei_sema);
}


/**
 * Execute the statemachine for autoboot handling
 *
 * \ingroup Internal
 */
static void ifx_adsl_autoboot_statemachine(
    void)
{
	autoboot_line_states_t prev_autoboot_state = autoboot_linestate;
	ifx_adsl_device_t *adsl_dev;
	
	adsl_dev = IFX_ADSL_GetAdslDevice();

	switch (autoboot_linestate) 
	{
		case AUTOBOOT_INIT:
			if (IFX_ADSL_IsModemReady(adsl_dev)==1) 
			{
				ifx_adsl_autoboot_handle_start();
			}
			break;
		case AUTOBOOT_DIAGNOSTIC:
			/*IFX_ADSL_DMSG("autoboot diagnostic\n"); */
		case AUTOBOOT_TRAIN:
			ifx_adsl_autoboot_handle_training();
			break;
		case AUTOBOOT_SHOWTIME:
			ifx_adsl_autoboot_handle_showtime();
			break;
		case AUTOBOOT_RESTART:
			autoboot_polltime = 1;
			ifx_adsl_autoboot_handle_restart();
			break;
		default:
			IFX_ADSL_DMSG("autoboot ???\n");
			autoboot_polltime = 30;
			break;
	}

	if (prev_autoboot_state != autoboot_linestate)
	{
		IFX_ADSL_DMSG("autoboot state: %d -> %d (%ld)\n",
			(int)prev_autoboot_state, (int)autoboot_linestate, jiffies);
	}
}


/**
 * Thread for the autoboot statemachine
 *
 * \ingroup Internal
 */
static int ifx_adsl_autoboot_thread(
	void *unused)
{
	daemonize();
	reparent_to_init();
	sigfillset(&current->blocked);

	snprintf(current->comm, sizeof(current->comm)-1, "%s", "adsl_autobootd");

	autoboot_running = 1;
	while(autoboot_shutdown==0) 
	{
		interruptible_sleep_on_timeout(&wait_queue_autoboot, 
			HZ*autoboot_polltime);
		if (autoboot_shutdown)
			break;
		ifx_adsl_autoboot_statemachine();
		/* IFX_ADSL_DMSG("autoboot running (%lu)\n", jiffies); */
	}
	IFX_ADSL_DMSG("autoboot ending (%lu)\n", jiffies);
	complete_and_exit(&autoboot_thread_exit,0);
	IFX_ADSL_DMSG("autoboot complete (%lu)\n", jiffies);
	return 0;
}


/************************************************************************
 * Global functions
 ************************************************************************/

/**
 * Initialize and start the autoboot thread
 *
 * \ingroup Internal
 */
int ifx_adsl_autoboot_thread_start(
	void)
{
	if (autoboot_running == 0) 
	{
		IFX_ADSL_DMSG("Starting adsl_autobootd...\n");
		autoboot_shutdown = 0;
		
		ifx_adsl_autoboot_thread_restart();
		kernel_thread(ifx_adsl_autoboot_thread, NULL, 
			CLONE_FS | CLONE_FILES | CLONE_SIGNAL);
		return 0;
	} 
	else 
	{
		IFX_ADSL_EMSG("autoboot already running\n");
		return -EBUSY; /* Device or resource busy */
	}
}


/**
 * Restart the autoboot thread
 *
 * \ingroup Internal
 */
int ifx_adsl_autoboot_thread_restart(
	void)
{
	if (autoboot_running == 0)
		return -ESRCH; /* No such process */

	IFX_ADSL_DMSG("Restarting adsl_autobootd...\n");
	MEI_ShowtimeStatusUpdate(DSL_TRUE);
	autoboot_linestate = AUTOBOOT_INIT;
	autoboot_polltime = 1;
	autoboot_modemstate = 0;
	wake_up_interruptible(&wait_queue_autoboot);
	return 0;
}


/**
 * Stop the autoboot thread
 *
 * \ingroup Internal
 */
int ifx_adsl_autoboot_thread_stop(
	void)
{
	if (autoboot_running == 0)
		return 0;

	IFX_ADSL_DMSG("Stopping adsl_autobootd...\n");
	autoboot_shutdown = 1;
	wake_up_interruptible(&wait_queue_autoboot);
	wait_for_completion(&autoboot_thread_exit);
	autoboot_running = 0;
	return 0;
}

#endif /* IFX_ADSL_INCLUDE_AUTOBOOT */

