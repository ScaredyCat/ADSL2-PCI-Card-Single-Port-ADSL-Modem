/* $Id: libtools.c,v 1.10 1999/11/03 16:13:36 paul Exp $
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
 * $Log: libtools.c,v $
 * Revision 1.10  1999/11/03 16:13:36  paul
 * Added { } to suppress egcs warnings.
 *
 * Revision 1.9  1998/10/18 20:13:51  luethje
 * isdnlog: Added the switch -K
 *
 * Revision 1.8  1998/10/13 22:17:22  luethje
 * isdnlog: evaluate the variable PATH for program starts.
 *
 * Revision 1.7  1998/10/13 21:53:36  luethje
 * isdnrep and lib: bugfixes
 *
 * Revision 1.6  1997/05/09 23:31:09  luethje
 * isdnlog: new switch -O
 * isdnrep: new format %S
 * bugfix in handle_runfiles()
 *
 * Revision 1.5  1997/04/15 00:20:18  luethje
 * replace variables: some bugfixes, README comleted
 *
 * Revision 1.4  1997/04/10 23:32:35  luethje
 * Added the feature, that environment variables are allowed in the config files.
 *
 * Revision 1.3  1997/03/20 00:28:02  luethje
 * Inserted lines into the files for the revision tool.
 *
 */

#define _LIB_TOOLS_C_

/****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <fnmatch.h>
#include <ctype.h>
#include <unistd.h>

#include "libtools.h"

/****************************************************************************/

#define SET_BEGIN_VAR 1

/****************************************************************************/

static int  (*print_msg)(const char *, ...) = printf;

/****************************************************************************/

void set_print_fct_for_libtools(int (*new_print_msg)(const char *, ...))
{
	print_msg = new_print_msg;
}

/****************************************************************************/

char *Not_Space(char *String)
{
	while (isspace(*String)) String++;

	if (*String == '\0')
		String = NULL;

	return String;
}

/****************************************************************************/

char *To_Upper (char *String)
{
	char *Ptr = String;

	while (*Ptr != '\0')
	{
		*Ptr = toupper(*Ptr);
		Ptr++;
	}

	return String;
}

/****************************************************************************/

char *Kill_Blanks(char *String)
{
	int Cnt1 = 0, Cnt2 = 0;


	while (String[Cnt1] != '\0')
	{
		if (!isspace(String[Cnt1]))
			String[Cnt2++] = String[Cnt1];

		Cnt1++;
	}

	String[Cnt2] = '\0';

	return String;
}

/****************************************************************************/

char *FGets(char *String, int StringLen, FILE *fp, int *Line)
{
	int Len;
	char *RetCode = NULL;
	char *Ptr = NULL;
	char *Help = NULL;


	if ((Help = (char*) alloca(StringLen*sizeof(char))) == NULL)
		return NULL;

	*String ='\0';

	while ((RetCode = fgets(Help, StringLen - strlen(String), fp)) != NULL)
	{
		(*Line)++;

		if ((Len = strlen(Help)) > 0 && Help[Len-1]  == '\n' )
			Help[Len-1] = '\0';

		if ((Ptr = Check_Quote(Help,S_COMMENT,QUOTE_DELETE)) != NULL)
			*Ptr = '\0';

		strcat(String,Help);

		if ((Len = strlen(String)) > 0 && String[Len-1]  == C_QUOTE_CHAR )
			String[Len-1] = '\0';
		else
			break;
	}

	Ptr = String;
	while (isspace(*Ptr)) Ptr++;

	if (*Ptr =='\0' && RetCode != NULL)
		RetCode = FGets(String,StringLen,fp,Line);
	else
	{
		strcpy(Help,Ptr);
		strcpy(String,Help);
	}

	return RetCode;
}

/****************************************************************************/

