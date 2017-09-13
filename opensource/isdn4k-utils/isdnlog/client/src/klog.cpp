/* $Id: klog.cpp,v 1.3 1999/05/23 14:34:40 luethje Exp $
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
 * $Log: klog.cpp,v $
 * Revision 1.3  1999/05/23 14:34:40  luethje
 * kisdnlog is ready for isdnlog 3.x and kde 1.1.1
 *
 * Revision 1.2  1998/05/10 23:40:07  luethje
 * some changes
 *
 */

#include <errno.h>

#include <qmlined.h>
#include <qfiledlg.h>
#include <qdir.h>

#include <kmsgbox.h>

#include "kisdnlog.h"

/****************************************************************************/

KLog::KLog(KConnection *newmainwin, QWidget *newframe, int newmaxlines) :
QMultiLineEdit(newframe)
{
	mainwin = newmainwin;
	frame = newframe;
	SetMaxLines(newmaxlines);

	setReadOnly(TRUE);
}

/****************************************************************************/

KLog::~KLog()
{
}

/****************************************************************************/

bool KLog::SetMaxLines(int newmaxlines)
{
	if (newmaxlines < 1)
	{
		Messager->HandleMessage(TRUE,KI_INVALID_LINES,0,
		                        (2,klocale->translate("Log Window"),
		                         MsgHdl::ltoa(newmaxlines)));

		maxlines = MAX_LOG_LINES;

		return FALSE;
	}

	maxlines = newmaxlines;

	while (maxlines < numLines())
		removeLine(0);

	return TRUE;
}

/****************************************************************************/

void KLog::AppendLine(char *Line)
{
	if (maxlines <= numLines())
		removeLine(0);

	insertLine(Line);
	setTopCell(numLines()-1);
}

/****************************************************************************/

bool KLog::SaveToFile(const char *Name)
{
	int i;
	const char *text;
	FILE *fp = NULL;
	QString FileName;
	const char *Modus = "w";

	if (Name == NULL)
	{
		QFileDialog FDlg;
		char DirName[BUFSIZ];

		if (( FileName = FDlg.getSaveFileName(mainwin->GetCurDir())) == NULL)
			return FALSE;
		
		QFileInfo FileInfo = QFileInfo(FileName);

		strcpy(DirName, FileInfo.dirPath(TRUE));

		if (!access(DirName,W_OK))
			mainwin->SetCurDir(DirName);
		else
		{
			Messager->HandleMessage(TRUE,KI_NO_DIR,errno,(1,DirName));
			return FALSE;
		}
	}
	else
		FileName = Name;

	if (!access(FileName,R_OK))
	{
		char MsgTxt[BUFSIZ];

		sprintf(MsgTxt,
		        klocale->translate("file `%s' exists.\nDo you want to overwrite, or append it?\n"),
		        (const char*) FileName);
		
		KMsgBox MsgBox(mainwin,klocale->translate("Warning"),
		               MsgTxt,
		               KMsgBox::EXCLAMATION,
		               klocale->translate("Overwrite"),
		               klocale->translate("Append"),
		               klocale->translate("Cancel"));

		switch(MsgBox.exec())
		{
			case 1:	 Modus = "w";
			         break;
			case 2:	 Modus = "a";
			         break;
			case 3:	 
			default: return FALSE;
			         break;
		}
	}

	if ((fp = fopen(FileName,Modus)) == NULL)
	{
		Messager->HandleMessage(TRUE,KI_NO_FILE,errno,(1,(const char*)FileName));
		return FALSE;
	}

	for (i = 0; i < numLines(); i++)
		if ((text = textLine(i)) != NULL)
		{
			fputs(text,fp);
			fputs("\n",fp);
		}

	fclose(fp);

	return TRUE;
}

/****************************************************************************/

