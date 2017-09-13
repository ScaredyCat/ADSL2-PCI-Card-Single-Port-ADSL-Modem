/* $Id: user_access.c,v 1.4 2002/04/22 19:07:50 akool Exp $
 *
 * ISDN accounting for isdn4linux.
 *
 * Copyright 1996 by Stefan Luethje (luethje@sl-gw.lake.de)
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
 * $Log: user_access.c,v $
 * Revision 1.4  2002/04/22 19:07:50  akool
 * isdnlog-4.58:
 *   - Patches from Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de>
 *     - uninitialized variables in
 *     	- isdn4k-utils/isdnlog/connect/connect.c
 *       - isdn4k-utils/isdnlog/tools/rate.c
 *     - return() of a auto-variable in
 *       - isdn4k-utils/isdnlog/isdnlog/user_access.c
 *
 *     *Many* thanks to Enrico!!
 *
 *   - New rates as of April, 23. 2002 (EUR 0,014 / minute long distance call ;-)
 *
 * Revision 1.3  1999/10/25 18:33:15  akool
 * isdnlog-3.57
 *   WARNING: Experimental version!
 *   	   Please use isdnlog-3.56 for production systems!
 *
 * Revision 1.2  1997/04/03 22:34:52  luethje
 * splitt the files callerid.conf and ~/.isdn.
 *
 */

/* Alle Funktionen liefern im Fehlerfall -1 oder NULL */

/****************************************************************************/

#define  _USER_ACCESS_C_

/****************************************************************************/

#include "isdnlog.h"

#include <pwd.h>
#include <sys/types.h>

/****************************************************************************/

#define ANY -1

#define C_DELIMITER_FLAGS					';'
#define C_DELIMITER_FLAGOPTIONS		','
#define C_DELIMITER_FLAG_AND_OPT	'='

#define ALLOW_ALL "WORLD"

/****************************************************************************/

typedef struct _access_flags{
	int type;
	char **options;
	struct _access_flags *next;
} access_flags;

typedef struct _access_entry{
	char *name;
	access_flags *flags;
	struct _access_entry *next;
} access_entry;

typedef struct _user_access{
	char *name;
	access_entry *hosts;
	access_entry *user;
	struct _user_access *next;
} user_access;

static struct ValidFlag{
	char *name;
	int   type;
	int   number;
	int   flag;
} ValidFlags[] = {
	{"ALL", 0, 0, T_ALL},
	{"MSN", 1, ANY, 0},
	{"I4LCONF", 2, 0, T_I4LCONF},
	{"PROTOCOL", 3, 0, T_PROTOCOL},
	{"ADDRESSBOOK", 4, 0, T_ADDRESSBOOK},
	{NULL,0}
};

/****************************************************************************/

/* ACHTUNG: Alle #define's muessen mit der obigen Variable ValidFlags
   abgestimmt sein. */

#define FLAG_ALL					0
#define FLAG_MSN					1
#define FLAG_I4LCONF			2
#define FLAG_PROTOCOL			3
#define FLAG_ADDRESSBOOK	4

/****************************************************************************/

static user_access *AllEntries = NULL;
static user_access *NewAllEntries = NULL;
static int NoName = 1;

/****************************************************************************/

static int _read_file (void);
static char *Find_Section(char* Dest, char const *Src);
static int Clear_All (user_access **Cursor);
static int Set_Flags(access_flags **Cursor, char *String);
static int _Set_Entry (char* Name, char* User, char *Host, char *Flags);
static void free_user_access(user_access *Ptr);
static void free_access_entry(access_entry *Ptr);
static void free_access_flags(access_flags *Ptr);
static int write_entry(FILE *fp, user_access *Ptr);
static int write_user(FILE *fp, access_entry *Ptr);
static int write_hosts(FILE *fp, access_entry *Ptr);
static int write_flag(FILE *fp, access_flags *Ptr);
static int Check_Entries(void);
static int create_new_file(void);
static access_flags *Find_Data(user_access *Cursor, char *User, char *Host);
static int Find_Host(access_entry *Cursor, char *Host);
static char *IsFlag(char *Flag, int FlagIndex);
static access_flags *Set_Flag(char* Flag);
static char **Set_Options(char *String);
static int return_for_user_access(access_flags *Ptr);

