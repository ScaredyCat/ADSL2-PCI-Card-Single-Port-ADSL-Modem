/* $Id: kconnect.cpp,v 1.4 1999/05/23 14:34:37 luethje Exp $
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
 * $Log: kconnect.cpp,v $
 * Revision 1.4  1999/05/23 14:34:37  luethje
 * kisdnlog is ready for isdnlog 3.x and kde 1.1.1
 *
 * Revision 1.3  1998/09/22 20:58:53  luethje
 * isdnrep:  -fixed wrong provider report
 *           -fixed wrong html output for provider report
 *           -fixed strange html output
 * kisdnlog: -fixed "1001 message window" bug ;-)
 *
 * Revision 1.2  1998/05/10 23:40:04  luethje
 * some changes
 *
 */

#include <pwd.h>
#include <sys/types.h>

#include <qframe.h>
#include <qdialog.h>

#include <stream.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>

#include "kisdnlog.h"

#include <errno.h>

/****************************************************************************/

int KConnection::logVisible   = FALSE;
int KConnection::ConnectionNr = 0;
bool KConnection::Restore = FALSE;
KConfig *KConnection::config = NULL;
QString KConnection::CurDir;
QList<KConnection>  KConnection::ConnectList;

/****************************************************************************/

KConnection::KConnection(KApplication *newapp, const char *NewHost, const char *NewPort) 
: KTopLevelWidget()
{
	app = newapp;

	ConnectList.append(this);

	if (++ConnectionNr == 1)
	{
		app->setTopWidget(this);
		config = app->getConfig();
		ReadConfig();
	}

	channels = 0;
	socket = NULL;
	server = NULL;

	SetHost(NewHost);
	SetPort(NewPort);

	ActiveRect = geometry();

	sn = NULL;
	chanwin = NULL;
	channr = NULL;
	curcalls = NULL;
	oldcalls = NULL;
	logwin = NULL;
	split1 = NULL;
	outwin = NULL;
	menu = NULL;
	logo = NULL;
	// config = this.getConfig();

	menu = new KMenu(this,logVisible);
	setMenu(menu);
	menu->Connect(FALSE);

	SetLogo();
}

/****************************************************************************/

KConnection::~KConnection()
{
	DestroyLogs();

	delete menu;

	if (logo != NULL)
		delete logo;

	ConnectList.removeRef(this);

	if (--ConnectionNr > 0)
	{
		if (app->topWidget() == this && ConnectList.first() != NULL)
			app->setTopWidget(ConnectList.first());
	}

printf("Closed Window: %d\n", ConnectionNr);
}

/****************************************************************************/

void KConnection::Quit()
{
printf("Quit\n");
	WriteConfig();
	emit quit();
	qApp->exit(0);
}

/****************************************************************************/

bool KConnection::NewConnect()
{
	if (ConnectionNr == 1 && socket == NULL)
	{
		/* I am the only window and have no connection */
		if (WinConnect())
			show();
		else
			return FALSE;
	}
	else
	{
		KConnection *conn = new KConnection(app);

		if (conn->WinConnect())
			conn->show();
		else
		{
			delete conn;
			return FALSE;
		}
	}

	return TRUE;
}

/****************************************************************************/

bool KConnection::WinConnect()
{
	int sock;
	KHost hostwin(this);

	if (socket != NULL)
		return FALSE;

	hostwin.setCaption(klocale->translate("Connect to server"));

	do
	{
		sock = -1;

		if (hostwin.exec() == FALSE)
			return FALSE;
	
		if (server == NULL || *server == '\0')
			Messager->HandleMessage(TRUE,KI_NO_HOST,0,(0));
		else
		if ((sock = client_connect(server, port)) < 0)
			Messager->HandleMessage(TRUE,KI_NO_CONNECT,errno,(2,server,MsgHdl::ltoa(port)));
	}
	while(server == NULL || sock < 0);

	return Connect(sock);
}

/****************************************************************************/

bool KConnection::ReConnect()
{
	if (socket != NULL)
	{
		DestroyLogs();
		SetLogo();
		SwitchContents();
	}
	
	return WinConnect();
}

/****************************************************************************/

bool KConnection::Connect()
{
	return Connect(NOT_CONNECTED);
}

/****************************************************************************/