char *Check_Quote(char *String, char *Quote, int Flag)
{
  char *Ptr = String;
  char *Ptr2;

  if (Ptr == NULL)
    return NULL;

  while ((Ptr = Strpbrk(Ptr,Quote)) != NULL)
  {
  	if (Ptr != String && *(Ptr - 1) == C_QUOTE_CHAR)
  	{
      if (Flag == QUOTE_IGNORE)
      {
      	Ptr++;
      }
      else
      if (Flag == QUOTE_DELETE)
      {
      	Ptr2 = Ptr - 1;

        do
          *Ptr2 = *(Ptr2 + 1);
        while(*Ptr2++ != '\0');
      }
      else
        return NULL;
    }
    else
    	break;
  }

  return Ptr;
}

/****************************************************************************/

const char* Quote_Chars(char *string)
{
	int i;
	int Index = 0;
  static char *Ptr = NULL;
  char *Help = NULL;

	if (Ptr != NULL)
		free(Ptr);

	Ptr = strdup(string);

	if (string == NULL)
	{
		print_msg("%s!\n","Got a NULL string!\n");
		return "";
	}

	while((Help = Strpbrk(Ptr+Index,S_COMMENT)) != NULL)
	{
		Index = Help - Ptr;

		if ((Ptr = (char*) realloc(Ptr,(strlen(Ptr)+2)*sizeof(char))) == NULL)
		{
			print_msg("%s!\n","Can not alllocate memory!\n");
			return "";
		}

		for (i = strlen(Ptr); i >= Index; i--)
			Ptr[i+1] = Ptr[i];

		Ptr[Index] =  C_QUOTE_CHAR;

		Index += 2;
	}

	return Ptr;
}

/****************************************************************************/

/* Als Workaround fuer strpbrk():  seltsame Abstuerze ;-(  */

char *Strpbrk( const char* Str, const char* Accept )
{
	int i;
	int len;
	int first = -1;
	int helpfirst = -1;
	int pos = -1;

	char *RetCode = NULL;


	if (Str == NULL || Accept == NULL)
		return NULL;

	len = strlen(Accept);

	for (i = 0; i < len; i++)
	{
		if ((RetCode = strchr(Str,Accept[i])) != NULL)
		{
			if ((helpfirst = RetCode - Str) < first || first == -1)
			{
				pos = i;
				first = helpfirst;
			}
		}
	}

	if (pos == -1)
		return NULL;
	else
		RetCode = (char*) (Str + first);

	/* Alte Zeile
	while (*Accept != '\0' && (RetCode = strchr(Str,*Accept++)) == NULL);
	*/

	return RetCode;
}

/****************************************************************************/

char **String_to_Array(char* String, char Trenn)
{
	char  *Ptr     = String;
	char **RetCode = NULL;
	char  *Ptr1    = NULL;
	char  *Ptr2    = NULL;
	int Cnt = 2;

	if (*String == '\0')
		return NULL;

	while((Ptr = strchr(Ptr,Trenn)) != NULL)
	{
		Cnt++;
		Ptr++;
	}

	RetCode = (char**) calloc(Cnt,sizeof(char*));
	Ptr     = strdup(String);

	if (RetCode == NULL || Ptr == NULL)
		return NULL;

	Cnt = 0;
	Ptr1 = Ptr;

	do
	{
		Ptr2 = strchr(Ptr1,Trenn);

		if (Ptr2)
			*Ptr2++ = '\0';

		RetCode[Cnt] = (char*) calloc(strlen(Ptr1)+1,sizeof(char));

		if (RetCode == NULL)
			return RetCode;

		strcpy(RetCode[Cnt++],Ptr1);
		Ptr1 = Ptr2;
	}
	while (Ptr1 != NULL);

	free(Ptr);
	return RetCode;
}

/****************************************************************************/

void del_Array(char **Ptr)
{
	int Cnt = 0;

	if (Ptr)
	{
		while (Ptr[Cnt])
			free(Ptr[Cnt++]);

		free (Ptr);
	}
}

