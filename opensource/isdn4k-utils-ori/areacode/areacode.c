/*****************************************************************************/
/*                                                                           */
/*                                AREACODE.C                                 */
/*                                                                           */
/*     Portable library module to search for an area code in a database.     */
/*                                                                           */
/*                                                                           */
/*                                                                           */
/* (C) 1996-98  Ullrich von Bassewitz                                        */
/*              Wacholderweg 14                                              */
/*              D-70597 Stuttgart                                            */
/* EMail:       uz@musoftware.de                                             */
/*                                                                           */
/*                                                                           */
/* This software is provided 'as-is', without any expressed or implied       */
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



/*
 * The code assumes
 *      - 8 bit bytes
 *      - unsigned long is 32 bit. This may be changed by #defining u32 to
 *        a data type that is an 32 bit unsigned when compiling this module.
 *      - ascii character set
 *
 * The code does *not* assume
 *      - a specific byte order. Currently the code autoadjusts to big or
 *        little endian data. If you have something more weird than that,
 *        you have to add conversion code.
 *
 */



#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <ctype.h>

#include "areacode.h"



/*****************************************************************************/
/*                          Externally visible data                          */
/*****************************************************************************/



/* The name of the areacode data file. The default is what is #defined as
 * DATA_FILENAME. If this is not #defined, the default is "areacode.dat",
 * which is probably not what you want. In the latter case set this to
 * the correct filename *before* your first call to GetAreaCodeInfo.
 */
#ifdef DATA_FILENAME
char* acFileName = DATA_FILENAME;
#else
char* acFileName = "areacode.dat";
#endif

/* How much dynamic memory is GetAreaCodeInfo allowed to consume? Having less
 * memory means more disk access and vice versa. The function does even work
 * if you set this value to zero. For maximum performance, the function needs
 * 4 byte per area code stored in the data file. The default is 32KB.
 */
unsigned long   acMaxMem        = 0x8000UL;



/*****************************************************************************/
/*                            Data and structures                            */
/*****************************************************************************/



/* Define an unsigned quantity with 32 bits. Try to make some clever
 * assumptions using the data from limits.h. This may break some older
 * (non ISO compliant) compilers, but I can't help...
 */
#if !defined(u32) && defined(ULONG_MAX)
#  if ULONG_MAX == 4294967295UL
#    define u32             unsigned long
#  endif
#endif
#if !defined(u32) && defined(UINT_MAX)
#  if UINT_MAX == 4294967295UL
#    define u32             unsigned
#  endif
#endif
#if !defined(u32) && defined(USHRT_MAX)
#  if USHRT_MAX == 4294967295UL
#    define u32             unsigned short
#  endif
#endif
#if !defined(u32)
#  define u32               unsigned long
#endif

/* The version of the data file we support (major only, minor is ignored) */
#define acVersion       0x200

/* The magic words in little and big endian format */
#define LittleMagic     0x35465768L
#define BigMagic        0x68574635L

/* Defining the byte ordering */
#define boLittleEndian  0
#define boBigEndian     1

/* The byte order used in the file is little endian (intel) format */
#define FileByteOrder   boLittleEndian

/* Shortcuts */
#define scCount		30

/* This is the header data of the data file. It is not used anywhere in
 * the code, just have a look at it since it describes the layout in the
 * file.
 */
typedef struct {
    u32         Magic;
    u32         Version;            /* Version in hi word, build in lo word */
    u32         Count;
    u32         AreaCodeStart;
    u32         InfoIndexStart;
    u32         InfoStart;
    u32		MinLength;
    u32	       	CodeDataSize;
    u32		Shortcut [scCount];    	/* Shortcuts for compression */
} PrefixHeader;

/* This is what's really used: */
typedef struct {

    /* The file we read from */
    FILE*       F;

    /* Machine byte order */
    unsigned    ByteOrder;

    /* Stuff from the file header */
    unsigned    Version;
    unsigned    Build;
    u32         Count;
    u32         AreaCodeStart;
    u32         InfoIndexStart;
    u32         InfoStart;
    u32		AreaCodeLenStart;
    u32		MinLength;		/* Minimum phone number length in data file */
    u32		CodeDataSize;
    u32 	Shortcut [scCount];	/* Compression shortcuts */

    /* Control data */
    long        First;
    long        Last;
    u32*        Table;

} AreaCodeDesc;

/* Translation table for translation ISO-8859-1 --> CP850. To save some space,
 * the table covers only values > 127
 */
