/* $Id: ksplit.cpp,v 1.2 1998/05/10 23:40:10 luethje Exp $
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
 * $Log: ksplit.cpp,v $
 * Revision 1.2  1998/05/10 23:40:10  luethje
 * some changes
 *
 */

#include "kisdnlog.h" 

/****************************************************************************/

KSplit::KSplit(KConnection* newmainwin, QWidget *newframe, enum KNewPanner::Orientation dir, KNewPanner::Units type, int units) : KNewPanner(newframe, NULL, dir, type, units)
{
	mainwin = newmainwin;
	frame = newframe;

	resize(frame->size());
}

/****************************************************************************/

KSplit::~KSplit()
{
}

/****************************************************************************/

void KSplit::SetSize()
{
	resize(frame->size());
}

/****************************************************************************/