/****************************************************************************/

int read_user_access( void )
{
	int RetCode = -1;
	static int ModTime = -2;
	static struct stat FileStat;

	/* Erster Durchlauf von read_user_access() */
	if (ModTime == -2)
	{
		if (stat(userfile(),&FileStat) != 0 && errno == ENOENT) {
			if ((RetCode = create_new_file()) == 0)
				stat(userfile(),&FileStat);
			else
				/* Return, weil keine Date existiert und nicht angelegt werden kann */
				return RetCode;
		}
		ModTime = FileStat.st_mtime;
	}
	else
	{
		ModTime = FileStat.st_mtime;
		stat(userfile(),&FileStat);

		/* Wenn das File seit dem Letzten Lesen nicht mehr geaendert wurde,
		   dann wieder zurueck, sonst wieder neu einlesen                  */

		if (ModTime == FileStat.st_mtime)
			/* Alles beim alten */
			return 0;
	}

	if ((RetCode = Clear_All(&NewAllEntries)) != 0)
		return RetCode;

	if ((RetCode = _read_file()) != 0)
		return RetCode;

	if ((RetCode = Clear_All(&AllEntries)) != 0)
		return RetCode;

	AllEntries = NewAllEntries;
	NewAllEntries = NULL;

	return 0;
}

/****************************************************************************/

static int _read_file (void)
{
	int Line = 0;
	int RetCode;
	FILE *fp;
	char String[SHORT_STRING_SIZE];
	char User[SHORT_STRING_SIZE];
	char Host[SHORT_STRING_SIZE];
	char Name[SHORT_STRING_SIZE];
	char dummy[SHORT_STRING_SIZE];
	char Flags[SHORT_STRING_SIZE];
	char *Ptr;


	if ((fp = fopen(userfile(), "r")) == NULL)
	{
		print_msg(PRT_ERR,"Can't open \"%s\" (%s)\n",userfile(),strerror(errno));
		return -1;
	}

	while (FGets(String, BUFSIZ, fp, &Line) != NULL)
	{
		Name[0] = User[0] = Host[0] = Flags[0] = '\0';

		if (*String != '\0' && *String != '\n')
		{
			if (Find_Section(Name, String) != NULL)
			{
				To_Upper(Name);
				print_msg(PRT_DEBUG_CS,"Name:*%s*\n",Name);
			}
			else
			if (NoName)
			{
				if (sscanf(String,"%[^@]@%s %[^\n]",User,Host,Flags) >= 2)
					print_msg(PRT_DEBUG_CS,"User:*%s* Host:*%s* Flags:*%s*\n",User,Host,Flags);
				else
				{
					print_msg(PRT_ERR,"File \"%s\": error in line %d!\n",userfile(),Line);
					Name[0] = User[0] = Host[0] = Flags[0] = '\0';
				}
			}
			else
			{
				if (sscanf(String,"%[^@]@%s %[^\n]",User,Host,Flags) >= 2)
				{
					print_msg(PRT_ERR,"File \"%s\": error in line %d!\n",userfile(),Line);
					Name[0] = User[0] = Host[0] = Flags[0] = '\0';
				}
				else
				if ((RetCode = sscanf(String,"@%s %[^\n]",Host,dummy)) == 1)
					print_msg(PRT_DEBUG_CS,"Host:*%s* Flags:*%s*\n",Host,Flags);
				else
				if (RetCode == 0 && sscanf(String,"%s %[^\n]",User,Flags) >= 1)
					print_msg(PRT_DEBUG_CS,"User:*%s* Flags:*%s*\n",User,Flags);
				else
				{
					print_msg(PRT_ERR,"File \"%s\": error in line %d!\n",userfile(),Line);
					Name[0] = User[0] = Host[0] = Flags[0] = '\0';
				}
			}
		}

		if ((RetCode = _Set_Entry(Name,User,Host,Flags)) != 0)
			return RetCode;
	}

	Check_Entries();

	fclose(fp);
	return 0;
}