bool KConnection::Connect(int sock)
{
	struct passwd *pw;

	if (socket != NULL)
		return FALSE;

	if (sock < 0)
	{
		if (server != NULL)
		{
			if ((sock = client_connect(server, port)) < 0)
			{
				Messager->HandleMessage(TRUE,KI_NO_CONNECT,errno,(2,server,MsgHdl::ltoa(port)));
				return FALSE;
			}
		}
		else
		{
			Messager->HandleMessage(TRUE,KI_NO_HOST,0,(0));
			return FALSE;
		}
	}

	if ((pw = getpwuid(getuid())) == NULL)
	{
		Messager->HandleMessage(TRUE,KI_NOBODY,0,(0));
		return FALSE;
	}

	if (add_socket (&socket, sock))
		Messager->HandleMessage(TRUE,KI_OUT_OF_MEMORY,0,(0));

	socket->msg = MSG_ANNOUNCE;
	msgcpy (socket, pw->pw_name, strlen (pw->pw_name) + 1);

	Write (socket);

	socket->waitstatus = WF_ACC;

	sn = new QSocketNotifier( sock, QSocketNotifier::Read, this);
	QObject::connect( sn, SIGNAL(activated(int)), SLOT(eval_message()) );
	// The next line is only a hack
	channels = 2;

	CreateLogs();

	return TRUE;
}

/****************************************************************************/

bool KConnection::SwitchContents()
{
	// The next 2 lines are only a hack
	QWidget::hide();
	QWidget::show();

	return TRUE;
}

/****************************************************************************/

bool KConnection::Disconnect()
{
	if (socket != NULL)
	{
		DestroyLogs();

		if (ConnectionNr == 1)
		{
			SetLogo();
			SwitchContents();
		}
		else
			close();
	}
	else
		close();

	return TRUE;
}

/****************************************************************************/

bool KConnection::CreateLogs()
{
	int i;
	chanwin		  = new KChannel[channels](this,this);
	channr			= new int[channels];
	QString Message;


	Message = KISDNLOG_NAME;
	Message += klocale->translate(": Connected to isdnlog on ");
	Message += server;
	setCaption(Message);

	for (i = 0; i < channels; i++)
		channr[i] = NO_CHAN;

  outwin = new QFrame(this);
	outwin->resize(size());
  outwin->setFrameStyle(QFrame::WinPanel | QFrame::Raised);

	split1		= new KSplit(this,outwin,KNewPanner::Horizontal,KNewPanner::Absolute,100);

	curcalls    = new KCurCalls(this,split1);
	oldcalls    = new KOldCalls(this,split1);
	logwin    = new KLogWin(this);

	Message = KISDNLOG_NAME;
	Message += klocale->translate(": Log from isdnlog on ");
	Message += server;
	logwin->setCaption(Message);

	QObject::connect(logwin,SIGNAL(destroyed()),SLOT(DestroyLogWin()));
	ShowLogWin();

	split1->activate(curcalls,oldcalls);

	if (qApp->desktop())
		setMaximumSize(qApp->desktop()->size());

  setMinimumWidth(channels * CHAN_X + 2 * BORDER_SIZE);
  setMinimumHeight(CHAN_Y + menu->height());

	if (ActiveRect.isValid())
		setGeometry(ActiveRect);

	menu->Connect(TRUE);

	if (logo != NULL)
	{
		delete logo;
		logo = NULL;
		SwitchContents();
	}
	else
		repaint();

	return TRUE;
}

/****************************************************************************/

bool KConnection::DestroyLogs()
{
	if (socket == NULL)
		return FALSE;

	delete sn;
	sn = NULL;

	channels = 0;
	delete[] chanwin;
	delete[] channr;

	chanwin = NULL;
	channr = NULL;

	delete curcalls;
	delete oldcalls;
	delete logwin;
	delete split1;
	delete outwin;

	curcalls = NULL;
	oldcalls = NULL;
	split1 = NULL;
	outwin = NULL;

	::close(socket->descriptor);
	del_socket(&socket,0);

	SetHost(NULL);

	menu->Connect(FALSE);

	return TRUE;
}

/****************************************************************************/

