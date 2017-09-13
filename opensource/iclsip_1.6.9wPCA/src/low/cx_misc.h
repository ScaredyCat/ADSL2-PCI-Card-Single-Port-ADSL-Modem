/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * cx_misc.h
 *
 * $Id: cx_misc.h,v 1.10 2004/12/20 03:41:53 tyhuang Exp $
 */

#ifndef CX_MISC_H
#define CX_MISC_H

#ifdef  __cplusplus
extern "C" {
#endif

#ifdef UNIX
#include <errno.h>
#elif defined(_WIN32)
#include <windows.h>
#include <winbase.h>
#endif
#include <common/cm_def.h>

#if defined(UNIX)
#ifndef closesocket
#define closesocket(s)		close(s)
#endif
#define getError()		errno
void sleeping(unsigned int msec);
#elif defined(_WIN32)
#define getError()		WSAGetLastError()
#define sleeping(msec)		Sleep(msec)
#endif

#ifdef  __cplusplus
}
#endif

#endif  /* CX_MISC_H */
