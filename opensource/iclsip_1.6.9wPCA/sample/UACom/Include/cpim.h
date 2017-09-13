/* 
 * Copyright (C) 2003-2004 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 *
 * 
 * cpim.h
 *
 * $Id: cpim.h,v 1.1 2004/12/23 12:06:52 ljchuang Exp $
 */

/*
 * cpim.h define structure and function handle CPIM
 */

#ifndef CPIM_H
#define CPIM_H


#ifdef  __cplusplus
extern "C" {
#endif

#include "datatype.h"

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

#ifdef _WIN32_WCE
#ifndef BUFSIZ
	#define BUFSIZ  4096
#endif
#endif

typedef struct _CPIMpres* CPIMpres;
typedef struct _CPIMstatus* CPIMstatus;
typedef struct _CPIMstatusExt* CPIMstatusExt;
typedef struct _CPIMcontact* CPIMcontact;
typedef struct _CPIMtuple* CPIMtuple;
typedef struct _CPIMelement* CPIMelement;


/* PRES related API */
CCLAPI CPIMpres CPIMpresNew(IN const char* entity);
CCLAPI RCODE CPIMpresFree(IN CPIMpres);
CCLAPI CPIMpres CPIMpresDup(IN CPIMpres);
CCLAPI char* CPIMpresPrint(IN CPIMpres);
CCLAPI CPIMpres FromXMLtoPRES(IN const char* xmldata, IN size_t len);
CCLAPI char* FromPREStoXML(IN CPIMpres);
CCLAPI const char* GetPRESNsname(IN CPIMpres);
CCLAPI RCODE SetPRESNsname(IN CPIMpres, IN const char* nsname);
CCLAPI const char* GetPRESEntity(IN CPIMpres);
CCLAPI RCODE SetPRESEntity(IN CPIMpres, IN const char* entity);
CCLAPI DxLst GetPRESTupleLst(IN CPIMpres);
CCLAPI RCODE SetPRESTupleLst(IN CPIMpres, DxLst tupleLst);
CCLAPI DxLst GetPRESNoteLst(IN CPIMpres);
CCLAPI RCODE SetPRESNoteLst(IN CPIMpres, DxLst noteLst);
CCLAPI DxHash GetPRESNsHash(IN CPIMpres); /* key = namespace URN , item = prefix */
CCLAPI RCODE SetPRESNsHash(IN CPIMpres, DxHash nsHash);
CCLAPI DxLst GetPRESExtLst(IN CPIMpres);
CCLAPI RCODE SetPRESExtLst(IN CPIMpres, DxLst extLst);


/* TUPLE related API */
CCLAPI CPIMtuple CPIMtupleNew();
CCLAPI RCODE CPIMtupleFree(IN CPIMtuple);
CCLAPI CPIMtuple CPIMtupleDup(IN CPIMtuple);
CCLAPI char* CPIMtuplePrint(IN CPIMtuple, IN const char* nsname);
CCLAPI CPIMstatus GetTupleStatus(IN CPIMtuple);
CCLAPI RCODE SetTupleStatus(IN CPIMtuple, IN CPIMstatus);
CCLAPI CPIMcontact GetTupleContact(IN CPIMtuple);
CCLAPI RCODE SetTupleContact(IN CPIMtuple, IN CPIMcontact);
CCLAPI const char* GetTupleID(IN CPIMtuple);
CCLAPI RCODE SetTupleID(IN CPIMtuple, const char* id);
CCLAPI const char* GetTupleTimestamp(IN CPIMtuple);
CCLAPI RCODE SetTupleTimestamp(IN CPIMtuple, const char* timestamp);
CCLAPI DxLst GetTupleNoteLst(IN CPIMtuple);
CCLAPI RCODE SetTupleNoteLst(IN CPIMtuple, DxLst noteLst);
CCLAPI DxLst GetTupleExtLst(IN CPIMtuple);
CCLAPI RCODE SetTupleExtLst(IN CPIMtuple, DxLst extLst);


/* STATUS related API */
CCLAPI CPIMstatus CPIMstatusNew();
CCLAPI RCODE CPIMstatusFree(IN CPIMstatus);
CCLAPI CPIMstatus CPIMstatusDup(IN CPIMstatus);
CCLAPI char* CPIMstatusPrint(IN CPIMstatus, IN const char* nsname);
CCLAPI short GetStatusBasic(IN CPIMstatus);
CCLAPI RCODE SetStatusBasic(IN CPIMstatus, BOOL val);
CCLAPI DxLst GetStatusExtLst(IN CPIMstatus);
CCLAPI RCODE SetStatusExtLst(IN CPIMstatus, DxLst extLst);


/* CONTACT related API */
CCLAPI CPIMcontact CPIMcontactNew(IN const char* URI);
CCLAPI RCODE CPIMcontactFree(IN CPIMcontact);
CCLAPI CPIMcontact CPIMcontactDup(IN CPIMcontact);
CCLAPI char* CPIMcontactPrint(IN CPIMcontact, IN const char* nsname);
CCLAPI float GetContactPriority(IN CPIMcontact);
CCLAPI RCODE SetContactPriority(IN CPIMcontact, float priority);
CCLAPI const char* GetContactURI(IN CPIMcontact);
CCLAPI RCODE SetContactURI(IN CPIMcontact _this, IN const char* URI);


/* Generic Element related API */
CCLAPI CPIMelement CPIMelementNew();
CCLAPI RCODE CPIMelementFree(IN CPIMelement);
CCLAPI CPIMelement CPIMelementDup(IN CPIMelement);
CCLAPI char* CPIMelementPrint(IN CPIMelement);
CCLAPI const char* GetElementNsname(IN CPIMelement);
CCLAPI RCODE SetElementNsname(IN CPIMelement, IN const char* nsname);
CCLAPI const char* GetElementTag(IN CPIMelement);
CCLAPI RCODE SetElementTag(IN CPIMelement, IN const char* tag);
CCLAPI const char* GetElementVal(IN CPIMelement);
CCLAPI RCODE SetElementVal(IN CPIMelement, IN const char* val);
CCLAPI DxHash GetElementAttrHash(IN CPIMelement);
CCLAPI RCODE SetElementAttrHash(IN CPIMelement, DxHash attrHash);
CCLAPI DxLst GetElementChildLst(IN CPIMelement);
CCLAPI RCODE SetElementChildLst(IN CPIMelement, DxLst childLst);

#ifdef  __cplusplus
}
#endif

#endif