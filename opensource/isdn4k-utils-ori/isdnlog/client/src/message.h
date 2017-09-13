/* $Id: message.h,v 1.2 1998/05/10 23:40:13 luethje Exp $
 *
 * kisdnog for ISDN accounting for isdn4linux. (Report-module)
 *
 * Copyright 1996, 1997 by Stefan Luethje (luethje@sl-gw.lake.de)
 *                         Claudia Weber  (weber@sl-gw.lake.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Log: message.h,v $
 * Revision 1.2  1998/05/10 23:40:13  luethje
 * some changes
 *
 */

#include <fstream.h>

/******************************************************************************/

#define HandleMessage(a,b,c,d) _HandleMessage(__LINE__,__FILE__,a,b,c,MsgHdl::List2Array##d)

/******************************************************************************/

class MsgHdl
{
	private:
		enum {noMsg = -4, unknownErr = -3, noFileHandle = -2, noFile = -1};

	private:
		char    *MsgFileName;
		fstream *LogFile;
		int (*fct)(const char*, int);

	private:
		void FormatString (char *OutString, const char *FormatString, const char *ErrText[]);
		const char *ValueString(const char *ErrText[]);

	protected:
		virtual const char *GetText(int ErrNumber, int *Cnt);
		int InternalError(int ErrNumber, const char *fmt, ...);

	public:
		MsgHdl(int (*)(const char*, int), const char* = NULL, const char* = NULL);

		virtual ~MsgHdl();

		static const char **List2Array(int Argc,...);
		static const char *ltoa(long);
		static const char *dtoa(double, int = 0, int = 2);

		int _HandleMessage( int, const char *, int, int, int, const char *ErrText[]);
		int SetMsgFile(const char *NewMsgFileName);
};

/******************************************************************************/

