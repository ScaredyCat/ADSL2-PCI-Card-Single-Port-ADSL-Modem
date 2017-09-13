/* $Id: start_prog.c,v 1.16 2000/04/13 15:44:20 paul Exp $
 *
 * ISDN accounting for isdn4linux.
 *
 * Copyright 1996 by Michael 'Ghandi' Herold,
 *                   Stefan Luethje (luethje@sl-gw.lake.de)
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
 * $Log: start_prog.c,v $
 * Revision 1.16  2000/04/13 15:44:20  paul
 * Fix for $5, $7, $8, $9, $10, always having same value as $11
 *
 * Revision 1.15  1999/11/03 17:54:13  paul
 * Fixed empty lines in syslog if program could not be started.
 *
 * Revision 1.14  1999/10/25 18:33:15  akool
 * isdnlog-3.57
 *   WARNING: Experimental version!
 *   	   Please use isdnlog-3.56 for production systems!
 *
 * Revision 1.13  1998/11/21 14:03:39  luethje
 * isdnctrl: added dialmode into the config file
 *
 * Revision 1.12  1998/10/22 18:22:43  luethje
 * isdnrep: suppress some messages
 * isdnlog: remove function Pathfind()
 *
 * Revision 1.11  1998/10/13 22:17:15  luethje
 * isdnlog: evaluate the variable PATH for program starts.
 *
 * Revision 1.10  1997/06/22 23:03:28  luethje
 * In subsection FLAGS it will be checked if the section name FLAG is korrect
 * isdnlog recognize calls abroad
 * bugfix for program starts
 *
 * Revision 1.9  1997/06/15 23:49:38  luethje
 * Some new variables for the isdnlog
 * isdnlog starts programs noe with the file system rights
 * bugfixes
 *
 * Revision 1.8  1997/05/28 22:03:10  luethje
 * some changes
 *
 * Revision 1.7  1997/05/28 21:22:58  luethje
 * isdnlog option -b is working again ;-)
 * isdnlog has new \$x variables
 * README completed
 *
 * Revision 1.6  1997/05/04 20:19:50  luethje
 * README completed
 * isdnrep finished
 * interval-bug fixed
 *
 * Revision 1.5  1997/04/16 22:22:51  luethje
 * some bugfixes, README completed
 *
 * Revision 1.4  1997/04/15 22:37:10  luethje
 * allows the character `"' in the program argument like the shell.
 * some bugfixes.
 *
 * Revision 1.3  1997/04/10 23:32:19  luethje
 * Added the feature, that environment variables are allowed in the config files.
 *
 * Revision 1.2  1997/04/03 22:58:34  luethje
 * some primitve changes.
 *
 * Revision 1.1  1997/03/16 20:58:55  luethje
 * Added the source code isdnlog. isdnlog is not working yet.
 * A workaround for that problem:
 * copy lib/policy.h into the root directory of isdn4k-utils.
 *
 * Revision 2.3.27  1996/05/05  20:35:46  akool
 *
 * Revision 2.21  1996/03/13  11:58:46  akool
 */


#define _START_PROG_C_
#include "isdnlog.h"
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>

/*************************************************************************/

#define C_SET_TAB   1
#define C_SET_SPACE 2

#define SET_BEGIN_VAR 1

/*************************************************************************/

#define C_TAB   '\t'
#define C_SPACE ' '
#define C_AT    '@'

#define S_AT    "@"

/*************************************************************************/

static interval *RootIntervall = NULL;

/** Prototypes ***********************************************************/

static void		KillCommand(int);
static int		GetArgs(char *, char *[], char *[], int);
static interval *Next_Interval(void);
static int set_user(char *User, char *File);
static int set_group(char *Group, char *File);
static char *StrToArg(char* string);
static char *Replace_Opts(char *String, char *Opts[], int MaxOpts);
static char *ArgToChar(int type, void* Ptr);
char **Get_Opts(int chan, int event, int InOut);

/****************************************************************************/