bool KConnection::SetLogo()
{
	QString Message;

	if (logo != NULL)
		return FALSE;

	logo = new KLogo(this,LOGO_XPM);

	Message = KISDNLOG_NAME;
	Message += klocale->translate(": Not connected");
	setCaption(Message);

	ActiveRect = geometry();

	if (Restore == FALSE)
		SetLogoSize();

	return TRUE;
}

/****************************************************************************/

void KConnection::SetLogoSize()
{
	if (logo != NULL)
	{
	  setFixedWidth(logo->width());
	  setFixedHeight(logo->height() + menu->height());
	}
}

/****************************************************************************/

bool KConnection::close(bool)
{
printf("close\n");
	if (ConnectionNr == 1)
		Quit();
	else
		return QWidget::close(TRUE);

	return TRUE;
}

/****************************************************************************/

void KConnection::show()
{
  QWidget::show();
  //logwin->setFocus();  
  //split1->setFocus();  
}

/****************************************************************************/

bool KConnection::ShowLogWin()
{
	if (logwin != NULL)
		if (menu->IsLogShowed() == TRUE)
			logwin->show();
		else
			logwin->hide();

	return TRUE;
}

/****************************************************************************/

void KConnection::DestroyLogWin()
{
printf("LogWin destroyed\n");
	logwin = NULL;
}

/****************************************************************************/

void KConnection::HideLogWin()
{
		menu->SetLogShowed(FALSE);
}

/****************************************************************************/

bool KConnection::SetLogWin()
{
	menu->SetLogShowed(!menu->IsLogShowed());
	return ShowLogWin();
}

/****************************************************************************/

bool KConnection::SaveLogFile()
{
	if (logwin != NULL)
		logwin->SaveToFile();

	return TRUE;
}

/****************************************************************************/

bool KConnection::StartApp(KApplication *app)
{
	KConnection *conn;


	if (app->isRestored())
	{
		int i = 0;

		Restore = TRUE;

		while (KTopLevelWidget::canBeRestored(++i))
		{
			if (!strcmp(KTopLevelWidget::classNameOfToplevel(i),"KConnection"))
			{
				if ((conn = new KConnection(app)) != NULL)
					conn->restore(i);
			}
		}

		Restore = FALSE;
	}

	if (ConnectionNr == 0)
	{
		QWidget *desktop = qApp->desktop();

		if ((conn = new KConnection(app)) != NULL)
		{
			int width = conn->width();
			int height = conn->height();

			conn->NewConnect();
			conn->setGeometry((desktop->width()-width)/2, (desktop->height()-height)/2,
		                  width, height);

			conn->show();
		}
	}

	return TRUE;
}

/****************************************************************************/

bool KConnection::ReadConfig()
{
	const char *Value;


	config->setGroup(KI_SEC_GLOBAL);

	Value = config->readEntry(KI_ENT_DIR);
	CurDir = Value?Value:".";

	Value = config->readEntry(KI_ENT_LOG_VISIBLE);
	logVisible = Value?atoi(Value):0;

	return TRUE;
}

/****************************************************************************/

bool KConnection::WriteConfig()
{
	config->setGroup(KI_SEC_GLOBAL);
	config->writeEntry(KI_ENT_DIR,CurDir);
	config->writeEntry(KI_ENT_LOG_VISIBLE,logVisible);

	if (curcalls != NULL)
		curcalls->WriteConfig();
	if (oldcalls != NULL)
		oldcalls->WriteConfig();

	return TRUE;
}

/****************************************************************************/

void KConnection::readProperties(KConfig *kc)
{
	const char *Value;


	SetHost((const char*) kc->readEntry(KI_ENT_SERVER));
	SetPort((const char*) kc->readEntry(KI_ENT_PORT));
	Value = (const char*) kc->readEntry(KI_ENT_LOG_VISIBLE);
	menu->SetLogShowed(Value?atoi(Value):0);

	ActiveRect = geometry();

	if (Connect() == FALSE)
		SetLogoSize();
}

/****************************************************************************/

void KConnection::saveProperties(KConfig *kc)
{
	kc->writeEntry(KI_ENT_SERVER,server);
	kc->writeEntry(KI_ENT_PORT,port);
	kc->writeEntry(KI_ENT_LOG_VISIBLE,menu->IsLogShowed());
}

/****************************************************************************/

