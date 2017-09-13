/* $Id: conffile.c,v 1.21 2000/07/19 19:45:43 akool Exp $
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
 * $Log: conffile.c,v $
 * Revision 1.21  2000/07/19 19:45:43  akool
 * increased BUFSIZ
 *
 * Revision 1.20  1999/11/03 16:13:36  paul
 * Added { } to suppress egcs warnings.
 *
 * Revision 1.19  1998/05/20 12:22:15  paul
 * More paranoid about freeing pointers.
 *
 * Revision 1.18  1998/05/20 09:56:14  paul
 * Oops, the temp string _was_ necessary. Made it static so that returning a
 * pointer to it is not a problem.
 *
 * Revision 1.17  1998/05/20 09:25:01  paul
 * function Find_Section returned pointer to local automatic variable. Local
 * variable removed, as it was not necessary; now return parameter.
 *
 * Revision 1.16  1997/05/25 19:41:23  luethje
 * isdnlog:  close all files and open again after kill -HUP
 * isdnrep:  support vbox version 2.0
 * isdnconf: changes by Roderich Schupp <roderich@syntec.m.EUnet.de>
 * conffile: ignore spaces at the end of a line
 *
 * Revision 1.15  1997/04/15 22:37:20  luethje
 * allows the character `"' in the program argument like the shell.
 * some bugfixes.
 *
 * Revision 1.14  1997/04/15 00:20:13  luethje
 * replace variables: some bugfixes, README comleted
 *
 * Revision 1.13  1997/04/10 23:41:26  luethje
 * some bug fixes
 *
 * Revision 1.11  1997/04/03 22:39:11  luethje
 * bug fixes: environ variables are working again, no seg. 11 :-)
 * improved performance for reading the config files.
 *
 * Revision 1.10  1997/03/24 03:56:30  fritz
 * Fixed 2 typos
 *
 * Revision 1.9  1997/03/23 23:12:10  luethje
 * improved performance
 *
 * Revision 1.8  1997/03/20 00:22:51  luethje
 * Only a test
 *
 */

#define _CONFFILE_C_

#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "conffile.h"

/****************************************************************************/

#define F_TAG   1
#define F_UNTAG 2

#define C_EXIST '!'

/****************************************************************************/

static int (*print_msg)(const char *, ...) = printf;

/****************************************************************************/

static int Write_Lines(section *Section, FILE *fp, const char *FileName, int Level);
static section *Read_Lines(section *Section, FILE *fp, const char *FileName, int *Line, int Flags);
static char *Find_Section(char* String);
static int Find_Entry(const char *FileName, int Line, char* String, char** Variable, char** Value);
static entry* Append_Entry(entry** Entry, char *Variable, char* Value, section *SubSection, int Flag);
static void free_entry(entry *Ptr);
static void free_cfile(cfile **cfiles);
static const char *Pre_String(int Level);
static int Compare_Sections(section* sec1, section *sec2, char **variables);
static section *Insert_Section(section **main_sec, section **ins_sec, char **variables, int flags);
static int Merge_Sections(section **main_sec, section **ins_sec, char **variables, int flags);
static int Append_Sections(section **main_sec, section *app_sec);
static int Find_Include(section **Section, char* String, const char *FileName, int Flags);
static section* _Get_Section_From_Path(char **array, section* Section, section **RetSection, entry **RetEntry, int flags);
static entry* _Get_Entry_From_Path(char **array, entry* Entry, section **RetSection, entry **RetEntry, int flags);
static section* Get_Section_From_Path(section* NewSection, char *Path, entry **RetEntry);
static char* Delete_Chars(char *String, char *Quote);
static int Set_Ret_Code(char *Value, int Type, void **Pointer);
static int del_untagged_items(section **sec);
static char** Compare_Section_Get_Path(char **array, int *retsize, int *retdepth);

/****************************************************************************/

void set_print_fct_for_conffile(int (*new_print_msg)(const char *, ...))
{
	print_msg = new_print_msg;
}

/****************************************************************************/

section *write_file(section *Section, const char *FileName, char *Program, char* Version)
{
	FILE *fp;
	time_t t =time(NULL);


	if ((fp = fopen(FileName, "w")) == NULL)
	{
		print_msg("Can't write `%s' (%s)\n",FileName,strerror(errno));
		return NULL;
	}

	fprintf(fp,"###############################################################################\n");
	fprintf(fp,"#\n");
	fprintf(fp,"# File %s generated by %s %s on %s",FileName,Program,Version,ctime(&t));
	fprintf(fp,"#\n");
	fprintf(fp,"###############################################################################\n");

	Write_Lines(Section,fp,FileName,0);

  fclose(fp);
	return Section;
}

/****************************************************************************/

