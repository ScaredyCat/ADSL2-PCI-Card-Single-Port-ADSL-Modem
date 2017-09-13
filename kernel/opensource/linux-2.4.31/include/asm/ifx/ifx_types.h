/************************************************************************
 *
 * Copyright (c) 2005
 * Infineon Technologies AG
 * St. Martin Strasse 53; 81669 Muenchen; Germany
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 ************************************************************************/

/* irq.h - INCA-IP2 interrupts */

#ifndef __IFX_TYPES_H
#define __IFX_TYPES_H

/* typedef enum {FALSE,TRUE}  bool_t;*/
typedef u8 bool_t;

#ifndef bool
typedef enum {
	false = 0,
	true = 1
} bool;
#endif

#ifndef BOOL
#ifdef FALSE
#undef FALSE
#endif
#ifdef TRUE
#undef TRUE
#endif
typedef enum { FALSE, TRUE } BOOL;
#endif

#ifndef OK
#define OK      0
#endif

#ifndef ERROR
#define ERROR  (-1)
#endif

#endif /* __INCincaip2IRQh */
