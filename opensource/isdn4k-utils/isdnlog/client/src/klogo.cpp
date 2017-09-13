/* $Id: klogo.cpp,v 1.3 1999/05/23 14:34:41 luethje Exp $
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
 * $Log: klogo.cpp,v $
 * Revision 1.3  1999/05/23 14:34:41  luethje
 * kisdnlog is ready for isdnlog 3.x and kde 1.1.1
 *
 * Revision 1.2  1998/05/10 23:40:07  luethje
 * some changes
 *
 */

#include<string.h>

#include <kiconloader.h>

#include "kisdnlog.h"

/****************************************************************************/

KLogo::KLogo(QWidget *mainwin, const char *newpixmap) : QFrame(mainwin)
{
	KIconLoader Icon;

Icon.insertDirectory(6,"../pixmaps");

	if (newpixmap != NULL)
		pixmap = strdup(newpixmap);
	else
		pixmap = NULL;

	if (pixmap != NULL)
		pic = Icon.loadIcon(pixmap);

	if (pic.size().width())
		setFixedSize(pic.size());
	else
		setFixedSize(LOGO_X, LOGO_Y);

	repaint();
}

/****************************************************************************/

KLogo::~KLogo()
{
	if (pixmap != NULL)
		free(pixmap);
}

/****************************************************************************/

void KLogo::paintEvent(QPaintEvent*)
{
	QPainter Painter;

	if (pic.size().width())
	{
		Painter.begin(&pic);
		Painter.end();
		bitBlt( this, 0,0, &pic);
	}
	else
	{
		QString Message;

		Message = klocale->translate("No Bitmap found");
		Message += " (";
		Message += pixmap;
		Message += ")";

		Painter.begin(this);
		Painter.drawText(0, 0, width(), height(),
		                 WordBreak | AlignVCenter | AlignCenter,
		                 Message);
		drawFrame(&Painter);
		Painter.end();
	}
}

/****************************************************************************/