static int Write_Lines(section *Section, FILE *fp, const char *FileName, int Level)
{
	entry *Ptr;


	while (Section != NULL)
	{
		fprintf(fp,"\n%s[%s]\n", Pre_String(Level), Section->name);

		Ptr = Section->entries;

		while (Ptr != NULL)
		{
			if (Ptr->name)
			{
				fprintf(fp,"%s%s %c ", Pre_String(Level), Ptr->name, C_EQUAL);

				if (Ptr->value != NULL)
					fprintf(fp,"%s\n",Quote_Chars(Ptr->value));
				else
				if (Ptr->subsection != NULL)
				{
					fprintf(fp,"%c\n", C_BEGIN_SUBSECTION);
					Write_Lines(Ptr->subsection,fp,FileName,Level+1);
					fprintf(fp,"%s%c\n", Pre_String(Level), C_END_SUBSECTION);
fflush(fp);
				}
				else
					print_msg("Warning in file `%s': There is no value for `%s'!\n", FileName, Ptr->name);
			}

			Ptr = Ptr->next;
		}

		Section = Section->next;
	}
	return 0;
}

/****************************************************************************/

static const char *Pre_String(int Level)
{
	static int  OldLevel = -1;
	static char PreString[SHORT_STRING_SIZE];

	if (Level != OldLevel)
	{
		memset(PreString,' ',Level*2);
		PreString[Level*2] = '\0';
		OldLevel = Level;
	}

	return PreString;
}

/****************************************************************************/

section *read_file(section *Section, const char *FileName, int Flags)
{
	int   Line = 0;
	FILE *fp   = NULL;
	section  *RetCode = NULL;


	if ((fp = fopen(FileName, "r")) == NULL)
	{
		if (!(Flags & C_NO_WARN_FILE))
			print_msg("Can't open `%s' (%s)\n",FileName,strerror(errno));

		return NULL;
	}

	RetCode = Read_Lines(Section, fp, FileName, &Line, Flags);

	fclose(fp);
	return RetCode;
}

/****************************************************************************/

static section *Read_Lines(section *Section, FILE *fp, const char *FileName, int *Line, int Flags)
{
	static int   InSubSection = 0;
	char  String[8192];
	char *Sectionname, *Variable, *Value;
	int   Res;
	int   InInclude = 0;
	section *Ptr = Section;

	if (Section != NULL)
		InInclude = 1;

	while (FGets(String, sizeof(String), fp, Line) != NULL)
	{
		if ((Sectionname = Find_Section(String)) != NULL)
		{
			if ((Ptr = Set_Section(&Section,Sectionname,C_OVERWRITE | C_WARN | Flags)) == NULL)
			{
				free_section(Section);
				return NULL;
			}
		}
		else
		if (Find_Include(&Section,String,FileName,Flags) == 0)
		{
			Ptr = Section;

			if (Ptr != NULL)
				while (Ptr->next != NULL)
					Ptr = Ptr->next;
		/*
			Ptr = NULL;
		*/
		}
		else
		if ((Res = Find_Entry(FileName,*Line,String,&Variable,&Value)) == 0)
		{
			if (Ptr == NULL)
			{
				print_msg("Error in file `%s', line %d: there is no section for variable `%s'!\n",FileName,*Line,Variable);
			}
			else
			{
				if (*Value == C_BEGIN_SUBSECTION && Not_Space(Value+1) == NULL)
				{
					InSubSection++;
					Set_SubSection(Ptr,Variable,Read_Lines(NULL,fp,FileName,Line,Flags),C_OVERWRITE | C_WARN);
					/*
					if (Set_SubSection(Ptr,Variable,Read_Lines(NULL,fp,FileName,Line,Flags),C_OVERWRITE | C_WARN) == NULL)
					{
						free_section(Section);
						return NULL;
					}
					*/
				}
				else
					if (Set_Entry(Ptr,NULL,Variable,Value,C_OVERWRITE | C_WARN) == NULL)
					{
						free_section(Section);
						return NULL;
					}
			}
		}
		else
		if (Res == -1 && *(Kill_Blanks(String)) == C_END_SUBSECTION)
		{
			InSubSection--;
			return Section;
		}
		else
		if (Res == -1)
			print_msg("Error in file `%s', line %d: there is no valid token!\n",FileName,*Line);
	}

	if (InInclude == 0 && InSubSection != 0)
	{
		print_msg("Error in file `%s': Missing a `%c'!\n",FileName,C_END_SUBSECTION);
		free_section(Section);
		return NULL;
	}

	return Section;
}

/****************************************************************************/

static char *Find_Section(char* String)
{
	char *Ptr = NULL;
	static char Help[SHORT_STRING_SIZE];

	strcpy(Help,String);
	String = Kill_Blanks(Help);

	if (*String == '\0' || *String != C_BEGIN_SECTION)
		return NULL;

	Ptr = String + strlen(String)-1;
	if (Ptr == strchr(String,C_END_SECTION))
	{
		*Ptr = '\0';
		Delete_Chars(String+1,S_ALL_QUOTE);
		return String+1;
	}

	return NULL;
}

/****************************************************************************/

