/*
** $Id: rc.c,v 1.5 1998/09/18 15:09:02 michael Exp $
**
** Copyright 1996-1998 Michael 'Ghandi' Herold <michael@abadonna.mayn.de>
*/

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <fnmatch.h>

#include "rc.h"
#include "log.h"
#include "stringutils.h"

/** Prototypes ***********************************************************/

static unsigned char *rc_strip(unsigned char *, int);

/*************************************************************************/
/** rc_read():		Liest eine Konfiguration vom Typ <name>=<value> ein.	**/
/*************************************************************************/
/** => rc			Zeiger auf die Konfigurations-Struktur.					**/
/** => rcname		Name der Konfiguration die eingelesen werden soll.		**/
/** => section		Sektion zu der gesprungen werden soll.						**/
/**																							**/
/** <=				0 wenn die Konfiguration eingelesen wurde, -1 bei		**/
/**					einem Fehler und -2 wenn die Sektion nicht gefunden	**/
/**					wurde.																**/
/*************************************************************************/

int rc_read(struct vboxrc *rc, unsigned char *rcname, unsigned char *section)
{
	unsigned char	rctmpln[VBOX_MAX_RCLINE_SIZE + 1];
	unsigned char *rctmplp;
	FILE          *rctxtio;
	int	         rcerror;
	int	         rcsjump;
	unsigned char *stop;
	unsigned char *name;
	unsigned char *args;

	if (section) section = xstrtoupper(section);

	log_line(LOG_D, "Parsing \"%s\" [%s]...\n", rcname, ((char *)section ? (char *)section : "global"));

	rcerror = 0;
	rcsjump = 0;
	
	if ((rctxtio = fopen(rcname, "r")))
	{
		while ((fgets(rctmpln, VBOX_MAX_RCLINE_SIZE, rctxtio)) && (rcerror == 0))
		{
			rctmpln[strlen(rctmpln) - 1] = '\0';

			rctmplp = rc_strip(rctmpln, 1);

			if (*rctmplp == '\0') continue;

			if ((section) && (!rcsjump))
			{
				if ((strlen(rctmplp) > 1) && (*rctmplp == '['))
				{
					rctmplp[strlen(rctmplp) - 1] = '\0';

					rctmplp++;
				}

				rctmplp = xstrtoupper(rctmplp);

				if (fnmatch(rctmplp, section, 0) == 0)
				{
					log(LOG_D, "Section match \"[%s]\"...\n", rctmplp);

					rcsjump = 1;
				}

				continue;
			}

			if ((section) && (rcsjump))
			{
				if (*rctmplp == '[') break;
			}

			if ((stop = index(rctmplp, '=')))
			{
				*stop++ = '\0';

				name = rc_strip(rctmplp, 0);
				args = rc_strip(stop	  , 0);

				rc_set_entry(rc, name, args);
			}
			else log(LOG_E, "Invalid line \"%s\".\n", rctmplp);
		}

		fclose(rctxtio);
	}
	else
	{
		log(LOG_E, "Can't open \"%s\" (%s).\n", rcname, strerror(errno));

		rcerror = -1;
	}

	if ((rcerror == 0) && (section) && (rcsjump == 0))
	{
		log_line(LOG_E, "Section \"%s\" not found.\n", section);

		rcerror = -2;
	}

	return(rcerror);
}

/************************************************************************* 
 ** rc_strip():	Strips comments, trailing and leading whitespaces.		**
 *************************************************************************
 ** => line			Pointer to line													**
 ** => strp			Boolean if you want to strip comments						**
 **																							**
 ** The call return a pointer to the stripped line.							**
 *************************************************************************/

unsigned char *rc_strip(unsigned char *line, int strp)
{
	char *stop;

	if (strlen(line) > 0)
	{
		while (isspace(*line)) line++;

		if (strp)
		{
			if ((stop = rindex(line, '#'))) *stop = '\0';
		}

		while (strlen(line) > 0)
		{
			if (!isspace(line[strlen(line) - 1])) break;

			line[strlen(line) - 1] = '\0';
		}
	}

	return(line);
}

/*************************************************************************/
/** rc_free():	Gibt den Speicher einer Konfigurations-Struktur frei.		**/
/*************************************************************************/
/** => rc		Zeiger auf die Konfigurations-Struktur.						**/
/*************************************************************************/

void rc_free(struct vboxrc *rc)
{
	int i = 0;
	
	while (rc[i].name)
	{
		if (rc[i].value) free(rc[i].value);
		
		rc[i].value = NULL;
	
		i++;
	}
}

/*************************************************************************/
/** rc_get_entry():	Gibt den Wert eines Konfigurations-Eintrags zu-		**/
/**						rück.																**/
/*************************************************************************/
/** => rc				Zeiger auf die Konfigurations-Struktur.				**/
/** => name				Name des Eintrags dessen Wert man haben möchte.		**/
/**																							**/
/** <=					Wert des Eintrages oder NULL.								**/
/*************************************************************************/

unsigned char *rc_get_entry(struct vboxrc *rc, unsigned char *name)
{
	int i = 0;

	while (rc[i].name)
	{
		if (strcasecmp(rc[i].name, name) == 0) return(rc[i].value);
	
		i++;
	}
	
	return(NULL);
}

/*************************************************************************/
/** rc_set_entry():	Setzt den Wert eines Konfigurations-Eintrags.		**/
/*************************************************************************/
/** => rc				Zeiger auf die Konfigurations-Struktur.				**/
/** => name				Name des Eintrags dessen Wert gesetzt werden soll.	**/
/** => value			Wert der gesetzt werden soll.								**/
/**																							**/
/** <=					Wert des Eintrags oder NULL.								**/
/*************************************************************************/

unsigned char *rc_set_entry(struct vboxrc *rc, unsigned char *name, unsigned char *value)
{
	int				i = 0;
	unsigned char *v = NULL;

	log(LOG_D, "Setting \"%s\" to \"%s\"...\n", ((char *)name ? (char *)name : "???"), ((char *)value ? (char *)value : "???"));

	if ((!name) || (!value))
	{
		if (!name ) log_line(LOG_W, "Configuration variable name not set (ignored).\n");
		if (!value) log_line(LOG_W, "Configuration argument not set (ignored).\n");

		return(NULL);
	}
	
	while (rc[i].name)
	{
		if (strcasecmp(rc[i].name, name) == 0)
		{
			v = strdup(value);
			
			if (v)
			{
				if (rc[i].value) free(rc[i].value);
				
				rc[i].value = v;
			}
			else log_line(LOG_E, "Can't set \"%s\"'s value.\n", name);

			return(v);
		}

		i++;
	}

	log_line(LOG_W, "Unknown entry \"%s\" ignored.\n", name);

	return(NULL);
}

/*************************************************************************/
/** rc_set_empty():	Setzt den Wert eines Konfigurations-Eintrags wenn	**/
/**						dieser noch keinen Wert enthält.							**/
/*************************************************************************/
/** Siehe rc_set_entry().																**/
/*************************************************************************/

unsigned char *rc_set_empty(struct vboxrc *rc, unsigned char *name, unsigned char *value)
{
	int i = 0;

	while (rc[i].name)
	{
		if (strcasecmp(rc[i].name, name) == 0)
		{
			if (rc[i].value) return(rc[i].value);

			return(rc_set_entry(rc, name, value));
		}

		i++;
	}

	return(NULL);
}
