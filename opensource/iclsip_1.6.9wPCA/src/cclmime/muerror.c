/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include "muerror.h"

int
mu_default_error_printer (const char *fmt, va_list ap)
{
  int status;
  status = vfprintf (stderr, fmt, ap);
  if (status >= 0)
    {
      if (fputc ('\n', stderr) != EOF)
	status++;
    }
  return status;
}

static error_pfn_t mu_error_printer = mu_default_error_printer;

int
mu_error (const char *fmt, ...)
{
  int status;
  va_list ap;

  if (!mu_error_printer)
    return 0;
  va_start (ap, fmt);
  status = (*mu_error_printer) (fmt, ap);
  va_end (ap);
  return status;
}


