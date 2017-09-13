/* $Id: kisdnlog.h,v 1.2 1998/05/10 23:40:06 luethje Exp $
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
 * $Log: kisdnlog.h,v $
 * Revision 1.2  1998/05/10 23:40:06  luethje
 * some changes
 *
 */

#ifndef KISDNLOG_H
#define KISDNLOG_H

/****************************************************************************/

#include <time.h>

#include <qwidget.h>
#include <qframe.h>
#include <qdict.h>
#include <qmlined.h>
#include <qstring.h>
#include <qpainter.h>
#include <qdialog.h>
#include <qsocknot.h> 
#include <qvalidator.h>
#include <qlist.h>

#include <ktopwidget.h>
#include <ktablistbox.h>
#include <knewpanner.h>
#include <kpopmenu.h>
#include <kapp.h>

#include "config.h"

extern "C" {
#include "socket.h"
}

#include "message.h"
#include "messagenr.h"

#undef INFORMATION

/****************************************************************************/

#define KISDNLOG_NAME		PACKAGE

#define DEV_SL          "Stefan Luethje (luethje@sl-gw.lake.de)"
#define DEV_CW          "Claudia Weber (weber@sl-gw.lake.de)"
#define LOGO_XPM				"isdnlog.xpm"

#define BORDER_SIZE			10
#define MAX_LOG_LINES		1000
#define MAX_CALLS_LINES	200

#define CALLS_ROWS			3
#define CALLS_COLS			(PROT_ELEMENTS-1)

#define IN_DATA					0
#define OUT_DATA				1

#define LOGO_X					320
#define LOGO_Y					240

#define THRU_X					100
#define THRU_Y					90

#define CHAN_X					205
#define CHAN_Y					155

#define MAX_RATE				8000

#define GET_BPS					2
#define NO_DIR					4

#define NO_CHAN					-1
#define NEW_CHAN				-2

#define THRU_TIMEOUT		20
#define CALLS_TIMEOUT		120
#define	KTIMERSEC				1000

#define NOT_CONNECTED		-1

#define	KI_SEC_HOSTS				"Hosts"
#define	KI_ENT_HOSTS				"Hosts"
#define	KI_ENT_PORTS				"Ports"
#define	KI_ENT_LAST_HOST		"LastPort"
#define	KI_ENT_LAST_PORT		"LastHost"

#define	KI_SEC_GLOBAL				"Global"
#define	KI_ENT_DIR					"Directory"
#define	KI_ENT_SERVER				"Server"
#define	KI_ENT_PORT					"Port"
#define	KI_ENT_LOG_VISIBLE	"LogVisible"
#define	KI_ENT_CUR_CALLS		"CurCalls"
#define	KI_ENT_OLD_CALLS		"OldCalls"

/****************************************************************************/

typedef struct {QString login; QString command;} hostParam;

/****************************************************************************/

class KMsgHdl;
class KConnection;

/****************************************************************************/

#ifdef _FUNCTIONS_CPP_
#define _EXTERN
#else
#define _EXTERN extern
#endif

_EXTERN const char *Byte2Str(double Bytes, int flag);

#undef _EXTERN


#ifdef _KISDNLOG_MAIN_C_
#define _EXTERN
#define _IS_NULL = NULL
#else
#define _EXTERN extern
#define _IS_NULL
#endif

_EXTERN KMsgHdl *Messager _IS_NULL;

#undef _EXTERN
#undef _IS_NULL

/****************************************************************************/

class KMsgHdl : public MsgHdl
{
	private:

	public:
		KMsgHdl::KMsgHdl(KConfig * = NULL);
		KMsgHdl::~KMsgHdl();
};

/****************************************************************************/

class KLogo : public QFrame
{
	Q_OBJECT

	private:
		QPixmap pic;
		char   *pixmap;

	public:
		KLogo(QWidget *, const char*);
		~KLogo();

	protected:
		virtual void paintEvent(QPaintEvent*);
};

/****************************************************************************/

class KMenu : public KMenuBar
{
	Q_OBJECT