static int Find_Entry(const char *FileName, int Line, char* String, char** Variable, char** Value)
{
	char *Ptr = NULL;


	if (String == NULL || Variable == NULL || Value == NULL)
		return -1;

	*Variable = *Value = NULL;

	if ((Ptr = strchr(String,C_EQUAL)) == NULL)
		return -1;
	else
	{
		*Ptr++ ='\0';
		Kill_Blanks(String);
		Delete_Chars(String,S_ALL_QUOTE);

		if (*String == '\0')
		{
			print_msg("Error in file `%s', line %d: There is no variable name!\n", FileName, Line);
			return -2;
		}
		else
			*Variable = String;
	}

	if ((Ptr = Not_Space(Ptr)) == NULL)
	{
		print_msg("Error in file `%s', line %d: There is no value for `%s'!\n", FileName, Line, *Variable);
		return -2;
	}

	*Value    = Ptr;

	return 0;
}

/****************************************************************************/

static int Find_Include(section **Section, char* String, const char *FileName, int Flags)
{
	char *sPtr;
	section *Ptr = NULL;
	char Help1[SHORT_STRING_SIZE];
	char Help2[SHORT_STRING_SIZE] = "";

	strcpy(Help1,String);
	Kill_Blanks(Help1);

	if (!strncasecmp(S_KEY_INCLUDE,Help1,strlen(S_KEY_INCLUDE)) &&
	    Help1[strlen(S_KEY_INCLUDE)] == C_BEGIN_INCLUDE         &&
	    Help1[strlen(Help1)-1]       == C_END_INCLUDE             )
	{
		Ptr = *Section;

		if (Ptr != NULL)
			while (Ptr->next != NULL)
				Ptr = Ptr->next;

		if (*(Help1+strlen(S_KEY_INCLUDE)+1) != C_SLASH)
		{
			if ((sPtr = strrchr(Help2,C_SLASH)) != NULL)
			{
				strcpy(Help2,FileName);
				sPtr[1] = '\0';
			}
		}

		Help1[strlen(Help1)-1] = '\0';
		strcat(Help2,Help1+strlen(S_KEY_INCLUDE)+1);

		if ((Ptr = read_file(Ptr,Help2,Flags & ~C_NO_WARN_FILE)) == NULL)
			return -1;
		else
			if (*Section == NULL)
				*Section = Ptr;

		return 0;
	}

	return -1;
}

/****************************************************************************/

section *Set_Section(section **Section, char *Sectionname, int Flag)
{
	char _Sectionname[SHORT_STRING_SIZE];
	section **Ptr = Section;

	if (Sectionname != NULL)
	{
		strcpy(_Sectionname,Sectionname);
		To_Upper(_Sectionname);
	}
	else
		return NULL;

	while ((*Ptr) != NULL)
	{
		if (!(Flag & C_NOT_UNIQUE) && !strcmp((*Ptr)->name,_Sectionname))
		{
			if (Flag & C_OVERWRITE)
			{
				section *Ptr2 = (*Ptr)->next;
				(*Ptr)->next = NULL;
				free_section(*Ptr);
				*Ptr = Ptr2;

				if (Flag & C_WARN)
					print_msg("Will overwrite section `%s'!\n", _Sectionname);
			}
			else
				return NULL;
		}
		else
			Ptr = &((*Ptr)->next);
	}

	if ((*Ptr = (section*) calloc(1,sizeof(section))) == NULL)
		return NULL;

	if (((*Ptr)->name = strdup(_Sectionname)) == NULL)
	{
		free_section(*Ptr);
		*Ptr = NULL;
		return NULL;
	}

 	return *Ptr;
}

/****************************************************************************/

section *Set_SubSection(section *Section, char *Variable, section *SubSection, int Flag)
{
	char _Variable[SHORT_STRING_SIZE];


	if (Variable != NULL)
	{
		strcpy(_Variable,Variable);
		To_Upper(_Variable);
	}
	else
		return NULL;

	if (Append_Entry(&(Section->entries),_Variable,NULL,SubSection,Flag) == NULL)
		return NULL;
	else
		return SubSection;
}

/****************************************************************************/

entry *Set_Entry(section *Section, char *Sectionname, char *Variable, char *Value, int Flag)
{
	char _Variable[SHORT_STRING_SIZE];
	section  *Ptr;


	if (Variable != NULL)
	{
		strcpy(_Variable,Variable);
		To_Upper(_Variable);
	}
	else
		return NULL;

	if (Sectionname != NULL)
	{
		if ((Ptr = Get_Section(Section,Sectionname)) == NULL)
			return NULL;
	}
	else
		Ptr = Section;

	return Append_Entry(&(Ptr->entries),_Variable,Value,NULL,Flag);
}

/****************************************************************************/

entry* Get_Entry(entry* Entry, char *Variable)
{
	while (Entry != NULL && strcasecmp(Entry->name,Variable))
		Entry = Entry->next;

	return Entry;
}

/****************************************************************************/

section* Get_Section(section* Section, char *Sectionname)
{
	while (Section != NULL && strcasecmp(Section->name,Sectionname))
		Section = Section->next;

	return Section;
}

