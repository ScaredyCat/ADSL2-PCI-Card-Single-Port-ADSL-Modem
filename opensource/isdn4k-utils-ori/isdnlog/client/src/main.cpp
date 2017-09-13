/* $Id: main.cpp,v 1.2 1998/05/10 23:40:12 luethje Exp $
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
 * $Log: main.cpp,v $
 * Revision 1.2  1998/05/10 23:40:12  luethje
 * some changes
 *
 */

#include <errno.h>

#define _KISDNLOG_MAIN_C_

#include "kisdnlog.h"

/****************************************************************************/

//const char*basename(const char *);

/****************************************************************************/

const char *options = "p:";

/****************************************************************************/

int main(int argc, char** argv)
{
	const char *port = NULL;
	const char *server = NULL;
	int c;

	KApplication app(argc, argv, KISDNLOG_NAME);

	Messager = new KMsgHdl();

//	Messager->HandleMessage(TRUE,123,0,(3,"wer","bist","du?"));
//	Messager->HandleMessage(TRUE,123,5,(3,"1",MsgHdl::ltoa(2),MsgHdl::dtoa(3.145)));

	while ((c = getopt(argc, argv, options)) != EOF)
		switch(c)
		{
			case 'p' : port = optarg;
			           break;
			case '?' : fprintf(stderr,"%s usage: %s %s [server]\n\n", basename(argv[0]), argv[0], options);
			           exit(1);
		}

	if (optind < argc)
	    server = argv[optind];

	if (server != NULL)
	{
		KConnection *conn;

		if ((conn = new KConnection(&app,server,port)) != NULL)
		{
			conn->Connect();
			conn->show();
		}
	}
	else
		KConnection::StartApp(&app);

	return app.exec();
}

/****************************************************************************/