#ifdef CHARSET_CP850
static char CP850Map [128] = {
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0xAD, 0x9B, 0x9C, 0x20, 0x9D, 0x20, 0x20,
    0x20, 0x20, 0xA6, 0xAE, 0xAA, 0x20, 0x20, 0x20,
    0xF8, 0xF1, 0xFD, 0x20, 0x20, 0xE6, 0x20, 0xF9,
    0x20, 0x20, 0xA7, 0xAF, 0xAC, 0x20, 0x20, 0xA8,
    0x20, 0x20, 0x20, 0x20, 0x8E, 0x8F, 0x92, 0x80,
    0x20, 0x90, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0xA5, 0x20, 0x20, 0x20, 0x20, 0x99, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x9A, 0x20, 0x20, 0xE1,
    0x85, 0xA0, 0x83, 0x20, 0x84, 0x86, 0x91, 0x87,
    0x8A, 0x82, 0x88, 0x89, 0x8D, 0xA1, 0x8C, 0x8B,
    0x20, 0xA4, 0x95, 0xA2, 0x93, 0x20, 0x94, 0xF6,
    0x20, 0x97, 0xA3, 0x20, 0x81, 0x20, 0xB0, 0x98
};
#endif

/* Macro to convert from big endian to little endian format and vice versa.
 * Beware: The macro evaluates its parameter more than once!
 */
#define _ByteSwap(__V) ((((__V) & 0x000000FF) << 24) |  \
                        (((__V) & 0xFF000000) >> 24) |  \
                        (((__V) & 0x0000FF00) <<  8) |  \
                        (((__V) & 0x00FF0000) >>  8))



/*****************************************************************************/
/*                             Helper functions                              */
/*****************************************************************************/



static int IsShortcut (char C)
/* Return true if the given character is a shortcut */
{
    return (C >= 0x01 && C <= 0x1F);
}



static u32 _ByteSwapIfNeeded (u32 D, unsigned ByteOrder)
/* Put the bytes into the correct order according to ByteOrder */
{
    /* Swap bytes if needed and return the result */
    switch (ByteOrder) {
        case boLittleEndian:    return D;
        default:                return _ByteSwap (D);
    }
}



static u32 ByteSwapIfNeeded (u32 D, const AreaCodeDesc* Desc)
/* Put the bytes into the correct order according to ByteOrder in Desc */
{
    /* Swap bytes if needed and return the result */
    return _ByteSwapIfNeeded (D, Desc->ByteOrder);
}



static u32 _Load_u32 (FILE* F, unsigned ByteOrder)
/* Load an u32 from the current file position and swap it if needed */
{
    u32 D;

    /* Read the data from the file */
    fread (&D, sizeof (D), 1, F);

    /* Swap bytes if needed and return the result */
    return _ByteSwapIfNeeded (D, ByteOrder);
}



static u32 _Load_u24 (FILE* F, unsigned ByteOrder)
/* Load an u24 from the current file position and swap it if needed */
{
    u32 D = 0;

    /* Read the data from the file */
    fread (&D, 3, 1, F);

    /* Swap bytes if needed and return the result */
    return _ByteSwapIfNeeded (D, ByteOrder);
}



static u32 Load_u32 (const AreaCodeDesc* Desc)
/* Load an u32 from the current file position and swap it if needed */
{
    return _Load_u32 (Desc->F, Desc->ByteOrder);
}



static u32 Load_u24 (const AreaCodeDesc* Desc)
/* Load an u32 from the current file position and swap it if needed */
{
    return _Load_u24 (Desc->F, Desc->ByteOrder);
}



static unsigned LoadShortcut (const AreaCodeDesc* Desc, char* S,
			      unsigned Size, unsigned char C)
/* Read the string for shortcut C into S, return the count of bytes read */
{
    unsigned CharsRead = 0;

    /* Get the replacement string in u32 format */
    u32 Shortcut = Desc->Shortcut [C-1];

    /* Insert character by character, do recursively replace shortcuts. */
    while (Shortcut && CharsRead < Size-1) {
	char C = Shortcut & 0x00FF;
       	if (IsShortcut (C)) {
	    /* This is another shortcut, replace it recursively */
	    unsigned ReadCount = LoadShortcut (Desc, S, Size-CharsRead, C);
	    CharsRead += ReadCount;
	    S         += ReadCount;
	} else {
	    /* No shortcut, use the character itself */
       	    *S++ = C;
	    CharsRead++;
	}
	Shortcut >>= 8;
    }

    /* Return the count of characters read */
    return CharsRead;
}



