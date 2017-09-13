/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * acc_adt.c
 *
 * $Id: acc_adt.c,v 1.4 2006/05/09 03:36:21 tyhuang Exp $
 */
#include <stdio.h>
#include <stdlib.h>

#include <common/cm_def.h>
#include <adt/dx_buf.h>
#include <adt/dx_vec.h>
#include <adt/dx_lst.h>
#include <adt/dx_hash.h>
#include <adt/dx_msgq.h>
#include <adt/dx_str.h>
#include <low/cx_thrd.h>
#include <low/cx_misc.h>

RCODE	accBuf(void)
{
	DxBuffer	b;
	char		m[10] = {"0123456789"};
	char		buf[200];
	int		i,len;
	RCODE		ret = RC_OK;

	b = dxBufferNew(200);
	if(!b) {
		printf( "\n!!ERROR!! %s:%d: dxBufferNew() error.\n",
			__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	for(i=0;i<20;i++) {
		len = dxBufferWrite(b,m,10);
		if( len!=10 ) {
			printf( "\n!!ERROR!! %s:%d: dxBufferWrite() error.\n",
				__CMFILE__,__LINE__);
			ret = RC_ERROR;
			goto BUF_ERR;
		}
	}
	len = dxBufferRead(b,buf,20);
	if( len!=20 || strncmp(buf,m,10)!=0 || strncmp(buf+10,m,10)!=0 ) {
		printf( "\n!!ERROR!! %s:%d: dxBufferRead() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto BUF_ERR;
	}
	len = dxBufferWrite(b,m,10);
	if( len!=10 ) {
		printf( "\n!!ERROR!! %s:%d: dxBufferWrite() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto BUF_ERR;
	}
	len = dxBufferRead(b,buf,200);
	if( len!=190 ) {
		printf( "\n!!ERROR!! %s:%d: dxBufferRead() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto BUF_ERR;
	}
	for(i=0;i<19;i++) {
		if( strncmp(buf+i*10,m,10)!=0 ) {
			printf( "\n!!ERROR!! %s:%d: dxBufferRead() error.\n",
				__CMFILE__,__LINE__);
			ret = RC_ERROR;
			goto BUF_ERR;
		}
	}


BUF_ERR:
	dxBufferFree(b);
	return ret;
}

RCODE	accHash(void)
{
	DxHash	h, hdup;
	RCODE	ret = RC_OK;
	int	i;
	char	*elm,*key;
	char	data[20][5] = { "0","1","2","3","4","5","6","7","8","9" };

	h = dxHashNew(20,0,DX_HASH_CSTRING);
	if(!h) {
		printf( "\n!!ERROR!! %s:%d: dxHashNew() error.\n",
			__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	for( i=0; i<10; i++) {
		if( dxHashAdd(h,data[i],data[i])!=RC_OK ) {
			printf( "\n!!ERROR!! %s:%d: dxHashAdd() error.\n",
				__CMFILE__,__LINE__);
			ret = RC_ERROR;
			goto HASH_ERR;
		}
	}
	elm = dxHashItem(h,data[5]);
	if( !elm || strcmp(elm,data[5])!=0 ) {
		printf( "\n!!ERROR!! %s:%d: dxHashItem() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto HASH_ERR;
	}
	elm = dxHashDel(h,data[5]);
	if( !elm || strcmp(elm,data[5])!=0 ) {
		printf( "\n!!ERROR!! %s:%d: dxHashDel() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto HASH_ERR;
	}
	free(elm);

	StartKeys(h);
	while( (key=NextKeys(h)) ) {
		elm = dxHashItem(h,key);
		if( strcmp(key,elm)!=0 ) {
			printf( "\n!!ERROR!! %s:%d: NextKeys() error.\n",
				__CMFILE__,__LINE__);
			ret = RC_ERROR;
			goto HASH_ERR;
		}
	}

	hdup = dxHashDup( h, NULL );
	if( !hdup ) {
		printf( "\n!!ERROR!! %s:%d: dxHashDup() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto HASH_ERR;
	}
	dxHashFree(hdup,free);

HASH_ERR:
	dxHashFree(h,free);
	return ret;
}


RCODE	accVector(void)
{
	DxVector	v;
	RCODE		ret = RC_OK;
	int		i = 0;
	char		data[20][5] = { "0","1","2","3","4","5","6","7","8","9",
				"10","11","12","13","14","15","16","17","18","19" };

	/* test V_POINTER */
	v = dxVectorNew(100,100,DX_VECTOR_POINTER);
	while(i<20 ) {
		ret = dxVectorAddElement(v,data[i]);
		if( ret!=0 ) {
			printf( "\n!!ERROR!! %s:%d: dxBufferRead() error.\n",
				__CMFILE__,__LINE__);
			ret = RC_ERROR;
			goto VEC_ERR;
		}
		i++;
	}

	if( !dxVectorRemoveElementAt(v,0) ) {
		printf( "\n!!ERROR!! %s:%d: dxVectorRemoveElementAt() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto VEC_ERR;
	}
	if( dxVectorAddElementAt(v,data[0],0)!=RC_OK ) {
		printf( "\n!!ERROR!! %s:%d: dxVectorAddElementAt() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto VEC_ERR;
	}
	if( !dxVectorRemoveElementAt(v,10) ) {
		printf( "\n!!ERROR!! %s:%d: dxVectorRemoveElementAt() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto VEC_ERR;
	}
	if( dxVectorAddElementAt(v,data[10],10)!=RC_OK ) {
		printf( "\n!!ERROR!! %s:%d: dxVectorAddElementAt() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto VEC_ERR;
	}
	if( !dxVectorRemoveElementAt(v,19) ) {
		printf( "\n!!ERROR!! %s:%d: dxVectorRemoveElementAt() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto VEC_ERR;
	}
	if( dxVectorAddElementAt(v,data[19],19)!=RC_OK ) {
		printf( "\n!!ERROR!! %s:%d: dxVectorAddElementAt() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto VEC_ERR;
	}
	if( dxVectorRemoveElement(v,data[19])!=RC_OK ) {
		printf( "\n!!ERROR!! %s:%d: dxVectorRemoveElement() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto VEC_ERR;
	}
	if( dxVectorAddElement(v,data[19])!=RC_OK ) {
		printf( "\n!!ERROR!! %s:%d: dxVectorAddElement() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto VEC_ERR;
	}

	/* error case test */
	/* all case should return NULL */
	if( dxVectorRemoveElementAt(v,20) ) {
		printf( "\n!!ERROR!! %s:%d: dxVectorRemoveElementAt() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto VEC_ERR;
	}
	if( dxVectorAddElementAt(v,data[0],21)==0 ) {
		printf( "\n!!ERROR!! %s:%d: dxVectorAddElementAt() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto VEC_ERR;
	}
	if( dxVectorRemoveElementAt(v,-1) ) {
		printf( "\n!!ERROR!! %s:%d: dxVectorRemoveElementAt() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto VEC_ERR;
	}
	if( dxVectorAddElementAt(v,data[0],-1)==0 ) {
		printf( "\n!!ERROR!! %s:%d: dxVectorAddElementAt() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto VEC_ERR;
	}
	if( dxVectorRemoveElement(v,data[-1])==0 ) {
		printf( "\n!!ERROR!! %s:%d: dxVectorRemoveElement() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto VEC_ERR;
	}

	if( dxVectorGetIndexOf(v,data[15],3)!=15 ) {
		printf( "\n!!ERROR!! %s:%d: dxVectorGetIndexOf() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto VEC_ERR;
	}
	dxVectorAddElementAt(v,data[16],5);
	if( dxVectorGetLastIndexOf(v,data[16],19)<0 ) {
		printf( "\n!!ERROR!! %s:%d: dxVectorGetLastIndexOf() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto VEC_ERR;
	}
	dxVectorRemoveElementAt(v,5);
	
	if( dxVectorGetSize(v)!=20 ) {
		printf( "\n!!ERROR!! %s:%d: dxVectorGetSize() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto VEC_ERR;
	}

VEC_ERR:
	dxVectorFree(v,NULL);
	return ret;
}

RCODE	accLst(void)
{
	DxLst l;
	RCODE ret = RC_OK;
	int   i = 0;
	char  data[20][5] = { "0","1","2","3","4","5","6","7","8","9",
	                      "10","11","12","13","14","15","16","17","18","19" };

	/* test V_POINTER */
	l = dxLstNew(DX_LST_POINTER);
	while(i<20 ) {
		ret = dxLstPutTail(l,data[i]);
		if( ret!=0 ) {
			printf( "\n!!ERROR!! %s:%d: dxBufferRead() error.\n",
				__CMFILE__,__LINE__);
			ret = RC_ERROR;
			goto LST_ERR;
		}
		i++;
	}

	if( !dxLstGetHead(l) ) {
		printf( "\n!!ERROR!! %s:%d: dxLstGetHead() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto LST_ERR;
	}
	if( dxLstPutHead(l,data[0])!=RC_OK ) {
		printf( "\n!!ERROR!! %s:%d: dxLstPutTail() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto LST_ERR;
	}
	if( strcmp(dxLstGetAt(l,10),data[10])!=0 ) {
		printf( "\n!!ERROR!! %s:%d: dxLstGetAt() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto LST_ERR;
	}
	if( dxLstInsert(l,data[10],10)!=10 ) {
		printf( "\n!!ERROR!! %s:%d: dxLstInsert() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto LST_ERR;
	}
	if( strcmp(dxLstGetAt(l,19),data[19])!=0 ) {
		printf( "\n!!ERROR!! %s:%d: dxLstGetAt() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto LST_ERR;
	}
	if( dxLstInsert(l,data[19],19)!=19 ) {
		printf( "\n!!ERROR!! %s:%d: dxLstInsert() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto LST_ERR;
	}
	if( strcmp(dxLstGetTail(l),data[19])!=0 ) {
		printf( "\n!!ERROR!! %s:%d: dxLstGetTail() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto LST_ERR;
	}
	if( dxLstPutTail(l,data[19])!=RC_OK ) {
		printf( "\n!!ERROR!! %s:%d: dxLstPutTail() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto LST_ERR;
	}

	/* error case test */
	/* all case should return NULL */
	if( dxLstGetAt(l,20) ) {
		printf( "\n!!ERROR!! %s:%d: dxLstGetAt() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto LST_ERR;
	}
	if( dxLstInsert(l,data[0],21)!=20 ) {
		printf( "\n!!ERROR!! %s:%d: dxLstInsert() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto LST_ERR;
	}
	if( dxLstGetAt(l,-1) ) {
		printf( "\n!!ERROR!! %s:%d: dxLstGetAt() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto LST_ERR;
	}
	if( dxLstInsert(l,data[0],-1)!=-1 ) {
		printf( "\n!!ERROR!! %s:%d: dxLstInsert() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto LST_ERR;
	}
	if( strcmp(dxLstPeek(l,3),data[3])!=0 ) {
		printf( "\n!!ERROR!! %s:%d: dxLstPeek() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto LST_ERR;
	}
	
	dxLstResetIter(l);
	while(1){
		char *s=dxLstPeekByIter(l);
		if(!s){
			printf( "\n!!ERROR!! %s:%d: dxLstPeekNext() error.\n",
			__CMFILE__,__LINE__);
			ret = RC_ERROR;
			goto LST_ERR;
		}
		if(strcmp(s,data[3])==0)
			break;
	}

	if( dxLstGetSize(l)!=21 ) {
		printf( "\n!!ERROR!! %s:%d: dxLstGetSize() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto LST_ERR;
	}

LST_ERR:
	dxLstFree(l,NULL);
	return ret;
}

#ifndef _WIN32_WCE

DxMsgQ	q;
RCODE	msgq_ret = RC_OK;

int	stopA = 0;
int	stopB = 0;
int	stopC = 0;
int	stopD = 0;
int	stopE = 0;

int	aa = 0;
int	bb = 0;
int	cc = 0;
int	dd = 0;
int	ee = 0;

void* producer1(void* data)
{
	char	d[100];
	int	i = 0;

	memset(d,'A',100);

	while( !stopA && i<500000 ) {
		if( dxMsgQSendMsg(q,d,1)!=1 ) {
			printf( "\n!!ERROR!! %s:%d: dxMsgQSendMsg() error.\n",
				__CMFILE__,__LINE__);
			msgq_ret = RC_ERROR;
			return NULL;
		}
		aa++; i++;
		if( i%50000==0 )
			sleeping(100);
	}

	return NULL;
}


void* producer2(void* data)
{
	char	d[100];
	int	i = 0;

	memset(d,'B',100);

	while( !stopB && i<500000 ) {
		if( dxMsgQSendMsg(q,d,1)!=1 ) {
			printf( "\n!!ERROR!! %s:%d: dxMsgQSendMsg() error.\n",
				__CMFILE__,__LINE__);
			msgq_ret = RC_ERROR;
			return NULL;
		}		
		bb++; i++;
		if( i%50000==0 )
			sleeping(100);
	}

	return NULL;
}

void* consumer1(void* data)
{
	char	d[100];
	int	len;

	memset(d,0,100);

	while(!stopC ) {
		len = dxMsgQGetMsg(q,d,100,0);
		if( len>0 )
			cc++;
		else if( len==0 )
			continue;			
		else {
			printf( "\n!!ERROR!! %s:%d: dxMsgQGetMsg() error.\n",
				__CMFILE__,__LINE__);
			msgq_ret = RC_ERROR;
			return NULL;
		}
	}

	return NULL;
}

void* consumer2(void* data)
{
	char	d[100];
	int	len;

	memset(d,0,100);

	while(!stopD) {
		len = dxMsgQGetMsg(q,d,100,10);
		if( len>0 )
			dd++;
		else if( len==0 )
			continue;
		else {
			printf( "\n!!ERROR!! %s:%d: dxMsgQGetMsg() error.\n",
				__CMFILE__,__LINE__);
			msgq_ret = RC_ERROR;
			return NULL;
		}
	}

	return NULL;
}

void* consumer3(void* data)
{
	char	d[100];
	int	len;

	memset(d,0,100);

	while(!stopE) {
		len = dxMsgQGetMsg(q,d,100,100);
		if( len>0 )
			ee++;
		else if( len==0 )
			continue;
		else {
			printf( "\n!!ERROR!! %s:%d: dxMsgQGetMsg() error.\n",
				__CMFILE__,__LINE__);
			msgq_ret = RC_ERROR;
			return NULL;
		}
	}

	return NULL;
}

RCODE	accMsgq(void)
{
	CxThread	A,B,C,D,E;
	int		i;

	A = B = C = D = E = NULL;
	for( i=0; i<3; i++) {
		q = dxMsgQNew(10000);
				
		A = cxThreadCreate(producer1,NULL);
		B = cxThreadCreate(producer2,NULL);
		C = cxThreadCreate(consumer1,NULL);
		D = cxThreadCreate(consumer2,NULL);
		E = cxThreadCreate(consumer3,NULL);
		if( !A||!B||!C||!D||!E ) {
			printf( "\n!!ERROR!! %s:%d: cxThreadCreate() error.\n",
				__CMFILE__,__LINE__);
			msgq_ret = RC_ERROR;
			break;
		}
		
		printf("\nProcess for %d second, ",(i+1)*2);
		fflush(stdout);		
		sleeping((i+1)*2000);
				
		stopA = stopB = stopC = stopD = stopE = 1;

		cxThreadJoin(A);
		cxThreadJoin(B);
		cxThreadJoin(C);
		cxThreadJoin(D);
		cxThreadJoin(E);

		if( ((aa+bb)-(cc+dd+ee))!=dxMsgQGetLen(q) ) {
			printf( "\n!!ERROR!! %s:%d: Message produced and consumed mismatch.\n",
				__CMFILE__,__LINE__);
			msgq_ret = RC_ERROR;
			break;
		}
		printf("msg in = %d, msg out = %d",(aa+bb),(cc+dd+ee) );
		fflush(stdout);		
		
		aa = bb = cc = dd = ee = 0;
		A = B = C = D = E = NULL;
		stopA = stopB = stopC = stopD = stopE = 0;
		dxMsgQFree(q);		
	}
	
	return msgq_ret;
}

#endif /* _WIN32_WCE */

RCODE	accStr(void)
{
	DxStr	s;
	RCODE	ret = RC_OK;

	s = dxStrNew();
	if( !s ) {
		printf( "\n!!ERROR!! %s:%d: dxStrNew() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto STR_ERR;
	}
	if( dxStrCat(s,"0123456789")!=RC_OK ) {
		printf( "\n!!ERROR!! %s:%d: dxStrCat() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto STR_ERR;
	}
	if( dxStrClr(s)!=RC_OK ) {
		printf( "\n!!ERROR!! %s:%d: dxStrClr() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto STR_ERR;
	}
	if( dxStrCatN(s,"0123456789",5)!=RC_OK ) {
		printf( "\n!!ERROR!! %s:%d: dxStrCatN() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto STR_ERR;
	}
	if( dxStrLen(s)!=5 ) {
		printf( "\n!!ERROR!! %s:%d: dxStrLen() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto STR_ERR;
	}
	if( strcmp(dxStrAsCStr(s),"01234")!=0 ) {
		printf( "\n!!ERROR!! %s:%d: dxStrAsCStr() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto STR_ERR;
	}
	if( dxStrFree(s)!=0 ) {
		printf( "\n!!ERROR!! %s:%d: dxStrFree() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto STR_ERR;
	}
STR_ERR:
	return ret;
}

RCODE	accAdt(void)
{
	RCODE	ret = RC_OK;
	printf( "\n-------------------------------------------------------------\n"
		"<adt> module test\n"
		"-------------------------------------------------------------\n"); 
	printf("\nCASE adt_1: test <dx_buf> module ... "); 
	if( accBuf()==RC_OK )
		printf("!!SUCCESS!!\n");
	else
		ret = RC_ERROR;

	printf( "\nCASE adt_2: test <dx_hash> module ... "); 
	if( accHash()==RC_OK )
		printf("!!SUCCESS!!\n");
	else
		ret = RC_ERROR;

	printf( "\nCASE adt_3: test <dx_vec> module ... "); 
	if( accVector()==RC_OK )
		printf("!!SUCCESS!!\n");
	else
		ret = RC_ERROR;

	printf( "\nCASE adt_4: test <dx_lst> module ... "); 
	if( accLst()==RC_OK )
		printf("!!SUCCESS!!\n");
	else
		ret = RC_ERROR;

#ifndef _WIN32_WCE

	printf( "\nCASE adt_5: test <dx_msgq> module ..."); 
	if( accMsgq()==RC_OK )
		printf(" !!SUCCESS!!\n");
	else
		ret = RC_ERROR;
#endif /* _WIN32_WCE */
	
	printf( "\nCASE adt_6: test <dx_str> module ... "); 
	if( accStr()==RC_OK )
		printf("!!SUCCESS!!\n");
	else
		ret = RC_ERROR;

	return ret;
}