/****************************************************************************/

int _append_element(void ***elem, void *ins)
{
	int num = 2;
	void **Ptr = *elem;


	if (Ptr != NULL)
		while(*Ptr != NULL)
			num++,Ptr++;

	if ((*elem = (void**) realloc(*elem,sizeof(void*)*num)) == NULL)
	{
		print_msg("Can not alloc memory!\n");
		return -1;
	}

	(*elem)[num-2] = ins;
	(*elem)[num-1] = NULL;
	return 0;
}

/****************************************************************************/

int _delete_element(void ***elem, int deep)
{
	void **Ptr = *elem;

	if (deep < 0)
		return -1;

	if (Ptr == NULL)
		return 0;

	if (deep != 0)
		while(*Ptr != NULL)
			delete_element(Ptr++,deep-1);

	free(*elem);
	*elem = NULL;
	
	return 0;
}

/****************************************************************************/

int match(register char *p, register char *s, int flags)
{
	char *ptr1;
	char *ptr2;

	if ((flags & F_IGNORE_CASE) || (flags & F_NO_HOLE_WORD))
	{
		if ((flags & F_IGNORE_CASE) && !(flags & F_NO_HOLE_WORD) &&
		    !strcasecmp(p,s)                                       )
			return 0;

		if ((ptr1 = (char*) alloca((strlen(p)+3)*sizeof(char))) == NULL)
			return -1;

		if ((ptr2 = (char*) alloca((strlen(s)+3)*sizeof(char))) == NULL)
			return -1;

		strcpy(ptr1,p);
		strcpy(ptr2,s);

		if (flags & F_IGNORE_CASE)
		{
			To_Upper(ptr1);
			To_Upper(ptr2);
		}

		if (flags & F_NO_HOLE_WORD)
		{
			char string[SHORT_STRING_SIZE];

			sprintf(string,"*%s*",ptr1);
			strcpy(ptr1,string);
		}

		flags &= ~F_IGNORE_CASE;
		flags &= ~F_NO_HOLE_WORD;

		p = ptr1;
		s = ptr2;
	}
	else
		if (!strcmp(p,s))
			return 0;

#ifdef OWN_MATCH
	return _match(p,s);
#else
	return fnmatch(p,s,flags);
#endif
} /* match */

/****************************************************************************/

#ifdef OWN_MATCH
int _match(char* p,char* s)
{
  register int sc, pcc;


  if (!*s || !*p)
    return(-1);

  while ((pcc = *p++ & QCMASK)) {
    sc = *s++ & QMASK;

    switch (pcc) {
      case '[' : if (!(p = cclass(p, sc)))
	           return(-1);
	         break;

      case '?' : if (!sc)
	           return(-1);
	         break;

      case '*' : s--;
	         do {
	           if (!*p || !_match(p, s))
	             return(0);
	         } while (*s++);
	         return(-1);

      default  : if (sc != (pcc &~QUOTE))
	           return(-1);
    }
  }

  return(*s);
} /* _match */
#endif

/****************************************************************************/

int is_double (char *string, double *value)
{
	double dummy2 = 0;
	char* dummy;

	if (*string == '\0')
		return 1;

	dummy = (char*) alloca((strlen(string)+1)*sizeof(char));

	if (value == NULL)
		value = &dummy2;

	return (sscanf(string,"%lf%s",value,dummy) == 1);
}

/****************************************************************************/

int is_integer (char *string, long int *value)
{
	long int dummy2 = 0;
	char* dummy;

	if (*string == '\0')
		return 1;

	dummy = (char*) alloca((strlen(string)+1)*sizeof(char));

	if (value == NULL)
		value = &dummy2;

	return (sscanf(string,"%ld%s",value,dummy) == 1);
}

/****************************************************************************/

