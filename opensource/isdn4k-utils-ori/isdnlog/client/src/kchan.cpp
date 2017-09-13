/* $Id: kchan.cpp,v 1.2 1998/05/10 23:40:03 luethje Exp $
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
 * $Log: kchan.cpp,v $
 * Revision 1.2  1998/05/10 23:40:03  luethje
 * some changes
 *
 */

#include <string.h>

#include <qtimer.h>

#include "kisdnlog.h"

/****************************************************************************/

KChannel::KChannel(KConnection* newmainwin, QWidget *newframe, int Scale) : QFrame(newframe)
{
	mainwin = newmainwin;
	frame = newframe;
	Timer = NULL;

	setFixedSize(CHAN_X,CHAN_Y);
	//setFrameStyle(QFrame::WinPanel | QFrame::Raised);

	thru = new KThruput(newmainwin,this,Scale);

	thru->move(BORDER_SIZE/2,BORDER_SIZE/2);
}

/****************************************************************************/

KChannel::~KChannel()
{
}

/****************************************************************************/

void KChannel::paintEvent(QPaintEvent*)
{
	int dir;
	int x;
	int y = 0;
	int old_y;
	QFont Font;
	QColor Color = QColor(255,255,255);
	QPainter Painter;

	Font.setPointSize((int) (Font.pointSize()*1.0));
	Painter.begin(this);
	Painter.setFont(Font);

	x = thru->x()+thru->width()+5;
	
	y = BORDER_SIZE/2 + Font.pointSize();
	Painter.setPen(Color);
	Painter.drawText(x, y, klocale->translate("Max"));

	for (dir=0; dir < 2; dir++)
	{
		y += Font.pointSize() + 3;
		Painter.setPen(thru->GetColor(dir));
		Painter.drawText(x+3, y, Byte2Str(thru->GetMaxRate(dir),GET_BPS|dir));
	}

	y += Font.pointSize() + 5;
	Painter.setPen(Color);
	Painter.drawText(x, y, klocale->translate("Average"));

	for (dir=0; dir < 2; dir++)
	{
		y += Font.pointSize() + 3;
		Painter.setPen(thru->GetColor(dir));
		Painter.drawText(x+3, y, Byte2Str(thru->GetRate(dir,KThruput::ALL_AVERAGE),GET_BPS|dir));
	}

	old_y = y += Font.pointSize() + 5;
	Painter.setPen(Color);
	Painter.drawText(x, y, klocale->translate("Current"));

	for (dir=0; dir < 2; dir++)
	{
		y += Font.pointSize() + 3;
		Painter.setPen(thru->GetColor(dir));
		Painter.drawText(x+3, y, Byte2Str(thru->GetRate(dir,KThruput::CUR_AVERAGE),GET_BPS|dir));
	}

	y = old_y;
	x = thru->x() + 5;
	Painter.setPen(Color);
	Painter.drawText(x, y, klocale->translate("Traffic"));

	for (dir=0; dir < 2; dir++)
	{
		y += Font.pointSize() + 3;
		Painter.setPen(thru->GetColor(dir));
		Painter.drawText(x+3, y, Byte2Str((double)thru->GetTraffic(dir),dir));
	}

	drawFrame(&Painter);
	Painter.end();
}

/****************************************************************************/

bool KChannel::AddValue(int duration, unsigned long in, unsigned long out)
{
	thru->AddValue(duration,in,out);

	repaint();
	return TRUE;
}

/****************************************************************************/

bool KChannel::SetTimeScale(int value)
{
	thru->SetTimeScale(value);

	return TRUE;
}

/****************************************************************************/

double KChannel::GetRate(int dir, int index)
{
	thru->GetRate(dir, index);

	return TRUE;
}

/****************************************************************************/

double KChannel::GetMinRate(int dir)
{
	thru->GetMinRate(dir);

	return TRUE;
}

/****************************************************************************/

double KChannel::GetMaxRate(int dir)
{
	thru->GetMaxRate(dir);

	return TRUE;
}

/****************************************************************************/

bool KChannel::StartConnection()
{
	DeleteConnection();

	thru->StartConnection();
	repaint();

	return TRUE;
}

/****************************************************************************/

bool KChannel::StopConnection()
{
	Timer = new QTimer(this);

	Timer->start(THRU_TIMEOUT * KTIMERSEC, TRUE);
	QObject::connect(Timer, SIGNAL(timeout()), SLOT(DeleteConnection()));

	thru->StopConnection();
	repaint();

	return TRUE;
}

/****************************************************************************/

const char *KChannel::DeleteConnection()
{
	if (Timer != NULL)
	{
		Timer->stop();
		delete Timer;
		Timer = NULL;
	}

	thru->DeleteConnection();
	repaint();

	return NULL;
}

/****************************************************************************/

