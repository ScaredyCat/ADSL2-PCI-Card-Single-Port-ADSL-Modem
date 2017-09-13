/* $Id: ctrlconf.c,v 1.11 2001/05/23 14:48:23 kai Exp $
 *
 * ISDN accounting for isdn4linux. (Utilities)
 *
 * Copyright 1995, 1997 by Stefan Luethje (luethje@sl-gw.lake.de)
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
 * $Log: ctrlconf.c,v $
 * Revision 1.11  2001/05/23 14:48:23  kai
 * make isdnctrl independent of the version of installed kernel headers,
 * we have our own copy now.
 *
 * Revision 1.10  1999/11/02 20:41:21  keil
 * make phonenumber ioctl compatible for ctrlconf too
 *
 * Revision 1.9  1998/11/21 14:03:31  luethje
 * isdnctrl: added dialmode into the config file
 *
 * Revision 1.8  1998/03/20 07:52:13  calle
 * Allow readconf to read multilink configs with more than 2 channels.
 *
 * Revision 1.7  1997/10/27 00:05:03  fritz
 * Fixed typo.
 *
 * Revision 1.6  1997/10/26 23:39:37  fritz
 * Applied (slightly modified) fix for compiling without triggercps by
 * Bernhard Rosenkraenzer <root@BOL-SubNet.ml.org>
 *
 * Revision 1.5  1997/08/21 14:46:58  fritz
 * Added Version-Checking of NET_DV.
 *
 * Revision 1.4  1997/07/23 20:39:14  luethje
 * added the option "force" for the commands delif and reset
 *
 * Revision 1.3  1997/07/22 22:36:08  luethje
 * isdnrep:  Use "&nbsp;" for blanks
 * isdnctrl: Add the option "reset"
 *
 * Revision 1.2  1997/06/26 21:25:14  luethje
 * Added the trigger function to the config file.
 *
 * Revision 1.1  1997/06/24 23:35:25  luethje
 * isdnctrl can use a config file
 *
 *
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "isdnctrl.h"
#include "ctrlconf.h"

/*****************************************************************************/

#define PHONE(phone) ((isdn_net_ioctl_phone*) phone)

/*****************************************************************************/

static int readinterfaces(int fd, section* CSec, section *PSec);
static char* readoptions(int fd, char *name, int is_master, section *CSec, section *PSec);
static int del_all_numbers(int fd, char *name, int direction);
static char* write_all_numbers(char *numbers);
static int set_all_numbers(int fd, char *name, int direction, char *numbers);
static int create_interface(int fd, char *name);
static int interface_exist(int fd, char *name);

/*****************************************************************************/

int writeconfig(int fd, char *file)
{
	section *Section;
	section *ConfigSection;
/*
	section *PhoneSection;
*/

	ConfigSection = read_file(NULL,file,C_NOT_UNIQUE|C_NO_WARN_FILE);
/*
	read_conffiles(&PhoneSection,NULL);
*/

	if ((Section = Set_Section(&ConfigSection,CONF_SEC_ISDNCTRL,C_OVERWRITE | C_WARN)) == NULL)
		return -1;

	readinterfaces(fd,Section,NULL /*PhoneSection*/);

	if (write_file(ConfigSection,file,cmd,VERSION) == NULL)
		return -1;

	return 0;
}

/*****************************************************************************/

static int readinterfaces(int fd, section* CSec, section *PSec)
{
	char name[10];
	FILE *iflst;
	char s[BUFSIZ];
	char *p;

	if ((iflst = fopen(FILE_PROC, "r")) == NULL)
	{
		perror(FILE_PROC);
		return -1;
	}

	while (!feof(iflst))
	{
		fgets(s, sizeof(s), iflst);

		if ((p = strchr(s, ':')))
		{
			*p = 0;
			sscanf(s, "%s", name);
			p = readoptions(fd, name, 1, CSec, PSec);

			while (p != NULL && *p != '\0')
			{
				strcpy(name, p);
				p = readoptions(fd, name, 0, CSec, PSec);
			}
		}
	}

	fclose(iflst);

	return 0;
}

/*****************************************************************************/