/****************************************************************************/

section* Get_SubSection(section* Section, char *Variable)
{
	entry*  Ptr;

	if ((Ptr = Get_Entry(Section->entries,Variable)) == NULL)
		return NULL;

	return Ptr->subsection;
}

/****************************************************************************/

static entry* Append_Entry(entry** Entry, char *Variable, char* Value, section *SubSection, int Flag)
{
	if (Entry == NULL)
		return NULL;

	if ((Value != NULL && SubSection != NULL) || (Value == NULL && SubSection == NULL))
		return NULL;

	while ((*Entry) != NULL)
	{
		if (!strcmp((*Entry)->name,Variable))
		{
			if (Flag & C_OVERWRITE)
			{
				entry *Ptr = (*Entry)->next;
				(*Entry)->next = NULL;
				free_entry(*Entry);
				*Entry = Ptr;

				if (Flag & C_WARN)
					print_msg("Will overwrite entry `%s'!\n", Variable);
			}
			else
			if (Flag & C_APPEND && SubSection != NULL && (*Entry)->subsection != NULL)
			{
				section **Section = &((*Entry)->subsection);

				while(*Section != NULL)
					Section = &((*Section)->next);

				*Section = SubSection;
				return *Entry;
			}
			else
				return NULL;
		}
		else
			Entry = &((*Entry)->next);
	}

	if ((*Entry = (entry*) calloc(1,sizeof(entry))) == NULL)
		return NULL;

	if (((*Entry)->name = strdup(Variable)) == NULL)
	{
		free_entry(*Entry);
		*Entry = NULL;
		return NULL;
	}

	if (Value != NULL)
	{
		if (!(Flag & C_ALLOW_LAST_BLANKS))
		{
			int len = strlen(Value)-1;

			while (len >= 0 && isspace(Value[len]))
				Value[len--] = '\0';
		}

		if (((*Entry)->value = strdup(Value)) == NULL)
		{
			free_entry(*Entry);
			*Entry = NULL;
			return NULL;
		}
	}
	else /* SubSection != NULL */
		(*Entry)->subsection = SubSection;

	return *Entry;
}

/****************************************************************************/

static void free_entry(entry *Ptr)
{
	if (Ptr == NULL)
		return;

	free_entry(Ptr->next);
	free_section(Ptr->subsection);
	if (Ptr->name) {
		free(Ptr->name);
		Ptr->name = NULL;
	}
	if (Ptr->value) {
		free(Ptr->value);
		Ptr->value = NULL;
	}
	free(Ptr);
}

/****************************************************************************/

void free_section(section *Ptr)
{
	if (Ptr == NULL)
		return;

	free_section(Ptr->next);
	free_entry(Ptr->entries);
	if (Ptr->name) {
		free(Ptr->name);
		Ptr->name = NULL;
	}
	free(Ptr);
}

/****************************************************************************/

section *Del_Section(section **Section, char *Sectionname)
{
	section **RetCode = Section;
	section *Ptr;

	if (Section == NULL)
		return NULL;

	if (Sectionname != NULL)
		while ((*Section) != NULL && strcmp((*Section)->name,Sectionname))
			Section = &((*Section)->next);

	if (*Section)
	{
		Ptr = *Section;
		if (Section == RetCode)
			RetCode = &((*Section)->next);

		*Section = (*Section)->next;
		Ptr->next = NULL;
		free_section(Ptr);
	}

	Section = RetCode;
	return *RetCode;
}

/****************************************************************************/

static section *Insert_Section(section **main_sec, section **ins_sec, char **variables, int flags)
{
	section *Ptr = NULL;


	if (main_sec == NULL || ins_sec == NULL || *ins_sec == NULL)
	{
		print_msg("%s","One of the sections is emtpy!\n");
		return NULL;
	}

	if ((*ins_sec)->next != NULL)
	{
		print_msg("%s","Can only insert one entry at time!\n");
		return NULL;
	}

	while(*main_sec != NULL)
	{
		if (!Compare_Sections(*main_sec,*ins_sec,variables))
		{
			if ((!(flags & C_NOT_UNIQUE) && variables == NULL) ||
			    (flags & C_OVERWRITE)                            )
			{
				Ptr = *main_sec;
				*main_sec = *ins_sec;
				*ins_sec = NULL;
				(*main_sec)->next = Ptr->next;
				Ptr->next = NULL;
				free_section(Ptr);
				return *main_sec;
			}

		}

		main_sec = &((*main_sec)->next);
	}

	*main_sec = *ins_sec;
	*ins_sec = NULL;
	(*main_sec)->next = NULL;

	return *main_sec;
}

/****************************************************************************/