static int set_user(char *User, char *File)
{
	struct passwd* Ptr = NULL;
	struct stat   filestat;


	if (User != NULL && User[0] != '\0')
	{
		setpwent();

		while ((Ptr = getpwent()) != NULL)
		{
			if (!strcmp(Ptr->pw_name,User) || (isdigit(*User) && atoi(User) == (int) Ptr->pw_uid))
			{
				endpwent();
				print_msg(PRT_DEBUG_RING, "New user is %d set by USER\n",(int) Ptr->pw_uid);
				return setuid(Ptr->pw_uid);
			}
		}

		endpwent();
	}

	if (!stat(File,&filestat))
	{
		print_msg(PRT_DEBUG_RING, "New user is %d set by filestat\n",(int) filestat.st_uid);
		return setuid(filestat.st_uid);
	}

	return 0;
}

/****************************************************************************/

static int set_group(char *Group, char *File)
{
	struct group* Ptr = NULL;
	struct stat   filestat;


	if (Group != NULL && Group[0] != '\0')
	{
		setgrent();

		while ((Ptr = getgrent()) != NULL)
		{
			if (!strcmp(Ptr->gr_name,Group) || (isdigit(*Group) && atoi(Group) == (int) Ptr->gr_gid))
			{
				endgrent();
				print_msg(PRT_DEBUG_RING, "New group is %d set by GROUP\n",(int) Ptr->gr_gid);
				return setgid(Ptr->gr_gid);
			}
		}

		endgrent();
	}

	if (!stat(File,&filestat))
	{
		print_msg(PRT_DEBUG_RING, "New group is %d set by filestat\n",(int) filestat.st_gid);
		return setgid(filestat.st_gid);
	}

	return 0;
}

/*************************************************************************
 ** Ring(-) - Externes Kommando ausfuehren oder loeschen. Bei einem Fehl-	    **
 **			  er wird -1 zurueckgegeben; ansonsten die PID des neuen    **
 **			  Prozesses. Wenn das Kommando mit system() aufgerufen wur- **
 **			  de (Async), wird -1 zurueckgegeben, da die PID nicht ge-  **
 **			  sichert werden muss.					    **
 *************************************************************************
 ** Die  : <= 0   Kommando ausfuehren.						    **
 **        >  0   PID killen.							    **
 ** Async: == 1   Kommando mit system() starten.				    **
 **        == 0   Kommando mit eigenem Prozess starten.				    **
 *************************************************************************/

int Ring(info_args *Cmd, char *Opts[], int Die, int Async)
{
	char  Command[SHORT_STRING_SIZE + 1];
	char  String[LONG_STRING_SIZE];
	int   filedes[2];
	char *Args[64];
	pid_t pid;
	FILE *fp;

	print_msg(PRT_DEBUG_EXEC, "Ring: Cmd: `%s' Die: %d Async: %d\n", (Cmd&&Cmd->infoarg)?Cmd->infoarg:"NO COMMAND", Die, Async);

	if (Die <= 0)
	{
		sprintf(Command,"%.*s",SHORT_STRING_SIZE-6,Cmd->infoarg);


			/* Kommando soll gestartet werden - 'Async' gibt an, ob dieses	*/
			/* mit system() oder mit einem eigenen Prozeß aufgerufen wer-	*/
			/* den soll.																	*/

		if (GetArgs(Command,Args,Opts,64) == -1)
		{
			print_msg(PRT_ERR, "Can't start \"%s\": Invalid arguments\n", Args[0]);
			return -1;
		}

		pipe(filedes);
		switch (pid = fork())
		{
			case -1: print_msg(PRT_ERR, "%s\n", "Can't start fork()!");
			         return 0;
			         break;
			case  0: if (set_group(Cmd->group, Args[0]) < 0)
			         {
			           print_msg(PRT_ERR, "Can not set group %s: %s\n",Cmd->group,strerror(errno));
			           exit(-1);
			         }

			         if (set_user(Cmd->user, Args[0]) < 0)
			         {
			           print_msg(PRT_ERR, "Can not set user %s: %s\n",Cmd->user,strerror(errno));
			           exit(-1);
			         }

			         if (paranoia_check(Args[0]) < 0)
			           exit(-1);

			         dup2(filedes[1],STDOUT_FILENO);
			         dup2(filedes[1],STDERR_FILENO);

/*			         execvp(Pathfind(Args[0],NULL,NULL), Args);*/
			         execvp(Args[0], Args);
			         print_msg(PRT_ERR, "Can't start \"%s\" with execvp(): %s\n", Args[0],strerror(errno));
			         /* Alarm(); */
			         exit(-1);
			         break;
		}

		fp = fdopen(filedes[0],"r");
		close(filedes[1]);

                print_msg(PRT_DEBUG_INFO, "Program \"%s\" \"%s\" started %ssynchronous.\n", Args[0], Args[1], (Async?"a":""));

		if (Async == 1)
		{
			while(fgets(String,LONG_STRING_SIZE,fp) != NULL)
				if (!feof(fp) || String[0])
					print_msg(PRT_PROG_OUT,"%s\n",String);

			waitpid(pid,NULL,0);
			fclose(fp);
			return(0);
		}
		else
		{
			int sock;

			if (add_socket(&sockets,filedes[0]))
				return NO_MEMORY;

			sock = socket_size(sockets)-1;
			sockets[sock].fp  = fp;
			sockets[sock].pid = pid;

			return sock;
		}
	}
	else
	{
		KillCommand(Die);
		return 0;
	}

	return(-1);
}