static char* readoptions(int fd, char *name, int is_master, section *CSec, section *PSec)
{
	static isdn_net_ioctl_cfg cfg;
	section *SubSec = NULL;
	char inphone[BUFSIZ];
	char outphone[BUFSIZ];
	char string[256];
	char *interface = is_master?CONF_SEC_INTERFACE:CONF_SEC_SLAVE;
	char *RetCode = NULL;


	if (name == NULL && *name != '\0')
		return NULL;

	strcpy(cfg.name, name);

	if (ioctl(fd, IIOCNETGCF, &cfg) < 0)
		return NULL;

	set_isdn_net_ioctl_phone(PHONE(inphone), name, "", 0);
	if (ioctl(fd, IIOCNETGNM, PHONE(inphone)) < 0)
		return NULL;

	set_isdn_net_ioctl_phone(PHONE(outphone), name, "", 1);
	if (ioctl(fd, IIOCNETGNM, PHONE(outphone)) < 0)
		return NULL;

	if (Set_Section(&SubSec,interface,C_OVERWRITE | C_WARN) == NULL)
		return NULL;

	if (Set_Entry(SubSec,interface,CONF_ENT_NAME,name, C_OVERWRITE | C_WARN) == NULL)
		return NULL;

	if (*cfg.eaz != '\0')
		if (Set_Entry(SubSec,interface,CONF_ENT_EAZ,cfg.eaz, C_OVERWRITE | C_WARN) == NULL)
			return NULL;

	if (*inphone != '\0')
		if (Set_Entry(SubSec,interface,CONF_ENT_PHONE_IN,write_all_numbers(inphone), C_OVERWRITE | C_WARN) == NULL)
			return NULL;

	if (*outphone != '\0')
		if (Set_Entry(SubSec,interface,CONF_ENT_PHONE_OUT,write_all_numbers(outphone), C_OVERWRITE | C_WARN) == NULL)
			return NULL;

	if (Set_Entry(SubSec,interface,CONF_ENT_SECURE, cfg.secure?"on":"off", C_OVERWRITE | C_WARN) == NULL)
		return NULL;

	if (Set_Entry(SubSec,interface,CONF_ENT_DIALMODE, cfg.dialmode == ISDN_NET_DM_MANUAL?"manual":cfg.dialmode == ISDN_NET_DM_AUTO?"auto":"off", C_OVERWRITE | C_WARN) == NULL)
		return NULL;

	if (cfg.callback)
	{
		if (Set_Entry(SubSec,interface,CONF_ENT_CALLBACK,num2callb[cfg.callback], C_OVERWRITE | C_WARN) == NULL)
			return NULL;

		if (Set_Entry(SubSec,interface,CONF_ENT_CBHUP, cfg.cbhup?"on":"off", C_OVERWRITE | C_WARN) == NULL)
			return NULL;

		sprintf(string,"%d",cfg.cbdelay / 5);
		if (Set_Entry(SubSec,interface,CONF_ENT_CBDELAY, string, C_OVERWRITE | C_WARN) == NULL)
			return NULL;
	}

	if (cfg.dialmax)
	{
		sprintf(string,"%d",cfg.dialmax);
		if (Set_Entry(SubSec,interface,CONF_ENT_DIALMAX, string, C_OVERWRITE | C_WARN) == NULL)
			return NULL;
	}

	if (cfg.onhtime)
	{
		sprintf(string,"%d",cfg.onhtime);
		if (Set_Entry(SubSec,interface,CONF_ENT_HUPTIMEOUT, string, C_OVERWRITE | C_WARN) == NULL)
			return NULL;
	}

	if (Set_Entry(SubSec,interface,CONF_ENT_IHUP, cfg.ihup?"on":"off", C_OVERWRITE | C_WARN) == NULL)
		return NULL;

	if (Set_Entry(SubSec,interface,CONF_ENT_CHARGEHUP, cfg.chargehup?"on":"off", C_OVERWRITE | C_WARN) == NULL)
		return NULL;

	if (cfg.chargeint)
	{
		sprintf(string,"%d",(data_version < 2)?0:cfg.chargeint);
		if (Set_Entry(SubSec,interface,CONF_ENT_CHARGEINT, string, C_OVERWRITE | C_WARN) == NULL)
			return NULL;
	}

	if (Set_Entry(SubSec,interface,CONF_ENT_L2_PROT, num2key(cfg.l2_proto, l2protostr, l2protoval), C_OVERWRITE | C_WARN) == NULL)
		return NULL;

	if (Set_Entry(SubSec,interface,CONF_ENT_L3_PROT, num2key(cfg.l3_proto, l3protostr, l3protoval), C_OVERWRITE | C_WARN) == NULL)
		return NULL;

	if (Set_Entry(SubSec,interface,CONF_ENT_ENCAP, num2key(cfg.p_encap, pencapstr, pencapval), C_OVERWRITE | C_WARN) == NULL)
		return NULL;

	if (*cfg.slave != '\0')
	{
		if (Set_Entry(SubSec,interface,CONF_ENT_ADDSLAVE, cfg.slave, C_OVERWRITE | C_WARN) == NULL)
			return NULL;

		sprintf(string,"%d",cfg.slavedelay);
		if (Set_Entry(SubSec,interface,CONF_ENT_SDELAY, string, C_OVERWRITE | C_WARN) == NULL)
			return NULL;
#ifdef HAVE_TRIGGERCPS		
		sprintf(string,"%d",(data_version < 3)?6000:cfg.triggercps);
#else
		sprintf(string,"6000");
#endif
		if (Set_Entry(SubSec,interface,CONF_ENT_TRIGGERCPS, string, C_OVERWRITE | C_WARN) == NULL)
			return NULL;
		
		RetCode = cfg.slave;
	}

	if (*cfg.drvid != '\0')
	{
		sprintf(string,"%s %s",cfg.drvid,cfg.exclusive>0?"exclusive":"");
		if (Set_Entry(SubSec,interface,CONF_ENT_BIND, string, C_OVERWRITE | C_WARN) == NULL)
			return NULL;
	}	
	
	if (cfg.pppbind >= 0)
	{
		sprintf(string,"%d",cfg.pppbind);
		if (Set_Entry(SubSec,interface,CONF_ENT_PPPBIND, string, C_OVERWRITE | C_WARN) == NULL)
			return NULL;
	}	
	
	if (Set_SubSection(CSec,CONF_ENT_INTERFACES,SubSec,C_APPEND | C_WARN) == NULL)
		return NULL;

	return RetCode;
}

