
/*
 *
 * Copyright (C) Eicon Technology Corporation, 2000.
 *
 * This source file is supplied for the exclusive use with Eicon
 * Technology Corporation's range of DIVA Server Adapters.
 *
 * Eicon File Revision :    1.2  
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY OF ANY KIND WHATSOEVER INCLUDING ANY 
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */


#include <stdio.h>
#include <string.h>

#include "idi.h"
#include "pc.h"

typedef struct
{
    int     value;
    char	*name;
} map_t;

static
map_t   global_id_name [] =
{
    { 0,			"?????" },
	{ GL_ERR_ID,	"ERR  " },
	{ DSIG_ID,		"D-SIG" },
	{ NL_ID,		"NL   " },
	{ BLLC_ID,		"B-LLC" },
	{ TASK_ID,		"TASK " },
	{ TIMER_ID,		"TIMER" },
	{ TEL_ID,		"TEL  " },
	{ MAN_ID,		"MGMT " },
    { 0,			NULL }
};

static
map_t   rc_name [] =
{
    { 0,				"?????" },
	{ UNKNOWN_COMMAND,	"unknown command   " },
	{ WRONG_COMMAND,	"wrong command     " },
	{ WRONG_ID,			"unknown id        " },
	{ WRONG_CH,			"wrong channel     " },
	{ UNKNOWN_IE,		"unknown IE        " },
	{ WRONG_IE,			"wrong IE          " },
	{ OUT_OF_RESOURCES,	"out of resources  " },
	{ ADAPTER_DEAD,		"adapter dead      " },
	{ N_FLOW_CONTROL,	"n-flow control    " },
	{ ASSIGN_RC,		"assign ack        " },
	{ ASSIGN_OK,		"assign O.K.       " },
	{ OK_FC,			"flow control O.K. " },
	{ READY_INT,		"ready interrupt   " },
	{ TIMER_INT,		"timer interrupt   " },
	{ OK,				"O.K.              " },

    { 0,		NULL }
};

/*
 * Map integer value to associated string
 */

static
char    *map_int(int val, map_t *table)

{
    static map_t   *t;

    t = table;
    t++;
    while (t->name)
    {
        if (t->value == val)
        {
            break;
        }
        t++;
    }

    if (!t->name)
    {
        t = table;
    }

    return(t->name);
}


/*
 * display ID name or value
 */

static
void		display_id(FILE *fp, int id)

{
	char	*s;

	s = map_int(id, global_id_name);
	if (*s != '?')
	{
		fprintf(fp, "%s ", s);
	}
	else
	{
		fprintf(fp, "   %02x ", id);
	}

	return;
}

void	log_idi_req(FILE *fp, void * buf)

{
	ENTITY	*e = buf;

	fprintf(fp, "req %04x.%04x ", e->user[0] & 0xffff, e->user[1] & 0xffff);

	display_id(fp, (int) e->Id & 0xff);

	fprintf(fp, "REQ: %02x\n", e->Req & 0xff);

	return;
}

void	log_idi_cb(FILE *fp, void * buf)

{
	ENTITY	*e = buf;

	fprintf(fp, "%s %04x.%04x ", e->Rc ? " rc" : "ind",
			e->user[0] & 0xffff, e->user[1] & 0xffff);

	display_id(fp, (int) e->Id & 0xff);

	if ((e->Rc) && (e->Ind))
	{
		fprintf(fp, "ERRROR - both Rc (%02x) and Ind (%02x) set !\n",
					e->Rc & 0xff, e->Ind & 0xff);
	}
	else
	{
		if (e->Rc)
		{
			fprintf(fp, "%s\n", map_int((int) e->Rc & 0xff, rc_name));
		}
		if (e->Ind)
		{
			fprintf(fp, "IND: %02x\n", e->Ind & 0xff);
		}
	}

	return;
}
