/* $Id: message.cpp,v 1.2 1998/05/10 23:40:12 luethje Exp $
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
 * $Log: message.cpp,v $
 * Revision 1.2  1998/05/10 23:40:12  luethje
 * some changes
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>

#include "message.h"

/******************************************************************************/

#define NUM_RET_STRINGS 30

/******************************************************************************/

MsgHdl::MsgHdl(int (*newfct)(const char*, int), const char* NewMsgFileName, const char* NewLogFile)
{
	if ((fct = newfct) == NULL)
		InternalError(1, "MsgHdl::MsgHdl: There is no message function!\n");

	LogFile     = NULL;
	MsgFileName = NULL;

	SetMsgFile(NewMsgFileName);

	if (NewLogFile != NULL)
	{
		LogFile = new fstream(NewLogFile,ios::out);

		if (!*LogFile)
		{
			InternalError(3, "MsgHdl::MsgHdl: Can't open logfile `%s': %s\n", NewLogFile, strerror(errno));
			delete LogFile;
			LogFile = NULL;
		}
	}
}

/******************************************************************************/

MsgHdl::~MsgHdl()
{
	if (MsgFileName != NULL)
		free(MsgFileName);

	if (LogFile != NULL)
	{
		LogFile->close();
		delete LogFile;
	}
}

/******************************************************************************/

int MsgHdl::SetMsgFile(const char *NewMsgFileName)
{
	if (MsgFileName != NULL)
	{
		free(MsgFileName);
		MsgFileName = NULL;
	}

	if (NewMsgFileName != NULL)
		MsgFileName = strdup(NewMsgFileName);

	return 0;
}

/******************************************************************************/

int MsgHdl::InternalError(int ErrNumber, const char *fmt, ...)
{
	auto va_list ap;
	static char Output[BUFSIZ];

	va_start(ap, fmt);
	vsnprintf(Output, BUFSIZ, fmt, ap);
	va_end(ap);

	if (fct != NULL)
		return fct(Output,ErrNumber);
	else
		fprintf(stderr, "%s %d: %s", ErrNumber < 0?"Error":"Warning", ErrNumber, Output);

	return (ErrNumber < 0?-1:1);
}

/******************************************************************************/

const char *MsgHdl::GetText(int ErrNumber, int *Cnt)
{
	static char Format[BUFSIZ];
	char FString[BUFSIZ];
	char dummy[BUFSIZ];
	int Number = 0;
	fstream	*MsgFile;


	Format[0] = '\0';

	if (MsgFileName == NULL)
	{
		*Cnt = noFile;
		return NULL;
	}

	if ((MsgFile = new fstream(MsgFileName,ios::in)) == NULL || !*MsgFile)
	{
		*Cnt = noFileHandle;
		return NULL;
	}

	while (*MsgFile && !Number) /* Bis das File ausgelesen ist */
	{
		MsgFile->getline(FString,BUFSIZ);
		if (sscanf(FString,"%d%[\t ]%d%[\t ]%[^\n]",&Number,dummy,Cnt,dummy,Format) != 5 ||
		    Number != ErrNumber)
			Number = 0;	
	}

	MsgFile->close();
	delete MsgFile;

	if (Number == 0)
		return NULL;
	else
		return Format;
}

/******************************************************************************/

const char **MsgHdl::List2Array(int Argc,...)
{
	int Cnt = 0;
	static const char *Ret_List[20];
       va_list ap;
       

       va_start(ap,Argc);
	for (Cnt = 0; Cnt < Argc; Cnt++)
       	Ret_List[Cnt] = va_arg(ap,char*);
       va_end(ap);

	Ret_List[Cnt] = NULL;

	return Ret_List;
}

/******************************************************************************/

