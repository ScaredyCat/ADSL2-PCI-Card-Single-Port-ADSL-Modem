/* $Id: khost.cpp,v 1.2 1998/05/10 23:40:05 luethje Exp $
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
 * $Log: khost.cpp,v $
 * Revision 1.2  1998/05/10 23:40:05  luethje
 * some changes
 *
 */

#include <qcombo.h> 
#include <qaccel.h>
#include <qpushbt.h>

#include "kisdnlog.h"

/****************************************************************************/

KHost::KHost(KConnection *newmainwin) : QDialog(newmainwin,NULL,TRUE)
{
	mainwin = newmainwin;

	frame  = new QFrame(this);
	portValid = new QIntValidator(0,65535,this);

	hostCombo = new QComboBox(true, frame);
	portCombo = new QComboBox(true, frame);

	hostLabel  = new QLabel(klocale->translate("Host"), frame);
	portLabel  = new QLabel(klocale->translate("Port"), frame);

	ok     = new QPushButton(klocale->translate("OK"), frame);
	cancel = new QPushButton(klocale->translate("Cancel"), frame);
	help   = new QPushButton(klocale->translate("Help"), frame);

	QObject::connect(hostCombo, SIGNAL(activated(int)),
				this, SLOT(hostChanged(int)));
	QObject::connect(portCombo, SIGNAL(activated(int)),
				this, SLOT(portChanged(int))); 

	QObject::connect(ok, SIGNAL(clicked()), this, SLOT(go()));
	QObject::connect(cancel, SIGNAL(clicked()), this, SLOT(Quit()));
	QObject::connect(help, SIGNAL(clicked()), this, SLOT(showHelp()));

	ok->setAccel(Key_Return);
	cancel->setAccel(Key_Escape);

	portCombo->setValidator(portValid);

	setFixedSize(280,150);

	ReadConfig();
}

/****************************************************************************/

KHost::~KHost()
{
	WriteConfig();

	delete portValid;
	delete hostCombo;
	delete portCombo;
	delete hostLabel;
	delete portLabel;
	delete frame;
}

/****************************************************************************/

void KHost::show()
{
	hostCombo->setFocus();

	QDialog::show();
}

/****************************************************************************/

void KHost::resizeEvent(QResizeEvent*)
{
	int len;

	frame->resize(size());

	hostLabel->setGeometry (15, 10, 30, 30);
	hostCombo->setGeometry (hostLabel->x(), hostLabel->y()+hostLabel->height(),
	               160, 30);

	portLabel->setGeometry(hostCombo->x()+hostCombo->width()+15, 
                 hostLabel->y(), hostLabel->height(), hostLabel->width());
	portCombo->setGeometry(portLabel->x(), hostCombo->y(),
	               75, hostCombo->height());

	len = (portCombo->x()+portCombo->width() - hostCombo->x()) / 4;

	ok->setGeometry (hostCombo->x(), portCombo->y() + hostCombo->height() + 20,
	                 len, 30);
	cancel->setGeometry (ok->x() + (int)(ok->width() *1.5), ok->y(),
	                 ok->width(), ok->height());
	help->setGeometry (cancel->x() + (int)(ok->width() *1.5), ok->y(),
	                 ok->width(), ok->height());
}

/****************************************************************************/

void KHost::hostChanged(int)
{
	for(int i=0; i<hostCombo->count()-1; i++)
		for(int j=i+1; j<hostCombo->count(); j++)
			if(!strcmp(hostCombo->text(i), hostCombo->text(j)))
			{
				hostCombo->removeItem(i);
				break;
			}
}

/****************************************************************************/

/*
void KHost::portChanged(int Index)
{
	long dummy;
	const char *text = portCombo->text(Index);
	QString Message;

	if (!is_integer((char*)text,&dummy) || dummy < 0)
	{
		Messager->HandleMessage(TRUE,KI_INVALID_PORT,0,(1,text));
		portCombo->removeItem(Index);
	}
}
*/

void KHost::portChanged(int)
{
	for(int i=0; i<portCombo->count()-1; i++)
		for(int j=i+1; j<portCombo->count(); j++)
			if(!strcmp(portCombo->text(i), portCombo->text(j)))
			{
				portCombo->removeItem(i);
				break;
			}
}

/****************************************************************************/

bool KHost::ReadConfig()
{
	KConfig *config = ((KConnection*) topLevelWidget())->GetConfig();
	QStrList List;
	const char *Value;
	int i;

	if (config == NULL)
		return FALSE;

	config->setGroup(KI_SEC_HOSTS);

	config->readListEntry(KI_ENT_HOSTS,List);
	hostCombo->insertStrList(&List);

	List.clear();

	config->readListEntry(KI_ENT_PORTS,List);
	portCombo->insertStrList(&List);

	Value = config->readEntry(KI_ENT_LAST_HOST);

	if (Value != NULL && *Value != '\0')
		for (i = 0; i < hostCombo->count(); i++)
			if (!strcmp(Value,hostCombo->text(i)))
			{
				hostCombo->setCurrentItem(i);
				break;
			}

	Value = config->readEntry(KI_ENT_LAST_PORT);

	if (Value != NULL && *Value != '\0')
		for (i = 0; i < portCombo->count(); i++)
			if (!strcmp(Value,portCombo->text(i)))
			{
				portCombo->setCurrentItem(i);
				break;
			}

	hostChanged(0);
	portChanged(0);

	return TRUE;
}

/****************************************************************************/

bool KHost::WriteConfig()
{
	KConfig *config = ((KConnection*) topLevelWidget())->GetConfig();
	QStrList List;
	const char *Value;
	int i;

	if (config == NULL)
		return FALSE;

	hostChanged(0);
	portChanged(0);

	config->setGroup(KI_SEC_HOSTS);

	if ((Value = hostCombo->currentText()) != NULL && *Value != '\0' && List.find(Value) == -1)
		List.append(Value);

	for (i = 0; i < hostCombo->count(); i++)
		if (List.find(hostCombo->text(i)) == -1)
			List.append(hostCombo->text(i));

	config->writeEntry(KI_ENT_HOSTS,List);
	List.clear();

	if ((Value = portCombo->currentText()) != NULL && *Value != '\0' && List.find(Value) == -1)
		List.append(Value);

	for (i = 0; i < portCombo->count(); i++)
		if (List.find(portCombo->text(i)) == -1)
			List.append(portCombo->text(i));

	config->writeEntry(KI_ENT_PORTS,List);

	config->writeEntry(KI_ENT_LAST_HOST,hostCombo->currentText());
	config->writeEntry(KI_ENT_LAST_PORT,portCombo->currentText());

	return TRUE;
}

/****************************************************************************/

void KHost::showHelp()
{
	printf("Will be come later :-)\n");
}

/****************************************************************************/

void KHost::Quit()
{
	mainwin->SetHost(NULL);
	mainwin->SetPort(NULL);
	reject();
}

/****************************************************************************/

void KHost::go()
{
	mainwin->SetHost(hostCombo->currentText());
	mainwin->SetPort(portCombo->currentText());
	accept();
}

/****************************************************************************/

