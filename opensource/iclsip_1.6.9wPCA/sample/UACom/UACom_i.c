/* this file contains the actual definitions of */
/* the IIDs and CLSIDs */

/* link this file in with the server and any clients */


/* File created by MIDL compiler version 5.01.0164 */
/* at Mon Jan 22 11:26:14 2007
 */
/* Compiler settings for D:\ITRI\ReleasePack\sip\sample\UACom\UACom.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
*/
//@@MIDL_FILE_HEADING(  )
#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __IID_DEFINED__
#define __IID_DEFINED__

typedef struct _IID
{
    unsigned long x;
    unsigned short s1;
    unsigned short s2;
    unsigned char  c[8];
} IID;

#endif // __IID_DEFINED__

#ifndef CLSID_DEFINED
#define CLSID_DEFINED
typedef IID CLSID;
#endif // CLSID_DEFINED

const IID IID_IUAControl = {0x4A91A134,0x1B60,0x497E,{0x87,0xEE,0x23,0xC9,0x7B,0x2A,0x6D,0x84}};


const IID LIBID_UACOMLib = {0x37F06A57,0x5ED1,0x4F73,{0x9B,0xEA,0x92,0x05,0x0B,0x19,0x5C,0x20}};


const IID DIID__IUAControlEvents = {0x3DFB7930,0x376E,0x44FD,{0x82,0x2A,0x61,0x7C,0xC4,0x76,0x4F,0xFA}};


const CLSID CLSID_UAControl = {0xD851737B,0x22E8,0x4947,{0xB8,0x12,0x5F,0xF1,0xF9,0x21,0x27,0x32}};


#ifdef __cplusplus
}
#endif

