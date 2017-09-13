#ifndef _IFX_ADSL_AUTOBOOT_H
#define _IFX_ADSL_AUTOBOOT_H
/*
 * external variables from ifx_adsl_autoboot.c
 */
extern wait_queue_head_t	wait_queue_autoboot;
extern int			autoboot_shutdown;
extern struct completion	autoboot_thread_exit;
extern autoboot_line_states_t	autoboot_linestate;
extern autoboot_adsl_mode_t	autoboot_adsl_mode;
extern int			autoboot_showtime_lock;

#endif //_IFX_ADSL_AUTOBOOT_H