	private:
		KConnection  *mainwin;

		QPopupMenu *fileMenu;
		QPopupMenu *winMenu;
		QPopupMenu *helpMenu;

		int showlog;
		int writefile;
		int reconnect;
		int disconnect;

	public:
		KMenu::KMenu(KConnection *, bool);
		KMenu::~KMenu();

		bool IsLogShowed();
		void SetLogShowed(bool);
		bool Connect(bool = TRUE);
};

/****************************************************************************/

class KHost : public QDialog
{
	Q_OBJECT

	private:
		KConnection  *mainwin;
		QFrame		*frame;
		QComboBox *hostCombo;
		QComboBox *portCombo;
		QLabel    *hostLabel;
		QLabel    *portLabel;

		QPushButton *ok;
		QPushButton *cancel;
		QPushButton *help;

		QValidator *portValid;

	private:
		bool ReadConfig();
		bool WriteConfig();

	private slots:
		void hostChanged(int);
		void portChanged(int);
		void go();
		void Quit();
		void showHelp();

	public:
		KHost(KConnection*);
		~KHost();

		virtual void show();

	protected:
		virtual void resizeEvent(QResizeEvent*);
};

/****************************************************************************/

class KThruput : public QFrame
{
	Q_OBJECT

	public:
		enum {ALL_AVERAGE = -2, CUR_AVERAGE = -1, COUNT = 1};

	private:
		KConnection  *mainwin;
		QWidget *frame;
		QColor PenColor[2];
		QColor BGColor;

		int ValueSize;
		int *Values[2];
		int *RunTime;
		double MinRate[2];
		double MaxRate[2];
		int TimeScale;
		int Connected;
		bool NotConnected;

	private:
		bool AddValue(int, unsigned long);
		bool ShiftArray(int *);
		bool DrawRate(QPainter &, int);
		int CurIndex(int = 0);

	private slots:

	public:
		KThruput(KConnection* , QWidget *newframe, int = 1);
		~KThruput();

		virtual void paintEvent(QPaintEvent*);

		bool AddValue(int, unsigned long, unsigned long);
		bool SetTimeScale(int);
		double GetRate(int, int = -1);
		double GetMinRate(int);
		double GetMaxRate(int);
		int GetTraffic(int);
		QColor &GetColor(int);

		bool StartConnection();
		bool StopConnection();
		const char *DeleteConnection();
};

/****************************************************************************/

class KChannel : public QFrame
{
	Q_OBJECT

	private:
		KConnection  *mainwin;
		QWidget  *frame;
		KThruput *thru;

		QTimer   *Timer;

	private slots:
		const char *DeleteConnection();

	public:
		KChannel(KConnection* , QWidget *newframe, int = 1);
		~KChannel();

		virtual void paintEvent(QPaintEvent*);

		bool AddValue(int, unsigned long, unsigned long);
		bool SetTimeScale(int);
		double GetRate(int, int = -1);
		double GetMinRate(int);
		double GetMaxRate(int);
		int GetTraffic(int);
		bool StartConnection();
		bool StopConnection();
};

/****************************************************************************/

class KSplit : public KNewPanner
{
	Q_OBJECT

	private:
		KConnection  *mainwin;
		QWidget *frame;

	public:
		KSplit(KConnection* , QWidget *newframe, enum KNewPanner::Orientation, enum KNewPanner::Units, int);
		~KSplit();
		void SetSize();
};

/****************************************************************************/

class KCalls : public KTabListBox
{
	Q_OBJECT

	private:
		KConnection  *mainwin;
		QWidget      *frame;

	protected:
		bool WriteHeader(const char *, int);
		bool SetHeader(void);
		bool SetLine(int, const CALL *);
		const char *timetoa(time_t);
		const char *durationtoa(time_t);
		const char *directiontoa(int);
		const char *statetoa(int);
		const char *emptytoa(const char *);
		bool WriteConfig(const char *EntName);
		bool ReadConfig(const char *EntName);

	public:
		KCalls(KConnection* , QWidget *newframe);
		~KCalls();
};

/****************************************************************************/

