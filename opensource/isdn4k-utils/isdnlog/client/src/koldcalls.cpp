/* $Id: koldcalls.cpp,v 1.2 1998/05/10 23:40:10 luethje Exp $
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
 * $Log: koldcalls.cpp,v $
 * Revision 1.2  1998/05/10 23:40:10  luethje
 * some changes
 *
 */

#include <qtimer.h>
#include <qtablevw.h>
#include <qpainter.h>

#include "kisdnlog.h"

/****************************************************************************/

KOldCalls::KOldCalls(KConnection *mainwin, QWidget *frame, int newmaxlines) :
KCalls(mainwin, frame)
{
	SetMaxLines(newmaxlines);

	ReadConfig(KI_ENT_OLD_CALLS);
}

/****************************************************************************/

KOldCalls::~KOldCalls()
{
}

/****************************************************************************/

bool KOldCalls::WriteConfig()
{
	return KCalls::WriteConfig(KI_ENT_OLD_CALLS);
}

/****************************************************************************/

bool KOldCalls::AppendLine(const CALL *Info)
{
	bool RetCode;
	int Line = numRows();


	if (maxlines <= Line)
	{
		removeItem(0);
		Line--;
	}

	insertItem("",Line);
	RetCode = SetLine(Line, Info);
	setTopItem(Line);

	return RetCode;
}

/****************************************************************************/

bool KOldCalls::SetMaxLines(int newmaxlines)
{
	if (newmaxlines < 1)
	{
		Messager->HandleMessage(TRUE,KI_INVALID_LINES,0,
		                        (2,klocale->translate("Old Calls"),
		                         MsgHdl::ltoa(newmaxlines)));

		maxlines = MAX_CALLS_LINES;

		return FALSE;
	}

	maxlines = newmaxlines;

	while (maxlines < numRows())
		removeItem(0);

	return TRUE;
}

/****************************************************************************/

