/*****************************************************************************/
/*                                                                           */
/*                                AREACODE.H                                 */
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



#ifndef AREACODE_H
#define AREACODE_H



#ifdef __cplusplus
extern "C" {
#endif



/*****************************************************************************/
/*                        Data, structs and constants                        */
/*****************************************************************************/



/* The name of the areacode data file. The default is what is #defined as
 * DATA_FILENAME. If this is not #defined, the default is "areacode.dat",
 * which is probably not what you want. In the latter case set this to
 * the correct filename *before* your first call to GetAreaCodeInfo.
 */
extern char* acFileName;

/* How much dynamic memory is GetAreaCodeInfo allowed to consume? Having less
 * memory means more disk access and vice versa. The function does even work
 * if you set this value to zero. For maximum performance, the function needs
 * 4 byte per area code stored in the data file. The default is 32KB.
 */
extern unsigned long acMaxMem;

/* Result codes of GetAreaCodeInfo */
#define acOk            0       /* Done */
#define acFileError     1       /* Cannot open/read file */
#define acInvalidFile   2       /* The file exists but is no area code data file */
#define acWrongVersion  3       /* Wrong version of data file */
#define acInvalidInput	4	/* Input string is not a number or empty */

/* The result of an area code search */
typedef struct {
    unsigned    AreaCodeLen;    /* The length of the area code found */
    char        Info [256];     /* An info string */
} acInfo;



/*****************************************************************************/
/*                                   Code                                    */
/*****************************************************************************/



unsigned GetAreaCodeInfo (acInfo* /*AC*/ , const char* /*PhoneNumber*/);
/* Return - if possible - an information for the area code of the given number.
 * The function returns one of the error codes defined in areacode.h. If the
 * returned value is acOk, the AC struct is filled with the data of the
 * area code found. If we did not have an error, but there is no area code
 * that corresponds to the given number, the function returns acOk, but the
 * AC struct is filled with an empty Info field and a AreaCodeLen of zero.
 */



#ifdef __cplusplus
}
#endif



/* End of AREACODE.H */

#endif

	
