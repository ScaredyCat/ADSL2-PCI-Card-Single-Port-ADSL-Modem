/* 
 * Copyright (C) 2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * CTransactionDatabase.h
 *
 * $Id: CTransactionDatabase.h,v 1.10 2004/11/02 14:43:18 ljchuang Exp $
 */
#ifndef __PROXY_TRANSACTION_DB_C__
#define __PROXY_TRANSACTION_DB_C__

#include <sip/cclsip.h>
#include "sipTx.h"

/*
 I add a "_C" at the end of those identifiers
 appear in both C++ and C versions
 Mac
*/

struct structTransactionDB_C;
typedef struct structTransactionDB_C TTransactionDB_C;
typedef TTransactionDB_C * const PCTransactionDB_C;

PCTransactionDB_C CreateTransactionDB ( const int iCapacity , BOOL bMatchACK_OF_2XX_TO_INVITE );
void DestroyTransactionDB ( TTransactionDB_C ** const ppDB );

TxStruct FindMatchingTransactionReq ( PCTransactionDB_C pDB, const SipReq pMsg, BOOL bMatchCancel );
TxStruct FindMatchingTransactionRsp ( PCTransactionDB_C pDB, const SipRsp pMsg );
TxStruct FindMatchingAck( PCTransactionDB_C pDB, const SipRsp pMsg );

RCODE InsertTransaction ( PCTransactionDB_C pDB, TxStruct pTx );
RCODE RemoveTransaction ( PCTransactionDB_C pDB, const TxStruct pTx );

TxStruct FindTxFromInternum( PCTransactionDB_C pDB, unsigned short InterNum );
TxStruct FindTxFromSipTCP( PCTransactionDB_C pDB, SipTCP tcp );

BOOL IsTransactionDbEmpty ( PCTransactionDB_C pDB );

SipViaParm * GetTopMostVia ( void * const pMsg, const BOOL bIsRequest );

#endif /* ifndef __PROXY_TRANSACTION_DB_C__ */