/****************************************************************************/

static char *Find_Section(char* Dest, char const *Src)
{
	char *Ptr = NULL;
	char Help[SHORT_STRING_SIZE];
	char *String;


	strcpy(Help,Src);
	String = Kill_Blanks(Help);

	if (*String == '\0' || *String != '[')
		return NULL;

	Ptr = String + strlen(String)-1;
	if (Ptr == strchr(String,']'))
	{
		*Ptr = '\0';
		strcpy(Dest, String+1);
		return Dest;
	}

	return NULL;
}

/****************************************************************************/

static int Set_Flags(access_flags **Cursor, char *String)
{
	int Cnt = 0;
	int RetCode = 0;
	char **Ptr = NULL;

	if (String == NULL)
		return RetCode;

	String = Kill_Blanks(To_Upper(String));

	print_msg(PRT_DEBUG_CS,"Short Flags:*%s*\n",String);

	if ((Ptr = String_to_Array(String,C_DELIMITER_FLAGS)) != NULL)
		while (Ptr[Cnt] != NULL)
		{
			if (Ptr[Cnt][0] != '\0')
			{
				*Cursor = Set_Flag(Ptr[Cnt]);
				Cursor = &((*Cursor)->next);
			}

			Cnt++;
		}

	del_Array(Ptr);

	return RetCode;
}

/****************************************************************************/

static access_flags *Set_Flag(char* Flag)
{
	int Cnt = 0;
	char *Ptr;
	access_flags *RetCode = NULL;

	if (*Flag == '\0')
		return NULL;

	print_msg(PRT_DEBUG_CS,"One Flag:*%s*\n",Flag);

	while (ValidFlags[Cnt].name != NULL)
	{
		if ((Ptr = IsFlag(Flag,Cnt)) != NULL)
		{
			print_msg(PRT_DEBUG_CS,"Found Flag:*%s*\n",ValidFlags[Cnt].name);

			if ((RetCode = (access_flags*) calloc(1,sizeof(access_flags))) == NULL)
			{
				print_msg(PRT_ERR,"Can't alloc memory!\n");
				Exit(26);
			}

			RetCode->type = ValidFlags[Cnt].type;

			if (ValidFlags[Cnt].number != 0)
				RetCode->options = Set_Options(Ptr);

			return RetCode;
		}

		Cnt++;
	}

	print_msg(PRT_ERR,"Error: invalid Flag \"%s\"!\n",Flag);

	return RetCode;
}

/****************************************************************************/

static char *IsFlag(char *Flag, int FlagIndex)
{
	char *RetCode = NULL;
	int Len = strlen(ValidFlags[FlagIndex].name);

	if ((RetCode = strstr(Flag,ValidFlags[FlagIndex].name)) != NULL     &&
			Flag == RetCode                                                   )
	{
		if (ValidFlags[FlagIndex].number == 0)
		{
			if (Flag[Len] == '\0')
				return RetCode;
		}
		else
		{
		  if (*(RetCode = &(Flag[Len])) == C_DELIMITER_FLAG_AND_OPT )
				return (RetCode+1);
			else
			{
				print_msg(PRT_ERR,"Warning: invalid option string \"%s\"!\n",Flag);
				return NULL;
			}
		}
	}

	return NULL;
}

/****************************************************************************/

static char **Set_Options(char *String)
{
	int Cnt = 0;
	int Size = 2;
	char **Ptr = NULL;
	char **Options = NULL;

	if ((Ptr = String_to_Array(String,C_DELIMITER_FLAGOPTIONS)) != NULL)
		while (Ptr[Cnt] != NULL)
		{
			if (Ptr[Cnt][0] != '\0')
			{
				if ((Options = (char**) realloc(Options,sizeof(char*)*Size)) == NULL)
				{
					print_msg(PRT_ERR,"Can't alloc memory!\n");
					Exit(25);
				}

				Options[Size-1] = NULL;
				if ((Options[Size-2] = (char*) calloc(strlen(Ptr[Cnt]),sizeof(char))) == NULL)
				{
					print_msg(PRT_ERR,"Can't alloc memory!\n");
					Exit(25);
				}
				else
					strcpy(Options[Size-2],Ptr[Cnt]);

				print_msg(PRT_DEBUG_CS,"Option:*%s*\n",Options[Size-2]);
				Size++;
			}

			Cnt++;
		}

	del_Array(Ptr);
	return Options;
}

