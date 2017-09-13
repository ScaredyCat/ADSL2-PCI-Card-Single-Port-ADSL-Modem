/*****************************************************************************/
/*                                                                           */
/*                                 ACTEST.C                                  */
/*                                                                           */
/*                       Test an areacode data file                          */
/*                                                                           */
/*                                                                           */
/*                                                                           */
/* (C) 1996,97  Ullrich von Bassewitz                                        */
/*              Wacholderweg 14                                              */
/*              D-70597 Stuttgart                                            */
/* EMail:       uz@musoftware.com                                            */
/*                                                                           */
/*                                                                           */
/* This software is provided 'as-is', without any express or implied         */
/* warranty.  In no event will the authors be held liable for any damages    */
/* arising from the use of this software.                                    */
/*                                                                           */
/* Permission is granted to anyone to use this software for any purpose,     */
/* including commercial applications, and to alter it and redistribute it    */
/* freely, subject to the following restrictions:                            */
/*                                                                           */
/* 1. The origin of this software must not be misrepresented; you must not   */
/*    claim that you wrote the original software. If you use this software   */
/*    in a product, an acknowledgment in the product documentation would be  */
/*    appreciated but is not required.                                       */
/* 2. Altered source versions must be plainly marked as such, and must not   */
/*    be misrepresented as being the original software.                      */
/* 3. This notice may not be removed or altered from any source              */
/*    distribution.                                                          */
/*                                                                           */
/*****************************************************************************/



#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "areacode.h"



int main (int argc, char* argv [])
{
    acInfo AC;
    char Buf [256];
    int Len;
    unsigned RC;
    acMaxMem = 0;

    /* See if there is an argument given. If so, use it as name and path of
     * the database.
     */
    if (argc >= 2) {
        acFileName = argv [1];
    }

    /* If we have an additional argument, use it as the phone number to look
     * up.
     */
    if (argc >= 3) {
	RC = GetAreaCodeInfo (&AC, argv [2]);
	if (RC == acOk) {
	    printf ("%u %s\n", AC.AreaCodeLen, AC.Info);
	}
	return RC;
    }

    /* Test loop */
    while (1) {

        printf ("Enter phone number: ");
        fflush (stdout);
        if (fgets (Buf, sizeof (Buf), stdin) == 0) {
	    /* EOF on stdin */
	    break;
	}
	Len = strlen (Buf);
	while (Len > 0 && Buf [Len-1] == '\n') {
	    Buf [--Len] = '\0';
	}
        if (Len == 0) {
            break;
        }

        switch ((RC = GetAreaCodeInfo (&AC, Buf))) {

            case acOk:
                printf ("acOK:\n"
                        "  Length    = %d\n"
                        "  Info      = %s\n",
                        AC.AreaCodeLen, AC.Info);
                break;

            case acFileError:
                printf ("acFileError\n");
                break;

            case acInvalidFile:
                printf ("acInvalidFile\n");
                break;

            case acWrongVersion:
                printf ("acWrongVersion\n");
                break;

	    case acInvalidInput:
	    	printf ("acInvalidInput\n");
		break;

            default:
                printf ("Unknown return: %u\n", RC);
                break;

        }
        printf ("\n");

    }

    return 0;
}

