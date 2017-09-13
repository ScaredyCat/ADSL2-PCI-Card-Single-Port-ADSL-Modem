/* $Id: kthruput.cpp,v 1.3 1999/05/23 14:34:42 luethje Exp $
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
 * $Log: kthruput.cpp,v $
 * Revision 1.3  1999/05/23 14:34:42  luethje
 * kisdnlog is ready for isdnlog 3.x and kde 1.1.1
 *
 * Revision 1.2  1998/05/10 23:40:11  luethje
 * some changes
 *
 */

#include <string.h>

#include "kisdnlog.h"

/****************************************************************************/

KThruput::KThruput(KConnection* newmainwin, QWidget *newframe, int newScale) : QFrame(newframe)
{
//	initMetaObject();

	setFixedSize(THRU_X,THRU_Y);
//	setFrameStyle(QFrame::WinPanel | QFrame::Raised);

	mainwin = newmainwin;
	frame = newframe; 

	TimeScale = newScale;

	BGColor.setRgb(0,0,0);
	PenColor[IN_DATA].setRgb(0,255,0);
	PenColor[OUT_DATA].setRgb(0,0,255);

	ValueSize = width()+1;
	Values[IN_DATA] = new int[ValueSize];
	Values[OUT_DATA] = new int[ValueSize];
	RunTime = new int[ValueSize];

	DeleteConnection();

	repaint();
}

/****************************************************************************/

KThruput::~KThruput()
{
	delete[] Values[IN_DATA];
	delete[] Values[OUT_DATA];
	delete[] RunTime;
}

/****************************************************************************/

void KThruput::paintEvent(QPaintEvent*)
{
	QPainter Painter;


	Painter.begin(this);

	// Set Background
	Painter.setPen(BGColor);
	Painter.setBrush(BGColor);
	Painter.drawRect(0,0,width(),height());

	if (Connected >= 0)
	{
		// Show In-Data
		DrawRate(Painter,IN_DATA);

		// Show Out-Data
		DrawRate(Painter,OUT_DATA);
	}

	if (NotConnected)
	{
		QColor Color = QColor(255,255,255);

		Painter.setPen(Color);
		Painter.drawText(0, 0, width(), height(),
		                 WordBreak | AlignVCenter | AlignCenter,
		                 klocale->translate("Not connected"));
	}

	drawFrame(&Painter);
	Painter.end();
}

/****************************************************************************/

bool KThruput::DrawRate(QPainter &Painter, int dir)
{
	int x = 0;
	int y;
	int max_rate = MAX_RATE;// > (int) MaxRate[dir] ? MAX_RATE : (int) MaxRate[dir];
	int last_y = height() - 1 - ((int) (GetRate(dir,x) * (height() - 1) / max_rate));

	Painter.setPen(PenColor[dir]);

	for (x = 1; x <= CurIndex(); x++)
	{
		y = height() - 1 - ((int) (GetRate(dir,x) * (height() - 1) / max_rate));
		Painter.drawLine(x-1, last_y, x, y);

		last_y = y;
	}

	return TRUE;
}

/****************************************************************************/

double KThruput::GetRate(int dir, int index)
{
	int interval;


	if (index == CUR_AVERAGE)
		index = CurIndex();
	else
	if (index == ALL_AVERAGE)
	{
		index = CurIndex();

		if (RunTime[index] <= 0) 
			return 0;

		return ((double) Values[dir][index]) / RunTime[index];
	}
	else
	if (index == 0)
	{
		if (RunTime[0] <= 0) 
			return 0;

		return ((double) Values[dir][0]) / RunTime[0];
	}

	if (index < 1 || index >= ValueSize || (interval = RunTime[index] - RunTime[index-1]) <= 0)
		return 0;

	return ((double) (Values[dir][index] - Values[dir][index-1]))/interval;
}

/****************************************************************************/

bool KThruput::ShiftArray(int *Array)
{
	memmove((void*) Array,(void*) (Array+1),(ValueSize - 1)*sizeof(int));

	Array[ValueSize - 1] = 0;

	return TRUE;
}

/****************************************************************************/

bool KThruput::AddValue(int dir, unsigned long data)
{
	double rate;
	int index = CurIndex();


	Values[dir][index] = data;

	rate = GetRate(dir, index);

	if (MaxRate[dir] < rate)
		MaxRate[dir] = rate;

	if (MinRate[dir] > rate || MinRate[dir] == 0)
		MinRate[dir] = rate;

	return TRUE;
}

/****************************************************************************/

bool KThruput::AddValue(int duration, unsigned long in, unsigned long out)
{
	int index = CurIndex();

	if (index >= 0 && RunTime[index] == duration)
		return TRUE;

	index = CurIndex(COUNT);

	RunTime[index]  = duration;

	AddValue(IN_DATA,in);
	AddValue(OUT_DATA,out);

	repaint();
	return TRUE;
}

/****************************************************************************/

bool KThruput::SetTimeScale(int value)
{
	TimeScale = value;

	/* Sonst kommen die Werte durcheinander */
	if (Connected >= 0)
	{
		StopConnection();
		StartConnection();
	}

	return TRUE;
}

/****************************************************************************/

bool KThruput::StartConnection()
{
	DeleteConnection();

	Connected = 0;
	NotConnected = FALSE;

	repaint();
	return TRUE;
}

/****************************************************************************/

bool KThruput::StopConnection()
{
	NotConnected = TRUE;

	repaint();
	return TRUE;
}

/****************************************************************************/

const char *KThruput::DeleteConnection()
{
	int i;


	Connected = NOT_CONNECTED;
	NotConnected = TRUE;

	for (i = 0; i < ValueSize; i++)
		Values[IN_DATA][i] = Values[OUT_DATA][i] = RunTime[i] = 0;

	MinRate[IN_DATA] = MinRate[OUT_DATA] = 0;
	MaxRate[IN_DATA] = MaxRate[OUT_DATA] = 0;

	repaint();
	return NULL;
}

/****************************************************************************/

double KThruput::GetMinRate(int dir)
{
	return MinRate[dir];
}

/****************************************************************************/

double KThruput::GetMaxRate(int dir)
{
	return MaxRate[dir];
}

/****************************************************************************/

int KThruput::GetTraffic(int dir)
{
	return Values[dir][CurIndex()];;
}

/****************************************************************************/

QColor &KThruput::GetColor(int dir)
{
	return PenColor[dir];
}

/****************************************************************************/

int KThruput::CurIndex(int todo)
{
	int index;

	if (todo == COUNT)
		Connected++;

	index = ((Connected-1)/TimeScale)+1;

	if (Connected < 1)
		index = 0;

	if (index >= ValueSize)
	{
		if (todo == COUNT && index > ((Connected-2)/TimeScale)+1)
		{
			ShiftArray(RunTime);
			ShiftArray(Values[IN_DATA]);
			ShiftArray(Values[OUT_DATA]);
		}

		index = ValueSize-1;
	}

	return index;
}

/****************************************************************************/

