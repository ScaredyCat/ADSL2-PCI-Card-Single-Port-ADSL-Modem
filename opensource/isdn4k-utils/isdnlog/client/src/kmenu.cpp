/* $Id: kmenu.cpp,v 1.3 1998/05/11 00:02:39 luethje Exp $
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
 * $Log: kmenu.cpp,v $
 * Revision 1.3  1998/05/11 00:02:39  luethje
 * Made some changes
 *
 * Revision 1.2  1998/05/10 23:40:09  luethje
 * some changes
 *
 */

#include <kmenubar.h>

#include "kisdnlog.h"

/****************************************************************************/

KMenu::KMenu(KConnection *newmainwin, bool logVisible) : KMenuBar(newmainwin)
{
	char HelpText[300];
	mainwin = newmainwin;

	fileMenu = new QPopupMenu();
	fileMenu->insertItem(klocale->translate("&Connect"), mainwin, SLOT(NewConnect()));
	reconnect = fileMenu->insertItem(klocale->translate("&Reconnect"), mainwin, SLOT(ReConnect()));
	disconnect = fileMenu->insertItem(klocale->translate("&Disconnect"), mainwin, SLOT(Disconnect()));
	writefile = fileMenu->insertItem(klocale->translate("&Save logfile"), mainwin, SLOT(SaveLogFile()));
	fileMenu->insertItem(klocale->translate("&Quit"), mainwin, SLOT(Quit()));
//	fileMenu->insertItem("&Disonnect", this, SLOT(moveWindow()));
//	fileMenu->insertItem("&Write Logfile", this, SLOT(moveWindow()));

	winMenu = new QPopupMenu();
	winMenu->setCheckable(TRUE);
	connect(winMenu, SIGNAL(activated(int)), mainwin, SLOT(SetLogWin()));
	showlog = winMenu->insertItem(klocale->translate("&Show Log"), 1);
	winMenu->setItemChecked(showlog,logVisible);
	//winMenu->insertSeparator();
	//showlog = winMenu->insertItem(klocale->translate("&Show Log"), mainwin, SLOT(SetLogWin()));
	//winMenu->setItemChecked(showlog,logVisible);

//	helpMenu = new KPopupMenu();

	strcpy(HelpText,KISDNLOG_NAME" "VERSION"\n\n"DEV_SL"\n"DEV_CW"\n\n");
	strcat(HelpText,klocale->translate("monitoring tool for the isdnlog"));
	QPopupMenu *helpMenu = kapp->getHelpMenu(TRUE, HelpText);

	insertItem(klocale->translate("&File"), fileMenu);
	insertItem(klocale->translate("&Windows"), winMenu);
	insertSeparator();
	insertItem(klocale->translate("&Help"), helpMenu);

	enableMoving();
	enableFloating();
}

/****************************************************************************/

KMenu::~KMenu()
{
	delete fileMenu;
	delete winMenu;
}

/****************************************************************************/

void KMenu::SetLogShowed(bool enable)
{
	winMenu->setItemChecked(showlog,enable);
}

/****************************************************************************/

bool KMenu::IsLogShowed()
{
	return winMenu->isItemChecked(showlog);
}

/****************************************************************************/

bool KMenu::Connect(bool enable)
{
	fileMenu->setItemEnabled(writefile, enable);
	fileMenu->setItemEnabled(reconnect, enable);
	fileMenu->setItemEnabled(disconnect, enable);
	winMenu->setItemEnabled(showlog, enable);

	return TRUE;
}

/****************************************************************************/