void KConnection::updateRects()
{
	int i;
	int menutop = 0;
	int menubottom = 0;


	if (menu != NULL)
	{
		if (menu->menuBarPos() == KMenuBar::Top)
		{
			menutop = menu->height();
			menu->setGeometry(0,0,width(),menu->height());
		}
		else
		if (menu->menuBarPos() == KMenuBar::Bottom)
		{
			menubottom = menu->height();
			menu->setGeometry(0,height() - menu->height(),width(),menu->height());
		}
	}


	if (socket != NULL)
	{
		for (i = 0; i < channels; i++)
			chanwin[i].move(BORDER_SIZE + i * CHAN_X, menutop);

		if (outwin != NULL)
			outwin->setGeometry(BORDER_SIZE, CHAN_Y + menutop, width() - BORDER_SIZE*2, height() - CHAN_Y - BORDER_SIZE - menutop - menubottom);

		if (split1 != NULL)
			split1->SetSize();
	}
	else
	if (logo != NULL)
		logo->setGeometry(0,menutop,width(),height()-menubottom);

printf("dlg: x0: %d, y0: %d, x1: %d, y1: %d\n",this->x(),this->y(),this->width(),this->height());
}

/****************************************************************************/

KConfig *KConnection::GetConfig()
{
	return config;
}

/****************************************************************************/

const char *KConnection::GetHost()
{
	return server;
}

/****************************************************************************/

const char *KConnection::SetHost(const char *newserver)
{
	if (server != NULL)
	{
		free(server);
		server = NULL;
	}

	if (newserver != NULL && *newserver != '\0')
		server = strdup(newserver);
	else
		server = NULL;

	return server;
}

/****************************************************************************/

int KConnection::SetPort(int newport)
{
	if (newport == 0)
		newport = SERV_PORT;

	return (port = newport);
}

/****************************************************************************/

int KConnection::SetPort(const char *newport)
{
	if (newport == NULL || *newport == '\0')
		return SetPort(0);

	return SetPort(atoi(newport));
}

/****************************************************************************/

int KConnection::GetPort()
{
	return port;
}

/****************************************************************************/

QString &KConnection::GetCurDir()
{
	return CurDir;
}

/****************************************************************************/

bool KConnection::SetCurDir(QString NewDir)
{
	CurDir = NewDir;

	return TRUE;
}

/****************************************************************************/

bool KConnection::SetCurDir(const char *NewDir)
{
	CurDir = NewDir;

	return TRUE;
}

/****************************************************************************/

void KConnection::SetLogVisible(bool enable)
{
	logVisible = enable;
}

/****************************************************************************/

bool KConnection::eval_message()
{
	int     size;

	do
	{
		if   ((size = Read (socket)) <= 0) {
			Disconnect();
			Messager->HandleMessage(TRUE,KI_CONN_BROKEN,0,(0));
			return FALSE;
		}

		switch(socket->waitstatus)
		{
			case WF_ACC:
				switch (socket->msg)
				{
					case MSG_ANN_ACC:
						socket->servtyp = stoi((unsigned char *) socket->msgbuf.buf,_MSG_2B);
						socket->waitstatus = WF_NOTHING;
						break;
				
					case MSG_VERSION:
						/* Meldung: Protokoll-Versionsnummer vom Server */
						compare_version (socket->msgbuf.buf);
						break;

					case MSG_ANN_REJ:
						/* Meldung: nicht authorisierter Zugriff auf Server */
 	        	socket->msg = MSG_CLOSE;
 	        	socket->msgbuf.used = 0;
 	        	Write(socket);

						Messager->HandleMessage(TRUE,KI_ACC_DENIED,0,(0));

						Disconnect();
						return FALSE;
						break;

					default:
						/* Meldung: Unbekannter Message-Typ Msg */
						Messager->HandleMessage(TRUE,KI_UNKNOWN_MSG,0,(1,MsgHdl::ltoa(socket->msg)));
						break;
				}
				break;

			default:
				switch (socket->msg)
				{
					case MSG_CALLER:
						/* Eingabe eines Neuen Benutzers socket->msgbuf */
						/*if (socket->servtyp & T_ADDRESSBOOK)*/
printf("MSG_CALLER:%s\n",socket->msgbuf.buf);
						break;
				
					case MSG_WHO_IS:
						/* Eingabe eines Neuen Benutzers socket->msgbuf */
						//if (socket->servtyp & T_ADDRESSBOOK)
						//	isdn_new_caller_display (socket);
						break;
				
					case MSG_SERVER:
						/* Eingabe eines Neuen Benutzers socket->msgbuf */
						if (logwin != NULL)
							logwin->AppendLine(socket->msgbuf.buf);
						break;

					case MSG_CHANGE_CHAN:
	          /* Wechsel der aktuellen Kanalnummer des aktuellen Gespraeches */
						{
							int  newchan, oldchan;
							char fmt[SHORT_STRING_SIZE];

							sprintf(fmt,"%%d%c%%d",C_DELIMITER);

							if (sscanf(socket->msgbuf.buf,fmt,&oldchan,&newchan) != 2)
								Messager->HandleMessage(TRUE,KI_WRONG_RESULT,0,(0));
							else
								change_chan(oldchan,newchan);
						}
	 	      	break;
				
					case MSG_CALL_INFO:
						{
							int chan;
							CALL *Info = NULL;

							chan = Set_Info_Struct(&Info,socket->msgbuf.buf);

							if (chan == -1)
						  	return FALSE;

							if (Info == NULL)
							{
								Messager->HandleMessage(TRUE,KI_OUT_OF_MEMORY,0,(0));
						  	return FALSE;
						  }

							SetCallInfo(chan, Info);

							free(Info);
						}
						break;

					default:
						/* Meldung: Unbekannter Message-Typ Msg */
						Messager->HandleMessage(TRUE,KI_UNKNOWN_MSG2,0,(1,MsgHdl::ltoa(socket->msg)));
						break;
				}
				break;
		}
	}
	while  (socket->status == NEXT_MSG);

	return TRUE;
}

