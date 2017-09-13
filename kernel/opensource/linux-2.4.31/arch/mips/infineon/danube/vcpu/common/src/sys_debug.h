#ifndef _sys_debug_h
#define _sys_debug_h
/****************************************************************************
       Copyright (c) 2000, Infineon Technologies.  All rights reserved.

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
   Module      : $RCSfile: sys_debug.h,v $
   Date        : $Date: 2005/08/30 10:43:41 $
   Description : system debug interface

   $Log: sys_debug.h,v $
   Revision 1.1  2005/08/30 10:43:41  pliu
   taken from INCA_IP II
   - change names
   - IM5_11 --> IM4_18

   Revision 1.1  2004/03/30 10:59:32  thomas
   Working version of common ...

   Revision 1.22  2003/12/19 13:52:43  tauche
   VXWORKS PRINTK function using logMsg with variable number of arguments (printf compatible)
   temporary OS-less section added

   Revision 1.21  2003/12/19 10:22:32  rah
   Added windows types and system calls for simulation
   customer module as template for new OS's, still not finished -> serves as discussion basis for moving to OS_... calls

   Revision 1.16  2003/06/27 13:01:18  rutkowski
   - Change the formation of the text to be easier to read.


 ***************************************************************************/

/* ============================= */
/* Global Macro Definitions    */
/* ============================= */

/* ============================= */
/* Group=VOS - debug interface   */
/* ============================= */

#define DBG_LEVEL_OFF 		4
#define DBG_LEVEL_HIGH		3
#define DBG_LEVEL_NORMAL	2
#define DBG_LEVEL_LOW 		1


#ifdef VXWORKS
/* enable logMsg with variable number of arguments */
#ifndef __PROTOTYPE_5_0
   #define __PROTOTYPE_5_0
   #include "loglib.h"
#endif
   #define PRINTF	printf
   #define PRINTK logMsg
   #define DRV_LOG(x) logMsg("%s", (int)x, 0, 0, 0, 0, 0)
#endif /* VXWORKS */

#ifdef LINUX

/*
to see the debug output on console
use following line:
   echo 8 > /proc/sys/kernel/printk
*/
#ifdef __KERNEL__
   #define __TRACE(fmt, args...) printk( KERN_DEBUG fmt, ##args); printk("\r")
   #define PRINTF __TRACE
   #define DRV_LOG(x) printk( "%s", x)
#else
   #define PRINTF printf
   #define DRV_LOG(x) printf( "%s", x)
#endif /* __KERNEL__ */
#endif /* LINUX */

#ifdef OSE
   #ifdef USE_DEBUG_PRINTF
   #include "dbgprintf.h"
   #define PRINTF dbgprintf
   #define DRV_LOG(x) dbgprintf( "%s", x)
   #else
   #define PRINTF printf
   #define DRV_LOG(x) printf( "%s", x)
   #endif/* USE_DEBUG_PRINTF*/
#endif /* OSE */

#ifdef PSOS
   #define PRINTF	printf
   #define DRV_LOG(x)
#endif /* PSOS */

#ifdef NO_OS
   #define PRINTF	printf
   #define PRINTK printf
   #define DRV_LOG(x) logMsg("%s", (int)x, 0, 0, 0, 0, 0)
#endif /* VXWORKS */

#ifdef WIN32
	#ifdef TRACE
		#undef TRACE
	#endif /* TRACE */
    #define PRINTF print
#endif /* WIN32 */

#ifdef ENABLE_TRACE
/****************************************************************************
Description:
   Prototype for a trace group.
Arguments:
   name - Name of the trace group
Return Value:
   None.
Remarks:
   None.
Example:
   DECLARE_TRACE_GROUP(LLC) - declares a logical trace group named LLC.
****************************************************************************/
#define DECLARE_TRACE_GROUP(name) extern u32 G_nTraceGroup##name;


/****************************************************************************
Description:
   Create a trace group.
Arguments:
   name - Name of the trace group
Return Value:
   None.
Remarks:
   This has to be done once in the project. We do it in the file prj_debug.c.
   The default level of this trace group is DBG_LEVEL_HIGH.
Example:
   CREATE_TRACE_GROUP(LLC) - creates a logical trace group named LLC.
****************************************************************************/
#define CREATE_TRACE_GROUP(name) u32 G_nTraceGroup##name = DBG_LEVEL_HIGH;