static void LoadString (const AreaCodeDesc* Desc, char* S, unsigned Count)
/* Read a zero terminated string from the file */
{
    unsigned CharsRead = 0;

    /* String must hold at least one character plus terminator */
    assert (Count >= 2);

    /* Fill the string */
    while (CharsRead < Count-1) {

 	/* Read the next character */
       	int C = getc (Desc->F);

 	/* If it is EOF or NUL, we're done */
 	if (C == EOF || C == 0) {
 	    break;
 	}

	/* Check if we have a real character or a shortcut */
	if (IsShortcut (C)) {
	    /* Insert a shortcut string */
	    unsigned ReadCount = LoadShortcut (Desc, S, Count-CharsRead, (char)C);
	    CharsRead += ReadCount;
       	    S         += ReadCount;
	} else {
	    /* Normal character */
       	    *S++ = (char) C;
	    CharsRead++;
	}

    }

    /* Set the terminating zero */
    *S = '\0';
}



static unsigned LoadFileHeader (AreaCodeDesc* Desc)
/* Load the header of a data file. Return one of the acXXX codes. */
{
    int I;
    u32 Version;

    /* Load the magic word in the format used int the file (do not convert) */
    u32 Magic = _Load_u32 (Desc->F, FileByteOrder);

    /* Check what we got from the file, determine the byte order */
    switch (Magic) {

        case BigMagic:
            Desc->ByteOrder = boBigEndian;
            break;

        case LittleMagic:
            Desc->ByteOrder = boLittleEndian;
            break;

        default:
            /* OOPS - the file is probably not a valid data file */
            return acInvalidFile;

    }

    /* Now read the rest of the header data */
    Version               = Load_u32 (Desc);
    Desc->Version         = (Version >> 16);
    Desc->Build           = (Version & 0xFFFF);
    Desc->Count           = Load_u32 (Desc);
    Desc->AreaCodeStart   = Load_u32 (Desc);
    Desc->InfoIndexStart  = Load_u32 (Desc);
    Desc->InfoStart       = Load_u32 (Desc);
    Desc->MinLength       = Load_u32 (Desc);
    Desc->CodeDataSize    = Load_u32 (Desc);

    /* Read the shortcuts */
    for (I = 0; I < scCount; I++) {
        Desc->Shortcut [I] = Load_u32 (Desc);
    }

    /* Check for some error conditions */
    if (ferror (Desc->F)) {
        /* Some sort of file problem */
        return acFileError;
    } else if (feof (Desc->F) || Desc->Count == 0) {
        /* This should not happen on a valid file */
        return acInvalidFile;
    } else if ((Desc->Version & 0xFF00) != acVersion) {
        return acWrongVersion;
    } else {
        /* Data is sane */
        return acOk;
    }
}



static u32 EncodeNumber (const char* Phone)
/* Encode the number we got from the caller into the internally used BCD
 * format. If there are invalid chars in the number, return 0xFFFFFFFF.
 */
{
    unsigned I;
    unsigned Len;
    u32 P = 0;          /* Initialize to make gcc happy */

    /* Get the amount of characters to convert */
    Len = strlen (Phone);
    if (Len == 0) {
	/* Invalid */
	return 0xFFFFFFFF;
    } else if (Len > 8) {
        Len = 8;
    }

    /* Convert the characters */
    for (I = 0; I < Len; I++) {
	/* Get the next character and check if it's valid */
	char C = Phone [I];
	if (!isascii (C) || !isdigit (C)) {
	    /* Invalid digit */
	    return 0xFFFFFFFF;
	}
        P = (P << 4) | (C & 0x0F);
    }

    /* Fill the rest of the number with 0x0F */
    I = 8 - Len;
    while (I--) {
        P = (P << 4) | 0x0F;
    }

    /* Done - return the result */
    return P;
}



static u32 ReadPhone (const AreaCodeDesc* Desc, long Index)
/* Read the phone number that is located at the given index. If we have a
 * part of the table already loaded into memory, use the memory copy, else
 * read the phone number from disk.
 */
{
    if (Desc->Table && Index >= Desc->First && Index <= Desc->Last) {
        /* Use the already loaded table, but don't forget to swap bytes */
        return ByteSwapIfNeeded (Desc->Table [Index - Desc->First], Desc);
    } else {
        /* Load the value from the file */
        fseek (Desc->F, Desc->AreaCodeStart + Index * sizeof (u32), SEEK_SET);
        return Load_u32 (Desc);
    }
}