/****************************************************************************/

bool KConnection::compare_version (char *Version)
{
	if (strcmp(PROT_VERSION,Version))
	{
		Messager->HandleMessage(TRUE,KI_DIFF_VERSION,0,(2,Version,PROT_VERSION));

		return FALSE;
	}

	return TRUE;
}

/****************************************************************************/

int KConnection::new_chan(int newchan)
{
	int i;

	for (i = 0; i < channels && channr[i] != NO_CHAN; i++);

	if (i != channels)
	{
		channr[i] = newchan;
		chanwin[i].StartConnection();
	}
	else
		return NO_CHAN;

	return i;
}

/****************************************************************************/

int KConnection::get_chan(int chan)
{
	int i;

	for (i = 0; i < channels && channr[i] != chan; i++);

	if (i == channels)
		return NO_CHAN;

	return i;
}

/****************************************************************************/

int KConnection::change_chan(int chan, int changechan)
{
	int i;

	curcalls->change_chan(chan, changechan);

	for (i = 0; i < channels && channr[i] != chan; i++);

	if (i != channels)
		channr[i] = changechan;
	else
		return NO_CHAN;

	return i;
}

/****************************************************************************/

int KConnection::del_chan(int delchan)
{
	int index;
	curcalls->del_chan(delchan);

	if ((index = change_chan(delchan,NO_CHAN)) >= 0)
		chanwin[index].StopConnection();

	return index;
}

/****************************************************************************/

bool KConnection::SetCallInfo(int chan, const CALL *Info)
{
	int index;


	if (Info == NULL)
		return FALSE;

	switch (Info->stat)
	{
		case STATE_CONNECT:
		case STATE_BYTE   :
		case STATE_AOCD   :
		case STATE_TIME   :
			if (get_chan(chan) == NO_CHAN)
				new_chan(chan);
		case STATE_RING   :
			if (curcalls->get_chan(chan) == NO_CHAN)
				curcalls->new_chan(chan);
		case STATE_CAUSE  :
			curcalls->WriteLine(chan,Info);

			if ((index = get_chan(chan)) >= 0)
				chanwin[index].AddValue(Info->t_duration, Info->ibytes, Info->obytes);

			break;
		case STATE_HANGUP :
			del_chan(chan);
			break;
		default:
			Messager->HandleMessage(TRUE,KI_UNKNOWN_MSG3,0,(1,Info->stat));
	}

	if (Info->stat == STATE_HANGUP) // || Info->stat == STATE_CAUSE)
		oldcalls->AppendLine(Info);

	return TRUE;
}

/****************************************************************************/

