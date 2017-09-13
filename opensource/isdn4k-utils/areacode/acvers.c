/*****************************************************************************/
/*                                                                           */
/*                                 ACVERS.C                                  */
/*                                                                           */
/*            Get the version and build of an areacode data file             */
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



/* This code assumes 8 bit bytes */



#include <stdio.h>
#include <string.h>
#include <stdlib.h>



/*****************************************************************************/
/*                                   Data                                    */
/*****************************************************************************/



#define MAGIC0  0x68
#define MAGIC1  0x57
#define MAGIC2  0x46
#define MAGIC3  0x35



/*****************************************************************************/
/*                                   Code                                    */
/*****************************************************************************/



static void Usage (const char* ProgName)
/* Print usage information and exit */
{
    printf ("%s: Print the version of an areacode database\n\n"
            "Usage: \t%s file ...\n",
            ProgName, ProgName);
    exit (1);
}



int main (int argc, char* argv [])
{
    int I;

    if (argc < 2) {
        Usage (argv [0]);
    }

    for (I = 1; I < argc; I++) {

        FILE* F;
        const char* Filename = argv [I];
        unsigned char MagicBuf [4];
        unsigned char BuildBuf [2];
        unsigned char VersionBuf [2];
        unsigned Build;

        if ((F = fopen (Filename, "rb")) == 0) {
            perror ("Cannot open input file");
            exit (2);
        }

        fread (MagicBuf, sizeof (MagicBuf), 1, F);
        if (MagicBuf [0] != MAGIC0 || MagicBuf [1] != MAGIC1 ||
            MagicBuf [2] != MAGIC2 || MagicBuf [3] != MAGIC3) {
            fprintf (stderr, "No dial prefix database\n");
            exit (3);
        }

        fread (BuildBuf, sizeof (BuildBuf), 1, F);
        fread (VersionBuf, sizeof (VersionBuf), 1, F);
        fclose (F);

        Build   = ((unsigned) BuildBuf [1]) * 256 + ((unsigned) BuildBuf [0]);
        printf ("%d %d %u\n", VersionBuf [1], VersionBuf [0], Build);

    }
    return 0;
}