static void LoadTable (AreaCodeDesc* Desc)
/* Load a part of the table into memory */
{
    u32 SpaceNeeded = (Desc->Last - Desc->First + 1) * sizeof (u32);
    Desc->Table = malloc (SpaceNeeded);
    if (Desc->Table == 0) {
        /* Out of memory. There is no problem with this now since we do
         * not really need the table in core memory (it speeds things up,
         * that's all). In addition to that, the memory requirement halves
         * with each iteration, so maybe we have more luck next time.
         */
        return;
    }

    /* Seek to the correct position in the file */
    fseek (Desc->F, Desc->AreaCodeStart + Desc->First * sizeof (u32), SEEK_SET);

    /* Read the data */
    fread (Desc->Table, SpaceNeeded, 1, Desc->F);
}



static unsigned char CalcCodeLen (u32 Code)
/* Calculate the length of a given (encoded) area code in characters */
{
    u32 Mask;
    unsigned char Len = 0;
    for (Mask = 0xF0000000L; Mask; Mask >>= 4) {
        if ((Code & Mask) != Mask) {
            Len++;
        } else {
            break;
        }
    }

    return Len;
}



static unsigned CalcMatchingDigits (u32 Code1, u32 Code2)
/* Return the count of digits that match when comparing both numbers from
 * left to right.
 */
{
    static const u32 Masks [9] = {
	0x00000000, 0xF0000000, 0xFF000000, 0xFFF00000,
	0xFFFF0000, 0xFFFFF000, 0xFFFFFF00, 0xFFFFFFF0,
	0xFFFFFFFF
    };

    unsigned I = sizeof (Masks) / sizeof (Masks [0]) - 1;
    while ((Code1 & Masks [I]) != (Code2 & Masks [I])) {
	/* Next one */
	I--;
    }

    /* Return the count of matching digits */
    return I;
}



/*****************************************************************************/
/*                                   Code                                    */
/*****************************************************************************/