/*****************************************************************************/

static char* write_all_numbers(char *numbers)
{
	return numbers;
}

/*****************************************************************************/

static char *get_masterinterface(int fd, char *name)
{
	isdn_net_ioctl_cfg cfg;
	static char mname[sizeof(cfg.master)];

	strncpy(cfg.name, name, sizeof(cfg.name));

	if (ioctl(fd, IIOCNETGCF, &cfg) < 0)
		return name;

	if (cfg.master[0] == 0) return name;
	return strncpy(mname, cfg.master, sizeof(mname));
}

/*****************************************************************************/

int readconfig(int fd, char *file)
{
	section *Section;
	section *ConfigSection;
/*
	section *PhoneSection;
*/
	entry   *Entry;
	char    *argv[5];
	char    *name;
	int      cnt = 1;

	if ((ConfigSection = read_file(NULL,file,C_NOT_UNIQUE|C_NO_WARN_FILE)) == NULL)
		return -1;

/*
	read_conffiles(&PhoneSection,NULL);
*/

	if ((Section = Get_Section(ConfigSection,CONF_SEC_ISDNCTRL)) == NULL)
	{
		fprintf(stderr,"File `%s' has no section `%s'!\n",file,CONF_SEC_ISDNCTRL);
		return -1;
	}
	if ((Entry = Get_Entry(Section->entries,CONF_ENT_INTERFACES)) == NULL)
	{
		fprintf(stderr,"Section `%s' has no entry `%s'!\n",CONF_SEC_ISDNCTRL,CONF_ENT_INTERFACES);
		return -1;
	}

	if ((Section = Entry->subsection) == NULL)
	{
		fprintf(stderr,"Entry `%s' has no subsections!\n",CONF_ENT_INTERFACES);
		return -1;
	}

	while (Section != NULL)
	{
		if ((Entry = Get_Entry(Section->entries,CONF_ENT_NAME)) == NULL)
		{
			fprintf(stderr,"Missing the interface name of the %d. interface sections!\n",cnt);
			return -1;
		}
		else 
			name = Entry->value;

		if (!strcmp(Section->name, CONF_SEC_INTERFACE))
			create_interface(fd,name);

		Entry = Section->entries;

		while (Entry != NULL)
		{
			if (!strcmp(Entry->name,CONF_ENT_NAME))
			{
			}
			else
			if (!strcmp(Entry->name,CONF_ENT_EAZ))
			{
				argv[0] = cmds[EAZ].cmd;
				argv[1] = name;
				argv[2] = Entry->value;
				argv[3] = NULL;
				exec_args(fd,3,argv);
			}
			else
			if (!strcmp(Entry->name,CONF_ENT_PHONE_IN))
			{
				del_all_numbers(fd,name,0);
				set_all_numbers(fd,name,0,Entry->value);
			}
			else
			if (!strcmp(Entry->name,CONF_ENT_PHONE_OUT))
			{
				del_all_numbers(fd,name,1);
				set_all_numbers(fd,name,1,Entry->value);
			}
			else
			if (!strcmp(Entry->name,CONF_ENT_SECURE))
			{
				argv[0] = cmds[SECURE].cmd;
				argv[1] = name;
				argv[2] = Entry->value;
				argv[3] = NULL;
				exec_args(fd,3,argv);
			}
			else
			if (!strcmp(Entry->name,CONF_ENT_DIALMODE))
			{
				argv[0] = cmds[DIALMODE].cmd;
				argv[1] = name;
				argv[2] = Entry->value;
				argv[3] = NULL;
				exec_args(fd,3,argv);
			}
			else
			if (!strcmp(Entry->name,CONF_ENT_CALLBACK))
			{
				argv[0] = cmds[CALLBACK].cmd;
				argv[1] = name;
				argv[2] = Entry->value;
				argv[3] = NULL;
				exec_args(fd,3,argv);
			}
			else
			if (!strcmp(Entry->name,CONF_ENT_CBDELAY))
			{
				argv[0] = cmds[CBDELAY].cmd;
				argv[1] = name;
				argv[2] = Entry->value;
				argv[3] = NULL;
				exec_args(fd,3,argv);
			}
			else
			if (!strcmp(Entry->name,CONF_ENT_CBHUP))
			{
				argv[0] = cmds[CBHUP].cmd;
				argv[1] = name;
				argv[2] = Entry->value;
				argv[3] = NULL;
				exec_args(fd,3,argv);
			}
			else
			if (!strcmp(Entry->name,CONF_ENT_DIALMAX))
			{
				argv[0] = cmds[DIALMAX].cmd;
				argv[1] = name;
				argv[2] = Entry->value;
				argv[3] = NULL;
				exec_args(fd,3,argv);
			}
			else
			if (!strcmp(Entry->name,CONF_ENT_HUPTIMEOUT))
			{
				argv[0] = cmds[HUPTIMEOUT].cmd;
				argv[1] = name;
				argv[2] = Entry->value;
				argv[3] = NULL;
				exec_args(fd,3,argv);
			}
			else
			if (!strcmp(Entry->name,CONF_ENT_IHUP))
			{
				argv[0] = cmds[IHUP].cmd;
				argv[1] = name;
				argv[2] = Entry->value;
				argv[3] = NULL;
				exec_args(fd,3,argv);
			}
			else
			if (!strcmp(Entry->name,CONF_ENT_CHARGEHUP))
			{
				argv[0] = cmds[CHARGEHUP].cmd;
				argv[1] = name;
				argv[2] = Entry->value;
				argv[3] = NULL;
				exec_args(fd,3,argv);
			}
			else
			if (!strcmp(Entry->name,CONF_ENT_CHARGEINT))
			{
				argv[0] = cmds[CHARGEINT].cmd;
				argv[1] = name;
				argv[2] = Entry->value;
				argv[3] = NULL;
				exec_args(fd,3,argv);
			}
			else
			if (!strcmp(Entry->name,CONF_ENT_L2_PROT))
			{
				argv[0] = cmds[L2_PROT].cmd;
				argv[1] = name;
				argv[2] = Entry->value;
				argv[3] = NULL;
				exec_args(fd,3,argv);
			}
			else
			if (!strcmp(Entry->name,CONF_ENT_L3_PROT))
			{
				argv[0] = cmds[L3_PROT].cmd;
				argv[1] = name;
				argv[2] = Entry->value;
				argv[3] = NULL;
				exec_args(fd,3,argv);
			}
			else
			if (!strcmp(Entry->name,CONF_ENT_ENCAP))
			{
				argv[0] = cmds[ENCAP].cmd;
				argv[1] = name;
				argv[2] = Entry->value;
				argv[3] = NULL;
				exec_args(fd,3,argv);
			}
			else
			if (!strcmp(Entry->name,CONF_ENT_BIND))
			{
				char  string[256];
				char *ptr;

				strcpy(string, Entry->value);
				
				if ((ptr = strchr(string,' ')) != NULL)
					while (isspace(*ptr)) *ptr++ = '\0';

				argv[0] = cmds[BIND].cmd;
				argv[1] = name;
				argv[2] = string;
				argv[3] = ptr;
				argv[4] = NULL;
				exec_args(fd,ptr?4:3,argv);
			}
			else
			if (!strcmp(Entry->name,CONF_ENT_PPPBIND))
			{
				argv[0] = cmds[PPPBIND].cmd;
				argv[1] = name;
				argv[2] = Entry->value;
				argv[3] = NULL;
				exec_args(fd,3,argv);
			}
			else
			if (!strcmp(Entry->name,CONF_ENT_ADDSLAVE))
			{
				argv[0] = cmds[ADDSLAVE].cmd;
				argv[1] = get_masterinterface(fd, name);
				argv[2] = Entry->value;
				argv[3] = NULL;
				exec_args(fd,3,argv);
			}
			else
			if (!strcmp(Entry->name,CONF_ENT_SDELAY))
			{
				argv[0] = cmds[SDELAY].cmd;
				argv[1] = name;
				argv[2] = Entry->value;
				argv[3] = NULL;
				exec_args(fd,3,argv);
			}
			else
			if (!strcmp(Entry->name,CONF_ENT_TRIGGERCPS))
			{
				argv[0] = cmds[TRIGGER].cmd;
				argv[1] = name;
				argv[2] = Entry->value;
				argv[3] = NULL;
				exec_args(fd,3,argv);
			}
			else
			{
				fprintf(stderr,"Unknown entry `%s' in interface section `%s'!\n",Entry->name,name);
				return -1;
			}

			Entry = Entry->next;
		}

		Section = Section->next;
		cnt++;
	}

	return 0;
}