/*************************************************************************
 ** GetArgs(-) - Zerlegt eine Kommandozeile in einzelne Argumente.		**
 *************************************************************************/

static int GetArgs(char *Line, char *Args[], char *Opts[], int MaxArgs)
{
	char	*Arg	= NULL;
	char	*Use	= Line;
	char  *Ptr  = NULL;
	char  *Org_Arg;
	int	 MaxOpts= 0;
	int	 i		= 0;
	int	 j		= 0;
	char HelpString[SHORT_STRING_SIZE];
	static char **MemPtr = NULL;

	if (MemPtr != NULL)
	{
		while(MemPtr[j] != NULL)
			free(MemPtr[j++]);

		free(MemPtr);
		MemPtr = NULL;
		j = 0;
	}

	while (Opts[MaxOpts] != NULL)
		MaxOpts++;

	while ((Org_Arg = Arg = StrToArg(Use)))
	{
		Use = NULL;

		if ((Ptr = Replace_Opts(Arg,Opts,MaxOpts)) != NULL)
		{
			Arg = strdup(Ptr);

			MemPtr = (char**) realloc(MemPtr,sizeof(char*)*(j+2));
			MemPtr[j++] = Arg;
			MemPtr[j] = NULL;
		}

		if (*Arg == C_AT)
		{
			FILE *fp = fopen(Arg+1,"r");

			if (fp != NULL)
			{
				fgets(HelpString,SHORT_STRING_SIZE,fp);

				if (*HelpString != '\0')
					HelpString[strlen(HelpString)-1] = '\0';

				Arg = strdup(HelpString);

				MemPtr = (char**) realloc(MemPtr,sizeof(char*)*(j+2));
				MemPtr[j++] = Arg;
				MemPtr[j] = NULL;
				fclose(fp);
			}
			else
				Arg = NULL;
		}

		if (Arg == NULL || *Arg == '\0')
		{
			if (Arg == NULL)
				print_msg(PRT_WARN,"Invalid argument `%s' for program start!\n",Org_Arg);

			Arg = "?";
		}

		Ptr = Arg;
		while((Ptr = Check_Quote(Ptr, S_AT, QUOTE_DELETE)) != NULL && Ptr[0] != '\0')
			Ptr++;

		if (i < MaxArgs) Args[i++] = Arg;
	}

	Args[i] = NULL;

	return(i);
}

/*************************************************************************/