static int Compare_Sections(section* sec1, section *sec2, char **variables)
{
	int i, Cnt1, Cnt2;
	int found1, found2, Cnt, depth, width, exist = 1;
	char   **array;
	char   **array2;
	section *RetSection   = NULL;
	entry   *RetEntry1    = NULL;
	entry   *RetEntry2    = NULL;
	section *Next1        = NULL;
	section *Next2        = NULL;


	if (sec1 == NULL || sec2 == NULL)
		return -1;

	if (variables == NULL)
	{
		if (!strcmp(sec1->name,sec2->name))
			return 0;
	}
	else
	{
		Next1 = sec1->next;
		sec1->next = NULL;
		Next2 = sec2->next;
		sec2->next = NULL;

		for (i=0; variables[i] != NULL; i++)
		{
			if ((array = String_to_Array(variables[i],C_SLASH)) == NULL)
			{
				sec1->next = Next1;
				sec2->next = Next2;
				return -1;
			}

			found1 = found2 = Cnt = 0;

			while ((array2 = Compare_Section_Get_Path(array,&width,&depth)) != NULL)
			{
				Cnt1 = Cnt2 = 0;

				while (_Get_Section_From_Path(array2,sec1,&RetSection,&RetEntry1,0) == sec1)
				{
					Cnt1++;

					while (_Get_Section_From_Path(array2,sec2,&RetSection,&RetEntry2,0) == sec2)
					{
						Cnt2++;

						if (RetEntry1 != NULL && RetEntry2 != NULL                      &&
						    !strcmp(RetEntry1->name, RetEntry2->name)                   &&
						    ((RetEntry1->value == NULL && RetEntry2->value == NULL) ||
						      !strcmp(RetEntry1->value,RetEntry2->value)              )   )
						 	found1++;
					}
				}

				if (exist && array2[depth-1][0] == C_EXIST && Cnt1 == 0 && Cnt2 == 0)
						found2++;

				if ((++Cnt)%width == 0)
                {
					if(found1 == 0)
						found2 = 0;
					else
						exist = 0;
                }
				if (array != NULL)
				{
					del_Array(array);
					array = NULL;
				}
			}


			if (width == found1 + found2)
			{
				sec1->next = Next1;
				sec2->next = Next2;
				return 0;
			}
		}
	}

	sec1->next = Next1;
	sec2->next = Next2;
	return -1;
}

/****************************************************************************/

static char** Compare_Section_Get_Path(char **array, int *retsize, int *retdepth)
{
	int i,j;
	static int     lsize;
	static int     index;
	static char ***arrayptr = NULL;
	static char  **retptr   = NULL;
	static int    *indexptr = NULL;


	if (array != NULL)
	{
		for (index=0; array[index] != NULL; index++);

		if ((indexptr = (int*) calloc(index,sizeof(int))) == NULL)
		{
			print_msg("%s","Can't allocate memory!\n");
			return NULL;
		}

		if ((retptr = (char**) calloc(index+1,sizeof(char*))) == NULL)
		{
			print_msg("%s","Can't allocate memory!\n");
			return NULL;
		}

		if ((arrayptr = (char***) calloc(index,sizeof(char**))) == NULL)
		{
			print_msg("%s","Can't allocate memory!\n");
			return NULL;
		}

		for (i=0; array[i] != NULL; i++)
			if ((arrayptr[i] = String_to_Array(array[i],C_OR)) == NULL)
			{
				print_msg("%s","Can't allocate memory!\n");
				return NULL;
			}

		for (lsize=0; arrayptr[index-1][lsize] != NULL; lsize++);
	}

	*retsize  = lsize;
	*retdepth = index;

	if (arrayptr == NULL)
		return NULL;


	if (arrayptr[0][indexptr[0]] != NULL)
	{
		i = index-1;

		while(i >= 0)
		{
			if (i > 0 && arrayptr[i][indexptr[i]] == NULL)
			{
				indexptr[i] = 0;
				i--;
			}
			else
			{
				for (j = index-1; j >= 0; j--)
					retptr[j] = arrayptr[j][indexptr[j]];

				indexptr[i]++;

				while (i >= 0)
				{
					if (i > 0 && arrayptr[i][indexptr[i]] == NULL)
					{
						indexptr[i] = 0;
						indexptr[i-1]++;
					}

					i--;
				}

				break;
			}
		}
	}
	else
	{
		for (i=0; i < index; i++)
			del_Array(arrayptr[i]);

		free(arrayptr);
		free(retptr);
		free(indexptr);

		arrayptr = NULL;
		retptr   = NULL;
		indexptr = NULL;
	}

	return retptr;
}

/****************************************************************************/

static int Append_Sections(section **main_sec, section *app_sec)
{
	while(*main_sec != NULL)
		main_sec = &((*main_sec)->next);

	*main_sec = app_sec;
	return 0;
}

/****************************************************************************/

