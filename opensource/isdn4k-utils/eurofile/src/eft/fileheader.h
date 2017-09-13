/* $Id: fileheader.h,v 1.1 1999/06/30 17:18:30 he Exp $ */
/* 
 * The following constants are defined by ETS 300 075 for coding of
 * file headers. Note that the coding and semantics of file headers are
 * outside the scope of the T-protocol but a matter of the application
 * which uses the T-protocol.
 */

/* not regarded as a PI but part of the file and compatible */ 
#define	TDU_PI_FILE_HEADER		0x30

#define	TDU_FH_FILE_TYPE		0x20
#define	TDU_FH_EXECUTION_ORDER		0x21
#define	TDU_FH_TRANSFER_NAME		0x22
#define	TDU_FH_FILE_NAME		0x23
#define	TDU_FH_DATE			0x24
#define	TDU_FH_FILE_LENGTH		0x25
#define	TDU_FH_DESTINATION_CODE		0x26
#define	TDU_FH_FILE_CODING		0x27
#define	TDU_FH_DESTINATION_NAME		0x28
#define	TDU_FH_COST			0x29
#define	TDU_FH_USER_FIELD		0x2a
#define	TDU_FH_LOAD_ADDRESS		0x2b
#define	TDU_FH_EXEC_ADDRESS_ABS		0x2c
#define	TDU_FH_EXEC_ADDRESS_REL		0x2d
#define	TDU_FH_COMPRESSION_MODE		0x2e
#define	TDU_FH_DEVICE			0x2f
#define	TDU_FH_FILE_CHECKSUM		0x30
#define	TDU_FH_AUTHOR_NAME		0x31
#define	TDU_FH_FUTURE_FILE_LENGTH	0x32
#define	TDU_FH_PERMITTED_ACTIONS	0x33
#define	TDU_FH_LEGAL_QUALIFICATION	0x34
#define	TDU_FH_CREATION			0x35
#define	TDU_FH_LAST_READ_ACCESS		0x36
#define	TDU_FH_ID_OF_LAST_MODIFIER	0x37
#define	TDU_FH_ID_OF_LAST_READER	0x38
#define	TDU_FH_RECIPIENT		0x39
#define	TDU_FH_TELEMATIC_FT_VERSION	0x3a


/* File type parameters in file headers known by this implementation */

#define	TDU_FH_FILE_TYPE_DATA		0x42
#define	TDU_FH_FILE_TYPE_DESCRIPTION	0x40
#define	TDU_FH_FILE_TYPE_TEXT		0x44

/* File coding parameters in file headers known by this implementation */

#define	TDU_FH_FILE_CODING_BINARY	0x30
#define	TDU_FH_FILE_CODING_VTX		0x51

/* Permitted action bits in file headers known by this implementation */

#define	TDU_FH_PERMITTED_ACTION_READ	(1 << 0)
#define	TDU_FH_PERMITTED_ACTION_INSERT	(1 << 1)
#define	TDU_FH_PERMITTED_ACTION_REPLACE	(1 << 2)
#define	TDU_FH_PERMITTED_ACTION_EXTEND  (1 << 3)
#define	TDU_FH_PERMITTED_ACTION_ERASE	(1 << 4)
#define	TDU_FH_PERMITTED_ACTION_HIDE    (1 << 5)

#define	TDU_FH_PLEN_NAME 254
#define	TDU_FH_PLEN_DATE 12		


/* classes of file headers used by EUROFILE */

#define TDU_FH_CLASS_EFT_DATA	-1
#define TDU_FH_CLASS_EFT_STRING	-2
#define TDU_FH_CLASS_EFT_LIST	-3
#define TDU_FH_CLASS_EFT_DIR	-4
#define TDU_FH_CLASS_EFT_XDIR	-5
#define TDU_FH_CLASS_EFT_DESCR	-6


#include <sys/stat.h>

struct fileheader_par {
	unsigned char fh_t_name[TDU_PLEN_DESIGNATION+1];
	unsigned char fh_name[TDU_FH_PLEN_NAME+1];
	int fh_type;
	struct stat fh_s;
	int fh_xdirrec; /* to influence writing order of file header parameters*/
};

extern int tdu_fh_get_zero_hdr (struct tdu_stream *, unsigned char *, int);
extern int tdu_fh_get_fd_hdr (struct tdu_stream *, unsigned char *, int);
extern void eft_fh_fwrite( FILE * str, unsigned char * tname, 
			   unsigned char * fname, struct stat * s );
extern int tdu_parse_fh( struct fileheader_par *, struct tdu_buf *);
extern int tdu_fh_parse( struct tdu_stream *, struct tdu_buf *, int maxlen);
extern int tdu_fh_parse_print (struct tdu_stream *, struct tdu_buf *, int);