static char *StrToArg(char* string)
{
	static char *Ptr = NULL;
	int in = 0;
	int begin = 1;
	char *Start;

	if (string != NULL)
	{
		Ptr = string;
		begin = 1;
	}

	Start = Ptr;

	if (Ptr == NULL)
		return NULL;

	while(*Ptr != '\0')
	{
		if (*Ptr == '\"')
		{
			if (begin != 1 && Ptr[-1] == C_QUOTE_CHAR)
			{
				memmove(Ptr-1,Ptr,strlen(Ptr)+1);
				Ptr--;
			}
			else
			{
				in = !in;
				memmove(Ptr,Ptr+1,strlen(Ptr));
				Ptr--;
			}
		}
		else
		if (!in && isspace(*Ptr))
		{
			*Ptr++ = '\0';
			break;
		}

		begin = 0;
		Ptr++;
	}

	if (in)
		print_msg(PRT_WARN,"Warning: Missing second char `\"'! in string `%s'!\n",Start);

	if (*Start == '\0')
		Start = NULL;

	return Start;
}

/****************************************************************************/

static char *Replace_Opts(char *String, char *Opts[], int MaxOpts)
{
	static char *RetCode = NULL;
	char *Begin = NULL;
	char *Var = NULL;
	char *End = NULL;
	char *Ptr = String;
	int cnt = 0;
	int num = 0;
	int Num = 0;


	if (Opts == NULL)
		return String;

	while ((Ptr = strchr(Ptr,C_BEGIN_VAR)) != NULL)
	{
		cnt++;
		Ptr++;
	}

	if (!cnt)
		return String;

	if (RetCode != NULL)
		free(RetCode);

	if ((RetCode = strdup(String))  == NULL ||
	    (Var     = strdup(RetCode)) == NULL ||
	    (End     = strdup(RetCode)) == NULL   )
	{
		print_msg(PRT_ERR,"%s!\n","Error: Can not allocate memory!\n");
		return NULL;
	}

	while ((Ptr = strchr(RetCode,C_BEGIN_VAR)) != NULL)
	{
		if (Ptr != RetCode && Ptr[-1] == C_QUOTE_CHAR)
		{
			*Ptr = SET_BEGIN_VAR;
			memmove(Ptr-1,Ptr,strlen(RetCode)-(Ptr-RetCode-1));
			cnt--;
		}
		else
		if ((num = sscanf(Ptr+1,"%[0-9]%[^\n]",Var,End))   >= 1 ||
		    (num = sscanf(Ptr+1,"{%[0-9]}%[^\n]",Var,End)) >= 1   )
		{
			if ((Num = atoi(Var)) > 0 && Num <= MaxOpts)
			{
				free(Begin);

				if ((Begin = strdup(RetCode)) == NULL)
				{
					print_msg(PRT_ERR,"%s!\n","Error: Can not allocate memory!\n");
					return NULL;
				}

				Begin[Ptr-RetCode] = '\0';

				if ((RetCode = (char*) realloc(RetCode,sizeof(char)*strlen(RetCode)+strlen(Opts[Num-1])-strlen(Var))) == NULL)
				{
					print_msg(PRT_ERR,"%s!\n","Error: Can not allocate memory!\n");
					return NULL;
				}

				if (num == 1)
					*End = '\0';

				sprintf(RetCode,"%s%s%s",Begin,Opts[Num-1],End);

				free(Var);
				free(End);

				if ((Var   = strdup(RetCode)) == NULL ||
				    (End   = strdup(RetCode)) == NULL   )
				{
					print_msg(PRT_ERR,"%s!\n","Error: Can not allocate memory!\n");
					return NULL;
				}

				cnt--;
			}
			else
			{
				*Ptr = SET_BEGIN_VAR;
				cnt--;

				print_msg(PRT_WARN,"Warning: Unknown variable `%s'!\n",Var);
			}
		}
		else
			*Ptr = SET_BEGIN_VAR;
	}

	if (cnt)
		print_msg(PRT_WARN,"Warning: Invalid token in string `%s'!\n",String);

	free(Begin);
	free(Var);
	free(End);

	if ((Ptr = RetCode) != NULL)
	{
		while (*Ptr != '\0')
		{
			if (*Ptr == SET_BEGIN_VAR)
				*Ptr = C_BEGIN_VAR;

			Ptr++;
		}
	}

	return RetCode;
}

/*************************************************************************
 ** KillCommand(-) - Beendet ein Programm anhand seiner PID.				**
 *************************************************************************/