/****************************************************************************/

static int Check_Entries(void)
{
	user_access *Cursor = NewAllEntries;

	if (Cursor == NULL)
		print_msg(PRT_WARN,"Warning: file \"%s\" is empty!\n",userfile());

	while(Cursor)
	{
		if (Cursor->name == NULL)
		{
			if (Cursor->hosts == NULL)
					print_msg(PRT_WARN,
					  "Warning: file \"%s\" in the first section has no host!\n",userfile());

			if (Cursor->user == NULL)
					print_msg(PRT_WARN,
					  "Warning: file \"%s\" in the first section has no user!\n",userfile());
			else
				if (Cursor->user->flags == NULL)
						print_msg(PRT_WARN,
						  "Warning: file \"%s\" in the first section a user has no flags!\n",userfile());
		}
		else
		if (strcmp(Cursor->name,ALLOW_ALL))
		{
			if (Cursor->hosts == NULL)
					print_msg(PRT_WARN,
					  "Warning: file \"%s\" in section \"%s\" has no host!\n",userfile(),Cursor->name);

			if (Cursor->user == NULL)
					print_msg(PRT_WARN,
					  "Warning: file \"%s\" in section \"%s\" has no user!\n",userfile(),Cursor->name);
			else
				if (Cursor->user->flags == NULL)
						print_msg(PRT_WARN,
						  "Warning: file \"%s\" in section \"%s\" a user has no flags!\n",userfile(),Cursor->name);
		}

		Cursor = Cursor->next;
	}

	return 0;
}

/****************************************************************************/

static int _Set_Entry (char* Name, char* User, char *Host, char *Flags)
{
	char *NewHost;
	access_entry **Ptr = NULL;
	static user_access **Cursor = NULL;

	if (*Name == '\0' && *User == '\0' && *Host == '\0')
		return 0;

	if (!strcmp(Name,ALLOW_ALL)                                  ||
	    (NewAllEntries != NULL && NewAllEntries->name != NULL &&
	     !strcmp(NewAllEntries->name,ALLOW_ALL)                 )  )
	{
		if (*Name != '\0' && NewAllEntries != NULL)
			print_msg(PRT_WARN,
				"Warning: There is more than section \"%s\" in user access!\n",ALLOW_ALL);

		Clear_All(&NewAllEntries);
		strcpy(Name,ALLOW_ALL);
		*User = *Host = '\0';
	}

	if (NewAllEntries == NULL)
	{
		Cursor = &NewAllEntries;
		NoName = 1;
	}

	if (NoName || *Name != '\0')
		if (*Cursor != NULL)
		{
			Cursor = &((*Cursor)->next);
		}

	if (*Cursor == NULL)
	{
		if ((*Cursor = (user_access*) calloc(1,sizeof(user_access))) == NULL)
		{
			print_msg(PRT_ERR,"Can't alloc memory!\n");
			Exit(28);
		}
	}

	if (*Name != '\0')
	{
		NoName = 0;

		if (((*Cursor)->name = (char*) calloc(strlen(Name)+1,sizeof(char))) == NULL)
		{
			print_msg(PRT_ERR,"Can't alloc memory!\n");
			Exit(28);
		}

		strcpy((*Cursor)->name,Name);
	}
	else
	{
		if (*User != '\0')
		{
			Ptr = &((*Cursor)->user);

			while (*Ptr)
				Ptr = &((*Ptr)->next);

			if ((*Ptr = (access_entry*) calloc(1,sizeof(access_entry))) == NULL)
			{
				print_msg(PRT_ERR,"Can't alloc memory!\n");
				Exit(28);
			}

			if (((*Ptr)->name = (char*) calloc(strlen(User)+1,sizeof(char))) == NULL)
			{
				print_msg(PRT_ERR,"Can't alloc memory!\n");
				Exit(28);
			}

			strcpy((*Ptr)->name,User);

			if (*Flags != '\0')
				Set_Flags(&((*Ptr)->flags),Flags);
			else
				print_msg(PRT_WARN,"Warning: User \"%s\" has no Flags!\n",User);
		}

		if (*Host != '\0') {
			if ((NewHost = GetHostByName(Host)) != NULL)
			{
				Ptr = &((*Cursor)->hosts);

				while (*Ptr)
					Ptr = &((*Ptr)->next);

				if ((*Ptr = (access_entry*) calloc(1,sizeof(access_entry))) == NULL)
				{
					print_msg(PRT_ERR,"Can't alloc memory!\n");
					Exit(28);
				}

				(*Ptr)->name = NewHost;
			}
			else
				print_msg(PRT_WARN,"Warning: unknown host \"%s\"!\n",Host);
		}		
	}

	return 0;
}