static int Merge_Sections(section **main_sec, section **ins_sec, char **variables, int flags)
{
	if (main_sec == NULL)
	{
		print_msg("%s","Main section is emtpy!\n");
		return -1;
	}

	if (ins_sec == NULL)
	{
		print_msg("%s","Insert section is emtpy!\n");
		return -1;
	}

	if (*ins_sec == NULL)
	{
		print_msg("%s","Insert section element is emtpy!\n");
		return -1;
	}

	if((*ins_sec)->next != NULL)
		Merge_Sections(main_sec,&((*ins_sec)->next),variables,flags);

	Insert_Section(main_sec,ins_sec,variables,flags);

	if (*ins_sec != NULL)
	{
		free_section(*ins_sec);
		*ins_sec = NULL;
	}

	return 0;
}

/****************************************************************************/

static void free_cfile(cfile **cfiles)
{
	cfile **cptr;

	if (cfiles != NULL)
	{
		cptr = cfiles;
		while (*cptr != NULL)
		{
			if ((*cptr)->name)
				free((*cptr)->name);
			free(*cptr);
			cptr++;
		}
		free(cfiles);
	}

	return;
}

/****************************************************************************/

/* Die Variable flags sollte _immer_ auf 'C_OVERWRITE|C_NOT_UNIQUE' gesetzt
   werden, anderes macht hier zur Zeit keinen Sinn.
   main_sec darf _NIE_ unintilisiert sein und muss beim ersten mal NULL
   enthlten!!!!
*/

int read_files(section **main_sec, char** files, int *fileflag, char **variables, int flags)
{
	int newread = 0;
	static cfile **cfiles = NULL;
	static struct stat FileStat;
	section *ins_sec = NULL;
	int i;

	if (files != NULL)
	{
		newread = 1;

		free_cfile(cfiles);
		cfiles = NULL;

		for (i=0; files[i] != NULL; i++)
		{
			if ((cfiles = (cfile**) realloc(cfiles,sizeof(cfile*)*(i+2))) == NULL)
			{
				print_msg("%s","Can't allocate memory!\n");
				return -1;
			}

			if ((cfiles[i] = (cfile*) calloc(1,sizeof(cfile))) == NULL)
			{
				print_msg("%s","Can't allocate memory!\n");
				return -1;
			}

			cfiles[i+1] = NULL;

			if (stat(files[i],&FileStat) != 0 && !(flags & C_NO_WARN_FILE))
			{
				print_msg("Can't open file `%s': %s!\n",files[i],strerror(errno));
				return -1;
			}

			cfiles[i]->flag = fileflag[i];
			cfiles[i]->name = strdup(files[i]);
			cfiles[i]->modtime = FileStat.st_mtime;
		}

	}
	else
	{
		if (cfiles == NULL)
		{
			print_msg("%s","There is no file!\n");
			return -1;
		}
		else
		{
			for (i=0; cfiles[i] != NULL; i++)
			{
				if (stat(cfiles[i]->name,&FileStat) != 0)
				{
					if (!(flags & C_NO_WARN_FILE))
					{
						print_msg("Can't open file `%s': %s!\n",cfiles[i]->name,strerror(errno));
						return -1;
					}
				}
				else
				{
					if (cfiles[i]->modtime != FileStat.st_mtime)
					{
						cfiles[i]->modtime = FileStat.st_mtime;
						newread = 1;
						break;
					}
				}
			}
		}
	}

	if (newread)
	{
		free_section(*main_sec);
		*main_sec = NULL;

		for (i=0; cfiles[i] != NULL; i++)
		{
			if (*main_sec == NULL)
			{
				*main_sec = read_file(NULL,cfiles[i]->name,flags);
			}
			else
			{
				if ((ins_sec = read_file(NULL,cfiles[i]->name,flags)) != NULL)
					switch(cfiles[i]->flag)
					{
						case APPEND_FILE: Append_Sections(main_sec,ins_sec);
						                  break;
						case MERGE_FILE :
						default         : Merge_Sections(main_sec,&ins_sec,variables,flags);
						                  break;
					}
			}
		}
	}

	return ((*main_sec != NULL && newread)?1:(*main_sec == NULL?-1:0));
}

/****************************************************************************/

int Filter_Sections(section **sec, char** path)
{
	int i;
	char   **array;
	section *secptr;
	section *retsec = NULL;
	entry   *retent = NULL;


	if (path == NULL || path[0] == NULL)
		return -1;

	if (sec == NULL)
	{
		print_msg("%s","Section is emtpy!\n");
		return -1;
	}

	for (i=0; path[i] != NULL; i++)
	{
		if ((array = String_to_Array(path[i],C_SLASH)) == NULL)
			return -1;

		secptr = *sec;

		while ((secptr = _Get_Section_From_Path(array,secptr,&retsec,&retent,F_TAG)) != NULL);

		del_Array(array);
	}

	return del_untagged_items(sec);
}

/****************************************************************************/