static void KillCommand(int sock)
{
	char String[LONG_STRING_SIZE] = "";
		/* Kein Erbarmen - Alles was uns zwischen die Finger kommt wird	*/
		/* gelöscht :-)																	*/

	if (sock > 0)
	{
		if (!feof(sockets[sock].fp))
		{
			while (fgets(String,LONG_STRING_SIZE,sockets[sock].fp))
				if (String[0])
					print_msg(PRT_PROG_OUT,"%s\n",String);
		}

		kill(sockets[sock].pid, SIGTERM);
		kill(sockets[sock].pid, SIGKILL);

    /* ACHTUNG: Die naechste Zeile kann schwierigkeiten machen!!!
       Der Prozess kann eventuell hier haengen bleiben. Dann Zeile
       auskommentieren. Ich weiss nicht, ob kill den Prozess auch
       sauber beendet. Damit keine Zombies rumfahren vorsichtshaber
       der waitpid.
       Alternativ: waitpid(sockets[sock].pid,NULL,WNOHANG) */

		waitpid(sockets[sock].pid,NULL,0);
		fclose(sockets[sock].fp);
		del_socket(&sockets,sock);
	}
}

/*************************************************************************
 ** Alarm(-) - Gibt ein Alarmsignal über den internen Lautsprecher aus.	**
 *************************************************************************/

void Alarm(void)
{
#ifdef ALARM

	int FD;
	int i;

	if ((FD = open("/dev/console", O_WRONLY)) == -1) FD = 0;

	for (i = 0; i < 30; i++)
	{
		ioctl(FD, KIOCSOUND, (3000 - (i * 10)));

		usleep((1 * 1000));
	}

	ioctl(FD, KIOCSOUND, 0);

#endif
}

/*************************************************************************
 ** CheckTime(-) - Prüft ob die Zeitangabe in den lokalen Zeitrahmen		**
 **					 fällt. Rückgabe ist TRUE/FALSE.								**
 *************************************************************************/

int CheckTime(char *Line)
{
	char	 Temp[SHORT_STRING_SIZE + 1];
	char	 Time[24];
	char	*Use;
	struct tm	*tm_Time;
	char	*Arg;
	char	*Minus;
	int	 i;
	int	 r;
	int	 Beg;
	int	 End;
	time_t Local;

	if (Line == NULL || *Line == '\0')
		return 1;

	strncpy(Temp, Line, SHORT_STRING_SIZE);

	for (i = 0; i < 24; i++) Time[i] = 0;

	Use = Temp;

		/* Zeile in die einzelnen Komponenten trennen, die durch ',',	*/
		/* ' ' oder ';' voneinander getrennt sein können.					*/

	while ((Arg = strtok(Use, ", ;")))
	{
		Use = NULL;

		if (*Arg == '*')
			return 1;

		if (!isdigit(*Arg))
		{
			print_msg(PRT_WARN, " Wrong time in `%s`: \"%s\"\n", CONFFILE, Arg);
			continue;
		}

		if ((Minus = strchr(Arg, '-')))
		{
			*Minus++ = 0;

			Beg = atoi(Arg);
			End = atoi(Minus);
		}
		else
		{
			Beg = atoi(Arg);
			End = Beg+1;
		}

    if ((Beg < 0) || (End < 0) || (Beg > 24) || (End > 25))
		{
			print_msg(PRT_WARN, "Time range not correct in `%s`: (%d-%d)\n", CONFFILE, Beg, End);
		}
		else
		{
			if (End <= Beg)
				End += 24;

			for (r = Beg; r < End; r++) Time[r%24] = 1;
		}
	}

		/* Lokale Zeit errechnen und mit den Stunden im Zeitarray ver-	*/
		/* gleichen.																	*/

	Local = time(NULL);

	if ((tm_Time = localtime(&Local)) != NULL)
	{
			if (Time[tm_Time->tm_hour] == 1)
				return(1);
	}
	else print_msg(PRT_ERR, "Can't get local time.\n");

	return(0);
}

/****************************************************************************/

int Print_Cmd_Output( int sock )
{
	char String[LONG_STRING_SIZE] = "";


	if (feof(sockets[sock].fp))
	{
		KillCommand(sock);
		return -1;
	}

	fgets(String,LONG_STRING_SIZE,sockets[sock].fp);

	if (!feof(sockets[sock].fp) || String[0])
		print_msg(PRT_PROG_OUT,"%s\n",String);

	return 0;
}