#ifdef DEBUG
int MsgHdl::_HandleMessage( int Line, const char *FileName, int Condition, int ErrNumber, int Errno, const char *ErrText[] )
#else
int MsgHdl::_HandleMessage( int, const char *, int Condition, int ErrNumber, int Errno, const char *ErrText[] )
#endif /* DEBUG */
{
	char OutString[BUFSIZ];
	char Output[BUFSIZ] = "";
	const char *FormatString;
	int TextCnt = 0;
	int ParamCnt = 0;


	if (ErrNumber == 0)
		return 0;

	if (Condition)
	{
#ifdef DEBUG
//		time_t t;
//		time(&t);
//		ERROUT << ctime(&t);
		sprintf(Output,"%s:%d# ",FileName,Line);
#endif /* DEBUG */

		/* Fehler fuer fehlende Umgebungsvariable muss gesondert 
		   behandelt werden, weil die Datei error.list noch nicht
		  gefunden werden kann. */
//		if (ErrNumber == 99)
//			sprintf(Output,"%sEnvironment variable `%s' is not set!\n", Output, ErrText[0]);
//		else
		{
			FormatString =  GetText(ErrNumber,&ParamCnt);
		
			while(ErrText[TextCnt] != NULL) TextCnt++;

			if (FormatString != NULL)
			{
				if (ParamCnt == TextCnt)
				{
					MsgHdl::FormatString(OutString,FormatString,ErrText);

					if (Errno)
						sprintf(Output,"%s%s: %s\n", Output, OutString, strerror(Errno));
					else
						sprintf(Output,"%s%s\n", Output, OutString);
				}
				else
					sprintf(Output,"%sHsgHdl::HandleMessage: Wrong number of ErrorText expected %d, got %d %s!\n",Output,ParamCnt,TextCnt,ValueString(ErrText));
			}
			else
			{
				switch(ParamCnt)
				{
				case noFile:
					sprintf(Output,"%sHsgHdl::GetText: There is no message file %s!\n", Output, ValueString(ErrText));
					break;
				case noFileHandle:
					sprintf(Output,"%sHsgHdl::GetText: Can't open message file `%s' %s: %s!\n", Output, MsgFileName,ValueString(ErrText), strerror(errno));
					break;
				case unknownErr:
					sprintf(Output,"%sHsgHdl::GetText: An unknown error is occoured %s!\n", Output, ValueString(ErrText));
					break;
				default:
					sprintf(Output,"%sHsgHdl::HandleMessage: unknown message number %d %s!\n", Output, ErrNumber,ValueString(ErrText));
					break;
				}
			}
		}

		if (LogFile)
			(*LogFile) << Output;

		if (fct == NULL)
			return InternalError(ErrNumber, "%s", Output);
		else
			return fct(Output,ErrNumber);

	}

	return 0;
}

/******************************************************************************/

void MsgHdl::FormatString (char *OutString, const char *FormatString, const char *ErrText[])
{
	char FS[BUFSIZ];
	char *Ptr = FS;

	strcpy(FS,FormatString);

	FormatString = FS;

	*OutString = '\0';

	while ((Ptr = strstr(Ptr,"\\n")) != NULL)
	{
		*Ptr = '\n';
		memmove(Ptr+1,Ptr+2,strlen(Ptr)-1);
	}

	while (*ErrText != NULL && (Ptr = strstr(FormatString,"%s")) != NULL)
	{
		*Ptr = '\0';
		strcat(OutString,FormatString);
		strcat(OutString,*ErrText);
		ErrText++;
		FormatString = Ptr+2;
	}
	strcat(OutString,FormatString);

#ifdef DEBUG
	if (*ErrText != NULL || strstr(FormatString,"%s") != NULL)
		InternalError(3,"MsgHdl::FormatString: numbers of parameters not equal!\n");
#endif DEBUG
}

/****************************************************************************/

const char *MsgHdl::ValueString(const char *ErrText[])
{
	static char RetCode[BUFSIZ];


	if (*ErrText != NULL)
	{
		strcpy(RetCode, "(values: ");

		while (*ErrText != NULL)
		{
			strcat(RetCode, *ErrText);

			if (*(++ErrText) != NULL)
				strcat(RetCode, ", ");
		}

		strcat(RetCode, ")");
	}
	else
		strcpy(RetCode, "(no values)");


	return RetCode;
}

/****************************************************************************/

const char *MsgHdl::ltoa(long Value)
{
	static char RetCode[NUM_RET_STRINGS][11];
	static int  Index = 0;

	Index = (Index + 1) % NUM_RET_STRINGS;
	sprintf(RetCode[Index], "%ld", Value);

	return RetCode[Index];
}

/****************************************************************************/

const char *MsgHdl::dtoa(double Value, int Pre, int Post)
{
	static char RetCode[NUM_RET_STRINGS][25];
	static int  Index = 0;

	Pre = Pre < 0 ? 0 : (Pre > 22 ? 22 : Pre);
	Post= Post < 0 ? 0 : Post;

	Index = (Index + 1) % NUM_RET_STRINGS;
	sprintf(RetCode[Index], "%*.*f", Pre, Post, Value);

	return RetCode[Index];
}

/****************************************************************************/

