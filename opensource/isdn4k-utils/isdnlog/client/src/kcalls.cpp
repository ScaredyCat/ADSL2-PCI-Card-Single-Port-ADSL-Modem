/* $Id: kcalls.cpp,v 1.2 1998/05/10 23:40:03 luethje Exp $
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
 * $Log: kcalls.cpp,v $
 * Revision 1.2  1998/05/10 23:40:03  luethje
 * some changes
 *
 */

#include <qtimer.h>
#include <qtablevw.h>
#include <qpainter.h>

#include "kisdnlog.h"

/****************************************************************************/

KCalls::KCalls(KConnection *newmainwin, QWidget *newframe) : KTabListBox(newframe)
{
	mainwin = newmainwin;
	frame = newframe;
	setNumCols(CALLS_COLS);
	setNumRows(0);

	SetHeader();
}

/****************************************************************************/

KCalls::~KCalls()
{
}

/****************************************************************************/

bool KCalls::SetLine(int Line, const CALL *Info)
{
	int i = 0;


	changeItemPart(statetoa(Info->stat), Line, i++);
	changeItemPart(directiontoa(Info->dialin), Line, i++);
	changeItemPart(emptytoa(Info->num[_ME(Info)]), Line, i++);
	changeItemPart(emptytoa(Info->alias[_ME(Info)]), Line, i++);
	changeItemPart(emptytoa(Info->num[_OTHER(Info)]), Line, i++);
	changeItemPart(emptytoa(Info->vorwahl[_OTHER(Info)]), Line, i++);
	changeItemPart(emptytoa(Info->rufnummer[_OTHER(Info)]), Line, i++);
	changeItemPart(emptytoa(Info->alias[_OTHER(Info)]), Line, i++);
	changeItemPart(emptytoa(Info->area[_OTHER(Info)]), Line, i++);
	changeItemPart(timetoa(Info->connect), Line, i++);
	changeItemPart(durationtoa(Info->t_duration), Line, i++);
	changeItemPart(MsgHdl::ltoa(Info->aoce), Line, i++);
	changeItemPart(Info->money, Line, i++);
	changeItemPart(Info->currency, Line, i++);
	changeItemPart(Byte2Str((double) Info->ibytes, NO_DIR), Line, i++);
	changeItemPart(Byte2Str((double) Info->obytes, NO_DIR), Line, i++);
	changeItemPart(Byte2Str(Info->ibps, GET_BPS|NO_DIR), Line, i++);
	changeItemPart(Byte2Str(Info->obps, GET_BPS|NO_DIR), Line, i++);
	changeItemPart(Info->msg, Line, i++);

	return TRUE;
}

/****************************************************************************/

bool KCalls::SetHeader(void)
{
	int cnt = 0;

	WriteHeader(klocale->translate("Status"),cnt++);
	WriteHeader(klocale->translate("Direction"),cnt++);
	WriteHeader(klocale->translate("My Number"),cnt++);
	WriteHeader(klocale->translate("My Alias"),cnt++);
	WriteHeader(klocale->translate("Full Number"),cnt++);
	WriteHeader(klocale->translate("Areacode"),cnt++);
	WriteHeader(klocale->translate("Other Number"),cnt++);
	WriteHeader(klocale->translate("Other Alias"),cnt++);
	WriteHeader(klocale->translate("Area"),cnt++);
	WriteHeader(klocale->translate("Connecting time"),cnt++);
	WriteHeader(klocale->translate("Duration"),cnt++);
	WriteHeader(klocale->translate("Units"),cnt++);
	WriteHeader(klocale->translate("Fee"),cnt++);
	WriteHeader(klocale->translate("Currency"),cnt++);
	WriteHeader(klocale->translate("Bytes in"),cnt++);
	WriteHeader(klocale->translate("Bytes out"),cnt++);
	WriteHeader(klocale->translate("Bps in"),cnt++);
	WriteHeader(klocale->translate("Bps out"),cnt++);
	WriteHeader(klocale->translate("Message"),cnt++);

	if (cnt != PROT_ELEMENTS-1)
	{
		Messager->HandleMessage(TRUE,KI_INVALID_ELEMS,0,(2,MsgHdl::ltoa(cnt),MsgHdl::ltoa(PROT_ELEMENTS)));
		return FALSE;
	}

	return TRUE;
}

/****************************************************************************/

bool KCalls::WriteHeader(const char *entry, int index)
{
	if (index < 0 || index >= numCols() || entry == NULL)
		return FALSE;

	setColumn(index, entry, (strlen(entry)+1) * 10, TextColumn);

	return TRUE;
}

/****************************************************************************/

const char *KCalls::timetoa(time_t Time)
{
	static char RetCode[25];

	struct tm *tm;

	tm = localtime(&Time);

	strftime(RetCode,25,"%X %x",tm);
	return RetCode;
}

/****************************************************************************/

const char *KCalls::durationtoa(time_t Time)
{
	static char RetCode[16];

	struct tm *tm;

	tm = gmtime(&Time);

	strftime(RetCode,16,"%X",tm);
	return RetCode;
}

/****************************************************************************/

const char *KCalls::directiontoa(int dir)
{
	if (dir == DIALOUT)
		return klocale->translate("dial out");

	return klocale->translate("dial in");
}

/****************************************************************************/

const char *KCalls::emptytoa(const char *string)
{
	return (string == NULL || string[0] == '\0'?klocale->translate("Unknown"):string);
}

/****************************************************************************/

const char *KCalls::statetoa(int state)
{
	const char *RetCode;

	switch (state)
	{
		case STATE_CONNECT:
			RetCode = klocale->translate("connected"); 
			break;
		case STATE_BYTE:
			RetCode = klocale->translate("got thruput"); 
			break;
		case STATE_AOCD:
			RetCode = klocale->translate("got AOCD"); 
			break;
		case STATE_TIME:
			RetCode = klocale->translate("got time"); 
			break;
		case STATE_RING:
			RetCode = klocale->translate("ringing"); 
			break;
		case STATE_CAUSE:
			RetCode = klocale->translate("got message"); 
			break;
		case STATE_HANGUP:
			RetCode = klocale->translate("hangup"); 
			break;
		default:
			RetCode = klocale->translate("unknown state"); 
			break;
	}

	return RetCode;
}

/****************************************************************************/

bool KCalls::WriteConfig(const char *EntName)
{
	int i;
	char s[BUFSIZ] = "";
	KConfig *config = ((KConnection*) topLevelWidget())->GetConfig();

	if (config == NULL)
		return FALSE;

	for (i = 0; i < numCols(); i++)
	{
		strcat(s,MsgHdl::ltoa(columnWidth(i)));
		strcat(s,",");
	}

	config->setGroup(KI_SEC_GLOBAL);
	config->writeEntry(EntName,s);

	return TRUE;
}

/****************************************************************************/

bool KCalls::ReadConfig(const char *EntName)
{
	KConfig *config = ((KConnection*) topLevelWidget())->GetConfig();
	QStrList List;
	char *elem;
	int i = 0;

	if (config == NULL)
		return FALSE;

	config->setGroup(KI_SEC_GLOBAL);
	config->readListEntry(EntName,List);

	if ((elem = List.first()) != NULL)
		do
		{
			setColumnWidth(i++,atoi(elem));
		}
		while((elem = List.next()) != NULL);

	return TRUE;
}

/****************************************************************************/

