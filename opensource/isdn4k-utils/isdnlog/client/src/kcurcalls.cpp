/* $Id: kcurcalls.cpp,v 1.3 1999/05/23 14:34:39 luethje Exp $
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
 * $Log: kcurcalls.cpp,v $
 * Revision 1.3  1999/05/23 14:34:39  luethje
 * kisdnlog is ready for isdnlog 3.x and kde 1.1.1
 *
 * Revision 1.2  1998/05/10 23:40:05  luethje
 * some changes
 *
 */

#include <qtimer.h>
#include <qtablevw.h>
#include <qpainter.h>

#include "kisdnlog.h"

/****************************************************************************/

KCurCalls::KCurCalls(KConnection *mainwin, QWidget *frame) :
KCalls(mainwin, frame)
{
	channr = NULL;

	Timer = new QTimer(this);
	connect(Timer, SIGNAL(timeout()), SLOT(ClearLines()));
	Timer->start(CALLS_TIMEOUT * KTIMERSEC, FALSE);

	ReadConfig(KI_ENT_CUR_CALLS);
}

/****************************************************************************/

KCurCalls::~KCurCalls()
{
	Timer->stop();
	delete Timer;

	free(channr);
}

/****************************************************************************/

bool KCurCalls::WriteConfig()
{
	return KCalls::WriteConfig(KI_ENT_CUR_CALLS);
}

/****************************************************************************/

bool KCurCalls::WriteLine(int Line, const CALL *Info)
{
	Line = get_chan(Line);

	if (Line >= numRows() || Line < 0)
		return FALSE;

	channr[Line].stat = Info->stat;
	channr[Line].time = time(NULL);    

	return SetLine(Line, Info);
}

/****************************************************************************/

int KCurCalls::new_chan(int chan)
{
	if ((channr = (chan_struct*) realloc(channr,(numRows()+1)*sizeof(chan_struct))) == NULL)
	{
		Messager->HandleMessage(TRUE,KI_OUT_OF_MEMORY,0,(0));
		return NO_CHAN;
	}

	memmove((void*) (channr+1),
	        (void*) channr,
	        numRows()*sizeof(chan_struct));
	insertItem("",0);
	channr[0].chan = chan;

	return 0;
}

/****************************************************************************/

int KCurCalls::get_chan(int chan)
{
	int i;

	for (i = 0; i < numRows() && channr[i].chan != chan; i++);

	if (i == numRows())
		return NO_CHAN;

	return i;
}

/****************************************************************************/

int KCurCalls::change_chan(int chan, int changechan)
{
	int i;

	if ((i = get_chan(chan)) != NO_CHAN)
		channr[i].chan = changechan;
	else
		return NO_CHAN;

	return i;
}

/****************************************************************************/

int KCurCalls::del_chan(int delchan)
{
	int i;

	if ((i = get_chan(delchan)) != NO_CHAN)
	{
		if (numRows() < 2 || channr == NULL)
		{
			free(channr);
			channr = NULL;
		}
		else
		{
			if (i+1 < numRows())
			{
				memmove((void*) (channr+i),
				        (void*) (channr+i+1),
				        (numRows()-i-1)*sizeof(chan_struct));
			}

			if ((channr = (chan_struct*) realloc(channr,(numRows()-1)*sizeof(chan_struct))) == NULL)
			{
				Messager->HandleMessage(TRUE,KI_OUT_OF_MEMORY,0,(0));
				return NO_CHAN;
			}
		}

		removeItem(i);
	}
	else
		return NO_CHAN;

	return i;
}

/****************************************************************************/

bool KCurCalls::ClearLines()
{
	int i;

	for (i = 0; i < numRows(); i++)
		if ((channr[i].stat == STATE_CAUSE || channr[i].stat == STATE_RING) &&
		    channr[i].time + CALLS_TIMEOUT < time(NULL))
			del_chan(channr[i].chan);

	return TRUE;
}

/****************************************************************************/