/*****************************************************************************/

static int interface_exist(int fd, char *name)
{
	char iname[10];
	FILE *iflst;
	char s[BUFSIZ];
	char *p;


	if ((iflst = fopen(FILE_PROC, "r")) == NULL)
	{
		perror(FILE_PROC);
		return -1;
	}

	while (!feof(iflst))
	{
		fgets(s, sizeof(s), iflst);

		if ((p = strchr(s, ':')))
		{
			*p = 0;
			sscanf(s, "%s", iname);

			if (!strcmp(name,iname))
			{
				fclose(iflst);
				return 1;
			}
		}
	}

	fclose(iflst);

	return 0;
}

/*****************************************************************************/

static int create_interface(int fd, char *name)
{
	char *argv[3];
	

	if (interface_exist(fd,name))
		return -1;

	argv[0] = cmds[ADDIF].cmd;
	argv[1] = name;
	argv[2] = NULL;
	exec_args(fd,2,argv);

	return 0;
}

/*****************************************************************************/

static int set_all_numbers(int fd, char *name, int direction, char *numbers)
{
	isdn_net_ioctl_phone phone;
	char phonestr[BUFSIZ];
	char *ptr = phonestr;
	char *ptr2;


	strcpy(phonestr, numbers);

	if (*phonestr != '\0')
	{
		do
		{
			if ((ptr = strrchr(phonestr,' ')) == NULL)
				ptr = phonestr;
			else
			{
				ptr2 = ptr++;
				while (isspace(*ptr2) && ptr2 != phonestr) *ptr2-- = '\0';
			}

			set_isdn_net_ioctl_phone(&phone, name, ptr, direction);
			if (*ptr != '\0')
			{
				if (ioctl(fd, IIOCNETANM, &phone) < 0)
				{
					perror(name);
					return -1;
				}
			}
		}
		while (ptr != phonestr);
	}

	return 0;
}

/*****************************************************************************/

static int del_all_numbers(int fd, char *name, int direction)
{
	isdn_net_ioctl_phone phone;
	char phonestr[BUFSIZ];
	char *ptr = phonestr;
	char *ptr2;


	if (name == NULL)
		return -1;

	set_isdn_net_ioctl_phone(PHONE(phonestr), name, "", direction);
	if (ioctl(fd, IIOCNETGNM, PHONE(phonestr)) < 0)
	{
		perror(name);
		return -1;
	}

	if (*phonestr != '\0')
	{
		do
		{
			if ((ptr = strrchr(phonestr,' ')) == NULL)
				ptr = phonestr;
			else
			{
				ptr2 = ptr++;
				while (isspace(*ptr2) && ptr2 != phonestr) *ptr2-- = '\0';
			}

			if (*ptr != '\0')
			{
				set_isdn_net_ioctl_phone(&phone, name, ptr, direction);
				if (ioctl(fd, IIOCNETDNM, &phone) < 0)
				{
					perror(name);
					return -1;
				}
			}
		}
		while (ptr != phonestr);
	}

	return 0;
}

/*****************************************************************************/

