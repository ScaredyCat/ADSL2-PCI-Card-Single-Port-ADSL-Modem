/* $Id: tdu_user.c,v 1.1 1999/06/30 17:18:48 he Exp $ */
/*
  Copyright 1997 by Henner Eisen

    This code is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This code is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/ 
/*
  A (rather incomplete) implementation of the T-protocol
  defined in ETS 300 075. 

  This file containing stuff that an application using the
  the T-protocol needs to call.
*/ 

#include <stdio.h>
#include <unistd.h>
#include <tdu_user.h>

#include "tdu.h"

struct tdu_stream * tdu_user_get_stream( struct tdu_user * user )
{
	return user -> fsm -> stream;
}

struct tdu_user * tdu_stream_get_user( struct tdu_stream * st )
{
	return st -> fsm -> user;
}

void tdu_user_set_initiator(struct tdu_user * user, int initiator)
{
	user->fsm->idle.initiator = initiator;
}


int tdu_msg2stdout(struct tdu_user * ts, char * prompt, struct tdu_param* par)
{
	par->par.udata->udata[par->par.udata->udata_len] = 0;
	printf( "%s%s\n", prompt,
		par->par.udata->udata);
	return 0;
}