static int del_untagged_items(section **sec)
{
	int del = 0;
	entry **ent;
	entry *Ptr;
	section *Ptr2;

	if (*sec == NULL)
		return -1;

	while (*sec != NULL)
	{
		if ((*sec)->flag != F_TAGGED)
		{
			Ptr2 = *sec;
			*sec = (*sec)->next;
			Ptr2->next = NULL;
			free_section(Ptr2);
		}
		else
		{
			(*sec)->flag = F_NOT_TAGGED;

			ent = &((*sec)->entries);

			while(*ent != NULL)
			{
				del = 0;

				if ((*ent)->flag == F_TAGGED && (*ent)->subsection != NULL)
				{
					del_untagged_items(&((*ent)->subsection));

					if ((*ent)->subsection == NULL)
						del = 1;
				}

				if ((*ent)->flag != F_TAGGED || del == 1)
				{
					Ptr = *ent;
					*ent = (*ent)->next;
					Ptr->next = NULL;
					free_entry(Ptr);
				}
				else
				{
					(*ent)->flag = F_NOT_TAGGED;
					ent = &((*ent)->next);
				}
			}

			if ((*sec)->entries == NULL)
			{
				Ptr2 = *sec;
				*sec = (*sec)->next;
				Ptr2->next = NULL;
				free_section(Ptr2);
			}
			else
				sec = &((*sec)->next);
		}
	}

	return 0;
}

/****************************************************************************/

static section* Get_Section_From_Path(section* NewSection, char *Path, entry **Entry)
{
	static section *RootSection = NULL;
	static section *RetSection = NULL;
	static entry *RetEntry = NULL;
	static char **array = NULL;


	if (Path != NULL && NewSection != NULL)
	{
		RootSection = NewSection;
		RetSection = NULL;
		RetEntry = NULL;

		if (array != NULL)
			del_Array(array);

		if ((array = String_to_Array(Path,C_SLASH)) == NULL)
			return NULL;

		if (array[0] == NULL)
			return NULL;
	}
	else if (Path == NULL && NewSection == NULL    &&
	         (RootSection == NULL || array == NULL)  )
		return NULL;
	else if (Path != NULL || NewSection != NULL)
		return NULL;


	if ((RootSection = _Get_Section_From_Path(array,RootSection,&RetSection,&RetEntry,0)) == NULL)
		RetSection = NULL;

	if (Entry != NULL)
		*Entry = RetEntry;

	return RetSection;
}

/****************************************************************************/

static entry* _Get_Entry_From_Path(char **array, entry* Entry, section **RetSection, entry **RetEntry, int flags)
{
	int index;
	int found = 0;
	int found_first = 0;
	char **array2 = NULL;
	char  *Ptr;


	if (array != NULL && array[0] == NULL)
		return NULL;

	if ((array2 = String_to_Array(array[0],C_OR)) == NULL)
			return NULL;

	while(found == 0 && Entry != NULL)
	{
		index = 0;

		if (Entry->name != NULL)
		{
			while(array2[index] != NULL && found == 0)
			{
				Ptr = (array2[index][0] == C_EXIST?array2[index]+1:array2[index]);

				if (match(Ptr,Entry->name,F_IGNORE_CASE) == 0)
				{
					if (array[1] == NULL)
					{
						if (Entry->value != NULL && *RetEntry != Entry &&
						    (*RetEntry == NULL || found_first != 0)      )
						{
							found = 1;

							if (flags == F_TAG)
								Entry->flag = F_TAGGED;

							*RetEntry = Entry;
						}
						else
							if (*RetEntry == Entry)
								found_first = 1;
					}
					else
					{
						if (_Get_Section_From_Path(array+1,Entry->subsection,RetSection,RetEntry,flags) != NULL)
						{
							found = 1;

							if (flags == F_TAG)
								Entry->flag = F_TAGGED;

							if (array[2] == NULL)
								*RetEntry = Entry;
						}
					}
				}

				index++;
			}
		}

		if (found == 0)
			Entry = Entry->next;
	}

	if (found == 0 && found_first != 0)
		*RetEntry = NULL;

	del_Array(array2);
	return Entry;
}

/****************************************************************************/

static section* _Get_Section_From_Path(char **array, section* Section, section **RetSection, entry **RetEntry, int flags)
{
	int index;
	int found = 0;
	int found_first = 0;
	char **array2 = NULL;
	char  *Ptr;


	if (array != NULL && array[0] == NULL)
		return NULL;

	if ((array2 = String_to_Array(array[0],C_OR)) == NULL)
			return NULL;

	while(found == 0 && Section != NULL)
	{
		index = 0;

		if (Section->name != NULL)
		{
			while(array2[index] != NULL && found == 0)
			{
				Ptr = (array2[index][0] == C_EXIST?array2[index]+1:array2[index]);

		    if (match(Ptr,Section->name,F_IGNORE_CASE) == 0)
		  	{
					if (array[1] == NULL)
					{
						if (*RetSection != Section && (*RetSection == NULL || found_first != 0))
						{
							found = 1;

							if (flags == F_TAG)
								Section->flag = F_TAGGED;

							*RetSection = Section;
						}
						else
							if (*RetSection == Section)
								found_first = 1;
					}
					else
					{
						if (_Get_Entry_From_Path(array+1,Section->entries,RetSection,RetEntry,flags) != NULL)
						{
							found = 1;

							if (flags == F_TAG)
								Section->flag = F_TAGGED;

							if (array[2] == NULL)
								*RetSection = Section;
						}
					}
				}

				index++;
			}
		}

		if (found == 0)
			Section = Section->next;
	}

	if (found == 0 && found_first != 0)
		*RetSection = NULL;

	del_Array(array2);
	return Section;
}