/****************************************************************************/

int Get_Sock_From_Info_Args( info_args *Ptr, int Cnt )
{
	if (socket_size(sockets) > Cnt || Cnt < 0) {
		while (sockets[Cnt].descriptor != -2)
			if (sockets[Cnt].info_arg == Ptr)
				return Cnt;
			else
				Cnt++;
	}
	return -1;
}

/****************************************************************************/

int Get_Sock_From_Call( int chan, int Cnt )
{
	if (socket_size(sockets) > Cnt || Cnt < 0) {
		while (sockets[Cnt].descriptor != -2)
			if (sockets[Cnt].chan == chan)
				return Cnt;
			else
				Cnt++;
	}
	return -1;
}

/****************************************************************************/

int Get_Sock_From_Call_And_Info_Args( int chan, info_args *Ptr, int Cnt )
{
	if (socket_size(sockets) > Cnt || Cnt < 0) {
		while (sockets[Cnt].descriptor != -2)
			if (sockets[Cnt].chan == chan && sockets[Cnt].info_arg == Ptr)
				return Cnt;
			else
				Cnt++;
	}
	return -1;
}

/****************************************************************************/

int Condition_Changed( int condition, int flag )
{
	if ((flag & RING_CONNECT) || (flag & RING_RING))
	{
		if ((flag & RING_CONNECT) && condition == RING_CONNECT)
			return 0;

		if ((flag & RING_CONNECT) && condition == RING_AOCD)
			return 0;

		if ((flag & RING_CONNECT) && condition == RING_ERROR)
			return 0;

		if ((flag & RING_RING) && condition == RING_RING)
			return 0;

		if ((flag & RING_RING) && condition == RING_ERROR)
			return 0;

		return 1;
	}
	return 0;
}

/****************************************************************************/

const char *Set_Ringer_Flags( int condtion, int InOut )
{
	char Char = 0;
	static char RetCode[10];

	if (InOut & RING_INCOMING) Char = 'I';
	else
	if (InOut & RING_OUTGOING) Char = 'O';
	else
	{
		print_msg(PRT_ERR, "Error: Expected flag `I' or `O'!\n");
		return NULL;
	}

	RetCode[0] = Char;

	if (condtion & RING_RING    ) Char = 'R';
	else
	if (condtion & RING_CONNECT ) Char = 'C';
	else
	if (condtion & RING_BUSY    ) Char = 'B';
	else
	if (condtion & RING_AOCD    ) Char = 'A';
	else
	if (condtion & RING_ERROR   ) Char = 'E';
	else
	if (condtion & RING_HANGUP  ) Char = 'H';
	else
	if (condtion & RING_KILL    ) Char = 'K';
	else
	if (condtion & RING_SPEAK   ) Char = 'S';
	else
	if (condtion & RING_PROVIDER) Char = 'P';
	else
	{
		print_msg(PRT_ERR, "Internal error: Unknown flag %d for flag -S!\n",condtion);
		return NULL;
	}

	RetCode[1] = Char;
	RetCode[2] = '\0';

	return RetCode;
}

/****************************************************************************/

int Start_Interval(void)
{
	interval *Ptr  = RootIntervall;
	interval *next;
	time_t cur_time = time(NULL);
	int RetCode     = 0;

	while (Ptr != NULL)
	{
		next = Ptr->next;

		if (Ptr->next_start <= cur_time)
		{
			Ptr->next_start = cur_time + Ptr->infoarg->interval;
			RetCode += Start_Ring(Ptr->chan, Ptr->infoarg, Ptr->event, RING_INTERVAL);
		}

		Ptr = next;
	}

	return RetCode;
}

/****************************************************************************/

static interval *Next_Interval(void)
{
	interval *Ptr     = RootIntervall;
	interval *RetCode = NULL;
	time_t next_time  = 0;

	while (Ptr != NULL)
	{
		if (next_time == 0 || Ptr->next_start < next_time)
		{
			next_time = Ptr->next_start;
			RetCode = Ptr;
		}

		Ptr = Ptr->next;
	}

	return RetCode;
}