/****************************************************************************
Description:
   Prints a trace message.
Arguments:
   name - Name of the trace group
   level - level of this message
   message - a printf compatible formated string + opt. arguments
Return Value:
   None.
Remarks:
   The string will be redirected via printf if level is higher or equal to the
   actual level for this trace group ( please see SetTraceLevel ).
Example:
   TRACE(LLC,DBG_LEVEL_NORMAL,("LLC> State:%d\n", nState));
****************************************************************************/
#define TRACE(name,level,message) do {if(level >= G_nTraceGroup##name) \
      { PRINTF message ; } } while(0)


/****************************************************************************
Description:
   Set the actual level of a trace group.
Arguments:
   name - Name of the trace group
   new_level - new trace level
Return Value:
   None.
Remarks:
   None.
****************************************************************************/
#define SetTraceLevel(name, new_level) {G_nTraceGroup##name = new_level;}

/****************************************************************************
Description:
   Get the actual level of a trace group.
Arguments:
   name - Name of the trace group
   new_level - new trace level
Return Value:
   None.
Remarks:
   None.
****************************************************************************/
#define GetTraceLevel(name) G_nTraceGroup##name

#else /* ENABLE_TRACE */
   #define DECLARE_TRACE_GROUP(name)
   #define CREATE_TRACE_GROUP(name)
   #define TRACE(name,level,message) {}
   #define SetTraceLevel(name, new_level) {}
   #define GetTraceLevel(name) 0
#endif /* ENABLE_TRACE */


#ifdef ENABLE_LOG
/****************************************************************************
Description:
   Prototype for a log group.
Arguments:
   name - Name of the log group
Return Value:
   None.
Remarks:
   None.
Example:
   DECLARE_LOG_GROUP(LLC) - declares a logical log group named LLC.
****************************************************************************/
#define DECLARE_LOG_GROUP(diag_group) extern u32 G_nLogGroup##diag_group;


/****************************************************************************
Description:
   Create a log group.
Arguments:
   name - Name of the log group
Return Value:
   None.
Remarks:
   This has to be done once in the project. We do it in the file prj_debug.c.
   The default level of this log group is DBG_LEVEL_HIGH.
Example:
   CREATE_LOG_GROUP(LLC) - creates a logical log group named LLC.
****************************************************************************/
#define CREATE_LOG_GROUP(diag_group) u32 G_nLogGroup##diag_group = DBG_LEVEL_HIGH;


/****************************************************************************
Description:
   Prints a log message.
Arguments:
   name - Name of the log group
   level - level of this message
   message - a c-string
Return Value:
   None.
Remarks:
   The string will be redirected via VOS_Log if level is higher or equal to the
   actual level for this log group ( please see SetLogLevel ).
Example:
   LOG(LLC,DBG_LEVEL_NORMAL,("LLC> State:%d\n", nState));
****************************************************************************/
#define LOG(diag_group,level,message) {if(level >= G_nLogGroup##diag_group) { DRV_LOG message; } else {}}

/****************************************************************************
Description:
   Set the actual level of a log group.
Arguments:
   name - Name of the log group
   new_level - new log level
Return Value:
   None.
Remarks:
   None.
****************************************************************************/
#define SetLogLevel(diag_group, new_level) {G_nLogGroup##diag_group = new_level;}

/****************************************************************************
Description:
   Get the actual level of a log group.
Arguments:
   name - Name of the log group
   new_level - new log level
Return Value:
   None.
Remarks:
   None.
****************************************************************************/
#define GetLogLevel(diag_group) G_nLogGroup##diag_group

#else /* ENABLE_LOG */
   #define DECLARE_LOG_GROUP(diag_group)
   #define CREATE_LOG_GROUP(diag_group)
   #define LOG(diag_group,level,message) {}
   #define SetLogLevel(diag_group, new_level) {}
   #define GetLogLevel(diag_group) 0
#endif /* ENABLE_LOG */

#ifdef DEBUG
extern void * sys_dbgMalloc(int size, int line, const char* sFile);
extern void sys_dbgFree (void *pBuf, int line, const char* sFile);
extern void dgbResult();
#endif /* DEBUG */
      
#endif /* _sys_debug_h */