class KCurCalls : public KCalls
{
	Q_OBJECT

	private:
		typedef struct {int chan; int stat; time_t time;} chan_struct;

	private:
		chan_struct  *channr;

		QTimer       *Timer;

	private slots:
		bool ClearLines();

	public:
		KCurCalls(KConnection* , QWidget *newframe);
		~KCurCalls();

		bool WriteConfig();
		bool WriteLine(int, const CALL*);
		int  new_chan(int);
		int  get_chan(int);
		int  change_chan(int, int);
		int  del_chan(int);
};

/****************************************************************************/

class KOldCalls : public KCalls
{
	private:
		int maxlines;

	public:
		KOldCalls(KConnection* , QWidget *newframe, int = MAX_CALLS_LINES);
		~KOldCalls();

		bool WriteConfig();
		bool SetMaxLines(int);
		bool AppendLine(const CALL*);
};

/****************************************************************************/

class KLog : public QMultiLineEdit
{
	Q_OBJECT

	private:
		KConnection  *mainwin;
		QWidget *frame;
		int maxlines;

	public:
		KLog(KConnection*, QWidget *, int = MAX_LOG_LINES);
		~KLog();

		bool SetMaxLines(int);
		void AppendLine(char *);
		bool SaveToFile(const char *Name = NULL);
};


/****************************************************************************/

class KLogWin : public KTopLevelWidget 
{
	Q_OBJECT

	private:
		KConnection  *mainwin;
		KLog         *log;

	protected:
		virtual void updateRects();

	public:
		KLogWin(KConnection*, int newmaxlines = MAX_LOG_LINES);
		~KLogWin();

		bool SetMaxLines(int);
		void AppendLine(char *);
		virtual void hide();

	public slots:
		void Quit();
		bool SaveToFile(const char *Name = NULL);
};
/****************************************************************************/

class KConnection : public KTopLevelWidget
{
	Q_OBJECT

	protected:
		static KConfig*  config;

	private:
		static int ConnectionNr;
		static int logVisible;
		static bool Restore;
		static QString CurDir;
		static QList<KConnection> ConnectList;

		KApplication    *app;
		QSocketNotifier *sn;
		socket_queue    *socket;

		KLogo     *logo;

		KMenu     *menu;
		KHost     *hostwin;
		KChannel  *chanwin;
		QFrame    *outwin;
		KSplit    *split1;
		KLogWin   *logwin;
		KCurCalls *curcalls;
		KOldCalls *oldcalls;

		QRect  ActiveRect;

		int *channr;
		int channels;

		char *server;
		int	  port;
		
	private:
		bool ShowLogWin();

		bool WinConnect();
		bool Connect(int);

		bool ReadConfig();
		bool WriteConfig();

	private slots:
		bool eval_message();
		void DestroyLogWin();

	public slots:
		void Quit();
		bool NewConnect();
		bool ReConnect();
		bool Disconnect();
		bool SetLogWin();
		bool SaveLogFile();

	protected:
		virtual void updateRects();
		virtual void saveProperties(KConfig *);
		virtual void readProperties(KConfig *);

		bool CreateLogs();
		bool DestroyLogs();
		bool SetLogo();
		void SetLogoSize();
		bool SwitchContents();
		bool compare_version (char *);
		bool SetCallInfo(int, const CALL *);
		int  new_chan(int);
		int  get_chan(int);
		int  change_chan(int, int);
		int  del_chan(int);

	public:
		KConnection(KApplication*, const char * = NULL, const char * = NULL);
		~KConnection();

		virtual void show();
		virtual bool close(bool = TRUE);

		void HideLogWin();

		const char *SetHost(const char *);
		const char *GetHost();
		int SetPort(int);
		int SetPort(const char*);
		int GetPort();
		KConfig *GetConfig();

		QString &GetCurDir();
		bool SetCurDir(QString);
		bool SetCurDir(const char *);

		void SetLogVisible(bool);
		bool Connect();

		static bool StartApp(KApplication *);

	signals:
		void quit();
};

/****************************************************************************/

#endif