/****************************************************************************/

static int Clear_All (user_access **Cursor)
{
	NoName = 1;

	if (*Cursor != NULL)
	{
		free_user_access(*Cursor);
		*Cursor = NULL;
	}

	return 0;
}

/****************************************************************************/

static void free_user_access(user_access *Ptr)
{
	if (Ptr != NULL)
	{
		free_user_access(Ptr->next);
		free_access_entry(Ptr->user);
		free_access_entry(Ptr->hosts);
		free(Ptr->name);
		free(Ptr);
	}
	else
		return;
}

/****************************************************************************/

static void free_access_entry(access_entry *Ptr)
{
	if (Ptr != NULL)
	{
		free_access_entry(Ptr->next);
		free_access_flags(Ptr->flags);
		free(Ptr->name);
		free(Ptr);
	}
	else
		return;
}

/****************************************************************************/

static void free_access_flags(access_flags *Ptr)
{
	int Cnt = 0;


	if (Ptr != NULL)
	{
		free_access_flags(Ptr->next);

		if (Ptr->options != NULL)
		{
			while(Ptr->options[Cnt] != NULL)
				free(Ptr->options[Cnt++]);

			free(Ptr->options);
		}

		free(Ptr);
	}
	else
		return;
}

/****************************************************************************/

int write_user_access( void )
{
	int RetCode = 0;
	FILE *fp;
	char Time[SHORT_STRING_SIZE] = "";
	time_t _Time = time(NULL);

	NoName = 1;


	strftime(Time,SHORT_STRING_SIZE,"%c",localtime(&_Time));

	if (AllEntries == NULL)
	{
		print_msg(PRT_ERR,"Error: file \"%s\": there are no user data!\n",userfile(),strerror(errno));
		return -1;
	}

	if ((fp = fopen(userfile(), "w")) == NULL)
	{
		print_msg(PRT_ERR,"Can't open \"%s\" (%s)\n",userfile(),strerror(errno));
		return -1;
	}

	fprintf(fp,"%s\n","###############################################################################");
	fprintf(fp,"%s\n","#");
	fprintf(fp,"%s %s %s on %s\n","#","Generated by isdnlog",VERSION,Time);
	fprintf(fp,"%s\n","#");
	fprintf(fp,"%s\n\n","###############################################################################");

	write_entry(fp,AllEntries);

	fclose(fp);

	return RetCode;
}

/****************************************************************************/

static int write_entry(FILE *fp, user_access *Ptr)
{
	int RetCode = -1;

	if (Ptr == NULL)
		return 0;

	if (Ptr->name == NULL || *(Ptr->name) == '\0')
	{
		if (NoName == 0)
		{
			print_msg(PRT_ERR,"%s\n","Internal error: file \"%s\": invalid user data in section with name",userfile());
			return RetCode;
		}

		if (Ptr->user == NULL || Ptr->user->name == NULL || Ptr->user->next != NULL ||
		    Ptr->hosts == NULL || Ptr->hosts->name == NULL || Ptr->hosts->next != NULL)
		{
			print_msg(PRT_ERR,"%s\n","Internal error: file \"%s\": invalid user data in section no name",userfile());
			return RetCode;
		}

		fprintf(fp,"%s@%s",Ptr->user->name,Ptr->hosts->name);

		if (Ptr->user->flags)
			if ((RetCode = write_flag(fp,Ptr->user->flags)) != 0)
				return RetCode;

		fprintf(fp,"\n");
	}
	else
	{
		NoName = 0;

		fprintf(fp,"\n[%s]\n",Ptr->name);

		if ((RetCode = write_user(fp,Ptr->user)) != 0)
			return RetCode;

		if ((RetCode = write_hosts(fp,Ptr->hosts)) != 0)
			return RetCode;
	}

	return write_entry(fp,Ptr->next);
}

