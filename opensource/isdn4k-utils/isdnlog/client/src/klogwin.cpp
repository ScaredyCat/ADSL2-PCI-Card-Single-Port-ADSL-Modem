/* $Id: klogwin.cpp,v 1.2 1998/05/10 23:40:08 luethje Exp $
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
 * $Log: klogwin.cpp,v $
 * Revision 1.2  1998/05/10 23:40:08  luethje
 * some changes
 *
 */

#include <errno.h>

#include "kisdnlog.h"

/****************************************************************************/

KLogWin::KLogWin(KConnection *newmainwin, int newmaxlines) :
KTopLevelWidget()
{
	mainwin = newmainwin;
	log = new KLog(newmainwin,this,newmaxlines);
}

/****************************************************************************/

KLogWin::~KLogWin()
{
	delete log;
}

/****************************************************************************/

void KLogWin::hide()
{
	QWidget::hide();
	mainwin->HideLogWin();
}

/****************************************************************************/

void KLogWin::Quit()
{
	QWidget::close(TRUE);
}

/****************************************************************************/

void KLogWin::updateRects()
{
	log->resize(size());
}

/****************************************************************************/

bool KLogWin::SetMaxLines(int newmaxlines)
{
	if (log != NULL)
		return log->SetMaxLines(newmaxlines);

	return FALSE;
}

/****************************************************************************/

void KLogWin::AppendLine(char *Line)
{
	if (log != NULL)
		log->AppendLine(Line);
}

/****************************************************************************/

bool KLogWin::SaveToFile(const char *Name)
{
	if (log != NULL)
		return log->SaveToFile(Name);

	return FALSE;
}

/****************************************************************************/