/****************************************************************************/

struct timeval *Get_Interval(int Sec)
{
	static struct timeval timeout;
	interval *Ptr   = NULL;


	timeout.tv_usec = 0;

	if (Sec < 0)
		Sec = 0;

	if ((Ptr = Next_Interval()) != NULL || Sec != 0)
	{
		if (Ptr != NULL && (timeout.tv_sec = (int) (Ptr->next_start - time(NULL))) < 0)
			timeout.tv_sec = 0;
                else if (Sec)
		/* if (Sec != 0 && Sec < timeout.tv_sec) AK:06-Jun-96 */
			timeout.tv_sec = Sec;
	}
	else
		return NULL;

	return &timeout;
}

/****************************************************************************/

int Del_Interval(int chan, info_args *infoarg)
{
	interval **Ptr = &RootIntervall;
	interval *Ptr2;

	while (*Ptr != NULL)
	{
		if ((*Ptr)->infoarg == infoarg && (*Ptr)->chan == chan)
		{
			Ptr2 = (*Ptr)->next;
			memset(*Ptr, 0, sizeof(**Ptr));
			free(*Ptr);
			*Ptr = Ptr2;
			return 0;
		}

		Ptr = &((*Ptr)->next);
	}

	return -1;
}

/****************************************************************************/

int New_Interval(int chan, info_args *infoarg, int event)
{
	interval **Ptr = &RootIntervall;

	if (infoarg->interval == 0)
		return -1;

	while (*Ptr != NULL)
		Ptr = &((*Ptr)->next);

	if ((*Ptr = (interval*) calloc(1,sizeof(interval))) == NULL)
		return -1;

	(*Ptr)->event      = event;
	(*Ptr)->infoarg    = infoarg;
	(*Ptr)->chan       = chan;
	(*Ptr)->next_start = infoarg->interval + time(NULL);

	return 0;
}

/****************************************************************************/

int Start_Process(int chan, info_args *infoarg, int event)
{
	int   InOut = call[chan].dialin?RING_INCOMING:RING_OUTGOING;
	int   sock = -1;


	if ((infoarg->flag & event) && (infoarg->flag & InOut)                       &&
			CheckTime(infoarg->time)                          &&     /* wenn die angegebene Zeit passt */
			(sock = Ring(infoarg, Get_Opts(chan,event,InOut), 0, 0)) != -1   )
	{
		sockets[sock].info_arg   = infoarg;
		sockets[sock].chan       = chan;
		sockets[sock].call_event = event;
	}

	return sock<0?-1:0;
}

/****************************************************************************/

static char *ArgToChar(int type, void* Ptr)
{
	static char RetCode[10][20];
	static int Cnt = 0;

	Cnt = Cnt%10;

	switch(type)
	{
		case R_TYPE_INT    : sprintf(RetCode[Cnt],"%d",*((int*) Ptr));
		                     break;
		case R_TYPE_LONG   : sprintf(RetCode[Cnt],"%ld",*((long*) Ptr));
		                     break;
		case R_TYPE_DOUBLE : strcpy(RetCode[Cnt],double2str(*((double*) Ptr),8,2,0));
		                     break;
		default            : RetCode[Cnt][0] = '\0';
		                     break;
	}

	return RetCode[Cnt++];
}

/****************************************************************************/