/****************************************************************************/

static int write_flag(FILE *fp, access_flags *Ptr)
{
	int First = 1;
	char** Cursor = NULL;

	if (Ptr != NULL)
	{
		Cursor = Ptr->options;
		fprintf(fp," %s",ValidFlags[Ptr->type].name);

		if (Cursor != NULL)
		{
			fprintf(fp,"%c",C_DELIMITER_FLAG_AND_OPT);

			while(*Cursor != NULL)
			{
				if (!First)
					fprintf(fp,"%c",C_DELIMITER_FLAGOPTIONS);

				fprintf(fp,"%s",*Cursor);
				Cursor++;

				First = 0;
			}

		}

		fprintf(fp,"%c",C_DELIMITER_FLAGS);
		write_flag(fp,Ptr->next);
	}

	return 0;
}

/****************************************************************************/

static int write_user(FILE *fp, access_entry *Ptr)
{
	int RetCode = 0;


	if (Ptr != NULL)
	{
		if (Ptr->name != NULL)
			fprintf(fp,"%s",Ptr->name);
		else
		{
			print_msg(PRT_ERR,"%s\n","Internal error: file \"%s\": user has no name!\n",userfile());
			return -1;
		}

		if (Ptr->flags != NULL)
			if ((RetCode = write_flag(fp,Ptr->flags)) != 0)
				return RetCode;

		fprintf(fp,"\n");

		return write_user(fp,Ptr->next);
	}

	return 0;
}

/****************************************************************************/

static int write_hosts(FILE *fp, access_entry *Ptr)
{
	if (Ptr != NULL)
	{
		if (Ptr->name != NULL)
			fprintf(fp,"@%s\n",Ptr->name);
		else
		{
			print_msg(PRT_ERR,"%s\n","Internal error: file \"%s\": host has no name!\n",userfile());
			return -1;
		}

		if (Ptr->flags != NULL)
		{
			print_msg(PRT_ERR,"%s\n","Internal error: file \"%s\": host has flags!\n",userfile());
			return -1;
		}

		return write_hosts(fp,Ptr->next);
	}

	return 0;
}

/****************************************************************************/

static int create_new_file(void)
{
	int RetCode = 0;
	struct passwd *password;
	char Name[SHORT_STRING_SIZE];


	strcpy(Name,ValidFlags[FLAG_ALL].name);

	print_msg(PRT_WARN,"Warning: no file \"%s\" exists.\nIt will be created a default file. Please edit it!\n",userfile());

	if (AllEntries != NULL)
	{
		print_msg(PRT_WARN,"Internal warning: entries but no file \"%s\"!\n",userfile());

		if ((RetCode = Clear_All(&AllEntries)) != 0)
			return RetCode;
	}

	if ((RetCode = Clear_All(&NewAllEntries)) != 0)
		return RetCode;

	_Set_Entry("isdnlog-local","","","");
	_Set_Entry("","","localhost","");

	setpwent();
	while ((password = getpwent()) != NULL)
	{
		_Set_Entry("",password->pw_name,"",Name);
	}
	endpwent();

	AllEntries = NewAllEntries;
	NewAllEntries = NULL;

	return write_user_access();
}

/****************************************************************************/

int user_has_access(char *User, char *Host)
{
	/* ACHTUNG IN DIESER FKT DARF KEIN print_msg() AUFGERUFEN WERDEN !!!!! */
	access_flags *Ptr = NULL;
	int RetCode = -1;

	if (AllEntries != NULL && !strcmp(AllEntries->name,ALLOW_ALL))
		return T_ALL;

	if (Host == NULL || *Host == '\0' || User == NULL || *User == '\0')
		return -1;

	if ((Ptr = Find_Data(AllEntries,User,Host)) != NULL)
		RetCode = return_for_user_access(Ptr);

	return RetCode;
}