unsigned GetAreaCodeInfo (acInfo* AC, const char* PhoneNumber)
/* Return - if possible - an information for the area code of the given number.
 * The function returns one of the error codes defined in areacode.h. If the
 * returned value is acOk, the AC struct is filled with the data of the
 * area code found. If we did not have an error, but there is no area code
 * that corresponds to the given number, the function returns acOk, but the
 * AC struct is filled with an empty Info field and a AreaCodeLen of zero.
 */
{
    u32           Phone;                /* PhoneNumber encoded in BCD */
    long          First, Last, Current; /* For binary search */
    u32           CurrentVal;           /* The value at Table [Current] */
    int		  Found;		/* Flag: We've found an exact match */
    unsigned      RC = acOk;            /* Result code of the function */
    unsigned char AreaCodeLen;		/* Length of areacode found */
    u32		  InfoStart;   	       	/* Starting offset of info string */
    AreaCodeDesc  Desc;


    /* Clear the fields of the AC struct. Write a zero to the last field of
     * Info - this field is never written to by the rest of the code. So by
     * setting this to zero, we will assure a terminated string in case some
     * problem prevents the code below from executing correctly.
     */
    AC->Info [0]  = '\0';
    AC->Info [sizeof (AC->Info) - 1] = '\0';
    AC->AreaCodeLen = 0;

    /* Convert the phone number into the internal representation. If the
     * number is invalid, return immidiately. This will also catch an empty
     * phone number, so the rest of the code may safely assume that phone
     * has a value that makes sense.
     */
    Phone = EncodeNumber (PhoneNumber);
    if (Phone == 0xFFFFFFFF) {
       	/* Invalid number */
       	return acInvalidInput;
    }

    /* Open the database file, check for errors */
    Desc.F = fopen (acFileName, "rb");
    if (Desc.F == 0) {
        /* We had an error opening the file */
        return acFileError;
    }

    /* Initialize descriptor data where needed */
    Desc.Table = 0;

    /* Read the header from the file */
    RC = LoadFileHeader (&Desc);
    if (RC != acOk) {
        /* Wrong file or file read error */
        goto ExitWithClose;
    }

    /* Add dead code to work around gcc warnings */
    Current    = 0;
    CurrentVal = 0;

    /* Now do a (eventually repeated) binary search over the data */
    Found   = 0;
    First   = 0;
    do {

	Last    = (long) Desc.Count - 1;
     	while (First <= Last && Found == 0) {

	    /* If we don't have read the table into memory, check if we can do
	     * so now.
	     */
	    if (Desc.Table == 0) {
		u32 NeedMemory = (Last - First + 1) * sizeof (u32);
		if (NeedMemory <= acMaxMem) {
		    /* Ok, the current part of the table is now small enough to
		     * load it into memory.
		     */
		    Desc.First = First;
	    	    Desc.Last  = Last;
		    LoadTable (&Desc);
		}
	    }

	    /* Set current to mid of range */
	    Current = (Last + First) / 2;

	    /* Get the phone number from that place */
	    CurrentVal = ReadPhone (&Desc, Current);

	    /* Do a compare */
	    if (Phone > CurrentVal) {
		First = Current + 1;
	    } else {
		Last = Current - 1;
		if (Phone == CurrentVal) {
     		    /* Exact match (whow!) - terminate the loop */
		    Found = 1;
	 	}
	    }
	}

	/* If we don't have an exact match, we check for a partially one. If
	 * Found is not true, the loop above will terminate with First > Last.
	 * Beware: This means that the index is eventually invalid!
	 */
	if (Found == 0) {

	    unsigned MatchingDigits;		/* Count of matching digits */

	    /* Set the new current index and check if it is valid */
	    Current = First;
	    if (Current >= (long) Desc.Count) {
	     	/* Not found */
	     	goto ExitWithClose;
	    }

	    /* The index is valid, load the value */
	    CurrentVal = ReadPhone (&Desc, Current);

	    /* Calculate the length of the area code */
	    AreaCodeLen = CalcCodeLen (CurrentVal);
	    assert (AreaCodeLen > 0);

     	    /* Check if the prefix is actually the first part of the phone
	     * number. If so, we've found a match.
	     */
       	    MatchingDigits = CalcMatchingDigits (Phone, CurrentVal);
	    if (MatchingDigits >= AreaCodeLen) {

       	       	/* Match! */
	     	Found = 1;

	    } else {

	     	/* Ok, now comes the tricky part: Since we allow numbers that
	     	 * do completely contain other numbers (e.g. 0123 --> AAAA,
	     	 * 01239 --> BBBB), we may have found the longer number and
	     	 * this caused the mismatch. There maybe a match if we remove
	     	 * one or more digits from the number.
		 * Because empty digits are filled with hex 'F', the shorter
		 * number (if one exists) comes *after* the one we found
    		 * already. If there is a shorter number, it has - as a
		 * maximum - as many digits as were matching between the
     		 * number we searched for, and the one we found. Since we have
		 * the length of the shortest number, contained in the data
	 	 * file in the header, we can stop, if the matching digits get
		 * below or equal to this value.
		 */
		if (MatchingDigits < Desc.MinLength) {
		    /* No match! */
		    goto ExitWithClose;
		}

		/* Replace all non-matching digits by hex 'F', and start again
		 * searching, beginning after the current value.
		 */
		Phone |= 0xFFFFFFFF >> (MatchingDigits * 4);
		First = Current + 1;
	    }

	}

    } while (Found == 0);


    /* Ok, we have now definitely found the code. Current is the index of the
     * area code. Seek to the corresponding position in the name index, get
     * the name position and the area code length from there (which is encoded
     * together with the offset in a three byte value). To be more compatible
     * with future versions, there is a field in the header that says, how big
     * the area code specific data at that place is.
     */
    fseek (Desc.F, Desc.InfoIndexStart + Current * Desc.CodeDataSize, SEEK_SET);
    InfoStart = Load_u24 (&Desc);

    /* The real area code length is in bit 20-23 of the value just read */
    AC->AreaCodeLen = (unsigned) ((InfoStart & 0xF00000) >> 20) + 1;
						       
    /* Seek to the start of the info */
    fseek (Desc.F, Desc.InfoStart + (InfoStart & 0xFFFFF), SEEK_SET);

    /* Zero terminated info string follows. */
    LoadString (&Desc, AC->Info, sizeof (AC->Info));

#ifdef CHARSET_CP850
    /* Translate the info to the CP850 charset */
    {
	unsigned char *S = (unsigned char*) AC->Info;
	while (*S) {
            if (*S >= 128) {
                *S = CP850Map [*S - 128];
            }
	    S++;
        }
    }
#endif

ExitWithClose:
    /* Free the table memory if allocated */
    free (Desc.Table);

    /* Close the data file */
    fclose (Desc.F);

    /* Done, return the result */
    return RC;
}