char *Replace_Variable(char *String)
{
	static char *RetCode = NULL;
	char *Begin = NULL;
	char *Var = NULL;
	char *End = NULL;
	char *Value = NULL;
	char *Ptr = String;
	int cnt = 0;
	int num = 0;


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
		print_msg("%s!\n","Error: Can not allocate memory!\n");
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
		if ((num = sscanf(Ptr+1,"%[0-9a-zA-Z]%[^\n]",Var,End))   >= 1 ||
		    (num = sscanf(Ptr+1,"{%[0-9a-zA-Z]}%[^\n]",Var,End)) >= 1   )
		{
			if ((Value = getenv(Var)) != NULL)
			{
				free(Begin);

				if ((Begin = strdup(RetCode)) == NULL)
				{
					print_msg("%s!\n","Error: Can not allocate memory!\n");
					return NULL;
				}

				Begin[Ptr-RetCode] = '\0';

				if ((RetCode = (char*) realloc(RetCode,sizeof(char)*strlen(RetCode)+strlen(Value)-strlen(Var))) == NULL)
				{
					print_msg("%s!\n","Error: Can not allocate memory!\n");
					return NULL;
				}

				if (num == 1)
					*End = '\0';

				sprintf(RetCode,"%s%s%s",Begin,Value,End);

				free(Var);
				free(End);

				if ((Var   = strdup(RetCode)) == NULL ||
				    (End   = strdup(RetCode)) == NULL   )
				{
					print_msg("%s!\n","Error: Can not allocate memory!\n");
					return NULL;
				}

				cnt--;
			}
			else
			{
				*Ptr = SET_BEGIN_VAR;
				cnt--;

				print_msg("Warning: Unknown variable `%s'!\n",Var);
			}
		}
		else
			*Ptr = SET_BEGIN_VAR;
	}

	if (cnt)
		print_msg("Warning: Invalid token in string `%s'!\n",String);

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

/****************************************************************************/

#define MAX_PREC 9

char *int2str(int value, int prec)
{
	static char RetCode[MAX_PREC+1];

	if (prec < 0 || prec > MAX_PREC)
	{
		print_msg("Error: precision %d is out of range (0-%d)!\n",prec,MAX_PREC);
		*RetCode = '\0';
	}
	else
		sprintf(RetCode,"%*d",prec,value);

	return RetCode;
}

/****************************************************************************/

char *Strncat(char *dest, const char *src, int len)
{
	int destlen = strlen(dest);

	return Strncpy(dest+destlen,src,len-destlen);
}

/****************************************************************************/

char *Strncpy(char *dest, const char *src, int len)
{
	int l = strlen(src);

	if (l > len - 1)
		l = len - 1;

	strncpy(dest,src,l);

	dest[l] = '\0';

	return dest;
}

/****************************************************************************/

const char *Pathfind(const char *path, const char *name, char *mode)
{
	static char file[PATH_MAX];
	char _path[PATH_MAX];
	int _mode = 0;
	char *ptr = _path;


	if (name == NULL)
		return NULL;

	if (mode != NULL)
		while (*mode)
		{
			switch(*mode++)
			{
				case 'x': _mode |= X_OK;
				          break;
				case 'w': _mode |= W_OK;
				          break;
				case 'r': _mode |= R_OK;
				          break;
				default :
			}
		}

	if (strchr(name,C_SLASH) != NULL)
    {
		if (!access(name,_mode))
			return name;
		else
			return NULL;
    }
	if (path == NULL)
	{
		if ((ptr = getenv(PATH_ENV)) == NULL)
			return NULL;

		Strncpy(_path,ptr,PATH_MAX);
		ptr = _path;
		_mode |= X_OK;
	}
	else
		Strncpy(_path,path,PATH_MAX);

	while((ptr = strtok(ptr,":")) != NULL)
	{
		snprintf(file,PATH_MAX-1,"%s/%s",ptr,name);
		if (!access(file,_mode))
			return file;

		ptr = NULL;
	}

	return NULL;
}

/****************************************************************************/