/****************************************************************************/

static access_flags *Find_Data(user_access *Cursor, char *User, char *Host)
{
	/* ACHTUNG IN DIESER FKT DARF KEIN print_msg() AUFGERUFEN WERDEN !!!!! */
	access_entry *Ptr = NULL;


	if (Cursor == NULL)
		return NULL;

	Ptr = Cursor->user;

	while (Ptr)
	{
		if (!strcmp(Ptr->name,User) && Find_Host(Cursor->hosts,Host) == 0)
				return Ptr->flags;

		Ptr = Ptr->next;
	}

	return Find_Data(Cursor->next,User,Host);
}

/****************************************************************************/

static int Find_Host(access_entry *Cursor, char *Host)
{
	/* ACHTUNG IN DIESER FKT DARF KEIN print_msg() AUFGERUFEN WERDEN !!!!! */
	while(Cursor)
	{
		if (Cursor->name != NULL)
			if (!strcmp(Cursor->name,Host))
				return 0;

		Cursor = Cursor->next;
	}

	return -1;
}

/****************************************************************************/

int User_Get_Message(char *User, char *Host, char* mymsn, int Flag)
{
	/* Hier muss ueberprueft werden, ob der User die MSG vom Server erhalten
	   darf. */
	/* Die Bedingung der Fkt. gilt als erfuellt, wenn entweder die MSN
	   oder ein Flag uebereinstimmt. */
	/* ACHTUNG IN DIESER FKT DARF KEIN print_msg() AUFGERUFEN WERDEN !!!!! */

	access_flags *Ptr = NULL;
	char   Help[SHORT_STRING_SIZE];
	char **Options = NULL;
	int Cnt;
	int RetCode = -1;


	if (Host == NULL || *Host == '\0' || User == NULL || *User == '\0')
		return RetCode;

	if (AllEntries != NULL && !strcmp(AllEntries->name,ALLOW_ALL))
		return 0;

	if ((Ptr = Find_Data(AllEntries,User,Host)) != NULL)
		while (Ptr)
		{
			/* Damit ALL ueberall (auch bei MSN) seine Wirkung erhaelt: */
			if (Ptr->type == FLAG_ALL)
				return 0;

			if (mymsn != NULL && Ptr->type == FLAG_MSN)
			{
				Cnt = 0;
				Options = Ptr->options;

				if (Options)
					while(Options[Cnt] != NULL)
					{
						if (Options[Cnt][0] == '*')
							strcpy(Help,Options[Cnt]);
						else
							sprintf(Help,"%c%s",'*',Options[Cnt]);

						/* ACHTUNG: match("","*") ist false -> Workaround */
						if (!match(Help,mymsn,0) || (*mymsn == '\0' && !strcmp(Options[Cnt],"*")))
							return 0;

						Cnt++;
					}
			}

			if ((ValidFlags[Ptr->type].flag & Flag) != 0)
				return 0;

		Ptr = Ptr->next;
	}

	return RetCode;
}

/****************************************************************************/

static int return_for_user_access(access_flags *Ptr)
{
	/* ACHTUNG IN DIESER FKT DARF KEIN print_msg() AUFGERUFEN WERDEN !!!!! */
	int RetCode = -1;


	if (Ptr == NULL)
		return RetCode;

	RetCode = ValidFlags[Ptr->type].flag;

	if (Ptr->next != NULL)
		RetCode = RetCode | return_for_user_access(Ptr->next);

	return RetCode;
}

/****************************************************************************/

const char *userfile(void)
{
	static char File[LONG_STRING_SIZE] = "";


	if (*File == '\0')
	{
		sprintf(File,"%s%c%s",confdir(),C_SLASH,USERFILE);

		if (*File == '\0')
		{
			print_msg(PRT_ERR,"%s\n","Invalid filename for user access!");
			Exit(23);
		}
	}

	return File;
}

/****************************************************************************/