/****************************************************************************/

section* Get_Section_Match(section* Section, char *Path,
                           char* Value, int  (*_match)(char*, char*), entry **RetEntry)
{
	entry *Entry;


	while ((Section = Get_Section_From_Path(Section,Path,&Entry)) != NULL)
	{
		if (RetEntry != NULL)
			*RetEntry = NULL;

		if (Entry->subsection != NULL)
		{
			if (Value == NULL)
			{
				if (RetEntry != NULL)
					*RetEntry = Entry;

				return Entry->subsection;
			}
/* Die naechsten Zeilen sind fuer Syntax-DAU's auskommentiert:
   NUMBER={
     [blabla]
   }
   fuehrt zum Abbruch!

			else
				return NULL;
*/
		}
		else
		{
			if (Value == NULL)
				return NULL;
			else
			{
				if ((_match == NULL && !strcmp(Entry->value,Value)) ||
				    !_match(Entry->value,Value)                       )
				{
					if (RetEntry != NULL)
						*RetEntry = Entry;

					return Section;
				}
			}
		}

		Section = NULL;
		Path = NULL;
	}

	return NULL;
}

/****************************************************************************/

char *Get_Value(section *Section, char *Path)
{
	entry *Entry;

	while ((Section = Get_Section_From_Path(Section,Path,&Entry)) != NULL)
	{
		if (Entry->value != NULL)
			return Entry->value;

		Section = NULL;
		Path = NULL;
	}

	return NULL;
}

/****************************************************************************/

static int Set_Ret_Code(char *Value, int Type, void **Pointer)
{
	int RetCode = -1;


	if (Pointer == NULL || Value == NULL)
		return -1;

	switch (Type)
	{
		case R_TYPE_LONG  :
		case R_TYPE_INT   : if (is_integer(Value,(long int*) Pointer))
		                    	RetCode = 0;
		                    else
		                    	*((long int*)Pointer) = 0;
		                    break;

		case R_TYPE_DOUBLE: if (is_double(Value,(double*) Pointer))
		                    	RetCode = 0;
		                    else
		                    	*((double*)Pointer) = 0.0;
		                    break;

		case R_TYPE_CHAR  : *((char*)Pointer) = Value[0];
		                    if (strlen(Value) == 1)
		                    	RetCode = 0;
		                    break;

		case R_TYPE_STRING: *((char**)Pointer) = Value;
		                    RetCode = strlen(Value);
		                    break;

		default           : break;
	}

	return RetCode;
}

/****************************************************************************/

int _Get_Type_Value(section *Section, char *Path, int Type, void **Pointer)
{
	register int   RetCode = -1;
	auto     char *Ptr;


	while (RetCode == -1 && (Ptr = Get_Value(Section,Path)) != NULL)
	{
		RetCode = Set_Ret_Code(Ptr,Type,Pointer);
		Section = NULL;
	}

	return RetCode;
}

/****************************************************************************/

int _Get_Type_Match(section *Section, char *Path, char* Pattern,
                            int  (*_match)(char*, char*), int Type, void **Pointer)
{
	entry *Entry;


	if (Get_Section_Match(Section,Path,Pattern,_match,&Entry) == NULL ||
	    Entry != NULL || Entry->name == NULL                            )
		return -1;

	return Set_Ret_Code(Entry->name,Type,Pointer);
}

/****************************************************************************/

static char* Delete_Chars(char *String, char *Quote)
{
	char *Ptr = String;

	if (Ptr == NULL)
		return NULL;

	while ((Ptr = Strpbrk(Ptr,Quote)) != NULL)
	{
		print_msg("Invalid character (`%s') in string `%s'. Character ignored!\n",Quote,String);

		do
			*Ptr = *(Ptr + 1);
		while(*Ptr++ != '\0');
	}

	return String;
}

/****************************************************************************/

int Replace_Variables(section *Section)
{
	entry *Entry;
	char  *Ptr = NULL;


	while(Section != NULL)
	{
		Entry = Section->entries;

		while(Entry != NULL)
		{
			if (Entry->value != NULL)
			{
				if ((Ptr = Replace_Variable(Entry->value)) != NULL)
				{
					if ((Ptr = strdup(Ptr)) == NULL)
					{
						print_msg("%s","Can't allocate memory!\n");
						return -1;
					}

					free(Entry->value);
					Entry->value = Ptr;
				}
			}
			else
				if (Entry->subsection != NULL)
					if (Replace_Variables(Entry->subsection) != 0)
						return -1;

			Entry = Entry->next;
		}

		Section = Section->next;
	}

	return 0;
}

/****************************************************************************/