char **Get_Opts(int chan, int event, int InOut)
{
	static char *Opts[21];
	static char Strings[2][30];

	Opts[0] = (char*) Set_Ringer_Flags(event,InOut);
	Opts[1] = call[chan].num[CALLING];
	Opts[2] = call[chan].num[CALLED];

	if (call[chan].connect)
	{
		Opts[3] = strcpy(Strings[0],ctime(&(call[chan].connect)));
		Opts[3][strlen(Opts[3])-1] = '\0';
	}
	else
		Opts[3] = "";

	if (call[chan].connect)
	{
		long Help = (long) (time(NULL) - call[chan].connect);
		Opts[4] = ArgToChar(R_TYPE_LONG, &Help);
	}
	else
		Opts[4] = "";

	if (call[chan].disconnect)
	{
		Opts[5] = strcpy(Strings[1],ctime(&(call[chan].disconnect)));
		Opts[5][strlen(Opts[5])-1] = '\0';
	}
	else
		Opts[5] = "";

	if (call[chan].ibytes)
		Opts[6] = ArgToChar(R_TYPE_LONG, &(call[chan].ibytes));
	else
		Opts[6] = "";

	if (call[chan].obytes)
		Opts[7] = ArgToChar(R_TYPE_LONG, &(call[chan].obytes));
	else
		Opts[7] = "";

	if (call[chan].ibps)
		Opts[8] = ArgToChar(R_TYPE_DOUBLE, &(call[chan].ibps));
	else
		Opts[8] = "";

	if (call[chan].obps)
		Opts[9] = ArgToChar(R_TYPE_DOUBLE, &(call[chan].obps));
	else
		Opts[9] = "";

	if (call[chan].si1)
		Opts[10] = ArgToChar(R_TYPE_INT, &(call[chan].si1));
	else
		Opts[10] = "";

	if (call[chan].pay)
		Opts[11] = ArgToChar(R_TYPE_DOUBLE, &(call[chan].pay));
	else
		Opts[11] = "";

	Opts[12] = call[chan].areacode[CALLING];
	Opts[13] = call[chan].areacode[CALLED];
	Opts[14] = call[chan].vorwahl[CALLING];
	Opts[15] = call[chan].vorwahl[CALLED];
	Opts[16] = call[chan].area[CALLING];
	Opts[17] = call[chan].area[CALLED];
	Opts[18] = call[chan].alias[CALLING];
	Opts[19] = call[chan].alias[CALLED];

	Opts[20] = NULL;
	
	return Opts;
}

/****************************************************************************/

int Start_Ring(int chan, info_args *infoarg, int event, int intervalflag)
{
	int   ProcessStarted = 0;
	int   InOut = call[chan].dialin?RING_INCOMING:RING_OUTGOING;
	int f    = infoarg->flag; /* die Flags zu diesem Eintrag */
	int sock = 0;


	if (intervalflag & RING_INTERVAL)
	{
		if (f & RING_KILL) {
			while ((sock = Get_Sock_From_Call_And_Info_Args(chan,infoarg,sock)) != -1)
				if (sockets[sock].call_event == event)
					Ring(NULL, NULL, sock++, 0);
				else
					sock++;
		}			
	}
	else
	{
		if (infoarg->interval != 0 && (event == RING_RING || event == RING_CONNECT))
			New_Interval(chan, infoarg, event);
	}

	if (Condition_Changed(event,f))
	{
		while ((sock = Get_Sock_From_Call_And_Info_Args(chan,infoarg,sock)) != -1)
			Ring(NULL, NULL, sock++, 0);

		Del_Interval(chan,infoarg);
	}

	/* Wenn Event (siehe oben) passt, und INCOMING/OUTGOING passen */

	if (!((f & RING_UNIQUE)                             &&
	    ((event == RING_RING) || (event == RING_CONNECT)  )) ||
	    Get_Sock_From_Call_And_Info_Args(chan,infoarg,0) == -1               )
	{
		if ((f & event) && (f & InOut)                       &&
				CheckTime(infoarg->time)                         &&     /* wenn die angegebene Zeit passt */
				(sock = Ring(infoarg, Get_Opts(chan,event,InOut), 0, 0)) != -1  )
		{
			sockets[sock].info_arg   = infoarg;
			sockets[sock].chan       = chan;
			sockets[sock].call_event = event;
			ProcessStarted++;
		}
	}

	return ProcessStarted;
}

/****************************************************************************/

int Change_Channel_Ring( int old_channel, int new_channel)
{
	interval *Ptr = RootIntervall;
	int sock = 0;

	while ((sock = Get_Sock_From_Call( old_channel, sock )) >= 0)
	{
		sockets[sock].chan = new_channel;
	}

	while (Ptr != NULL)
	{
		if (Ptr->chan == old_channel)
			Ptr->chan = new_channel;

		Ptr = Ptr->next;
	}

	return 0;
}

/****************************************************************************/

