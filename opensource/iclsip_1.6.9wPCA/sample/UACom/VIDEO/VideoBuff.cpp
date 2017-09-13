// VideoBuff.cpp: implementation of the CVideoBuff class.
//
//////////////////////////////////////////////////////////////////////


#include <malloc.h>
#include <memory.h> 
#include <windows.h>
#include <stdio.h>

#include "VideoBuff.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//extern FILE *ptr_test;
extern FILE *ptr_video;
extern FILE *ptr_video_des;
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CVideoBuff::CVideoBuff()
{
//	NumOfRingBuff=200;	//marked by James
	NumOfRingBuff=RING_BUF_SIZE;	//marked by James
	Initial();
}

CVideoBuff::CVideoBuff(int numofpacket)
{
//	NumOfRingBuff=numofpacket;	//marked by James
	NumOfRingBuff=RING_BUF_SIZE;
	
	Initial();
}

CVideoBuff::~CVideoBuff()
{
#if 0 // marked by sjhuang 2006/03/31, no need to free, because not pointer, it's static array.
	for (int i=0; i<MAX_TMC_TERMINAL_COUNT; i++)
		for (int j=0; j<RING_BUF_SIZE; j++)
		delete RingBuff[i][j].buf;
#endif

//James++
//	for(int i=0;i<NumOfRingBuff;i++){
//		free(RingBuff[i].buf);
//		free(TempBuff);	
//	}
//James--
//	CloseHandle(RingBuff_Semaphore);	//marked by james
	//free(RingBuff_Length);
}

//James++
char	g_szBuf[MAX_TMC_TERMINAL_COUNT][RING_BUF_SIZE][RING_BUF_UNIT_SIZE];
//James--

void CVideoBuff::Initial()
{
	/*for(int i=0;i<NumOfRingBuff;i++){
		RingBuff[i]=(char *)malloc(2048);
		memset(RingBuff[i],0,NumOfRingBuff);
	}
	RingBuff_Length=(char *)malloc(NumOfRingBuff);
	memset(RingBuff_Length,0,NumOfRingBuff);*/
//	RingBuff_Semaphore=CreateSemaphore(NULL,0,RING_BUF_SIZE,NULL);	//marked by james

	//James++
//	TempBuff=(RINGBUFF *)malloc(sizeof(RINGBUFF));
//	TempBuff->leng=2048;
//	TempBuff->buf=(char *)malloc(2048);
//	memset(TempBuff->buf,0,2048);
//	for(int i=0;i<NumOfRingBuff;i++){
//		RingBuff[i].leng=2048;
//		RingBuff[i].buf=(char *)malloc(2048);
//		memset(RingBuff[i].buf,0,2048);
//	}
	for (int i=0; i<MAX_TMC_TERMINAL_COUNT; i++)
	{
		for (int j=0; j<RING_BUF_SIZE; j++)
		{
			RingBuff[i][j].leng = RING_BUF_UNIT_SIZE;
			RingBuff[i][j].buf = (char*) g_szBuf[i][j];
			memset(RingBuff[i][j].buf, 0x00, RING_BUF_UNIT_SIZE);
		}
		RB_head[i]=0;
		RB_tail[i]=0;
//		dwMaxQueueSize[i] = 0;
	}
	//James--

	//RingBuff_Length=(char *)malloc(NumOfRingBuff);
	//memset(RingBuff_Length,0,NumOfRingBuff);
//	RB_head=0;	//marked by james
//	RB_tail=0;	//marked by james
	iToken_Semaphore=1;
	
}

int CVideoBuff::EmptyOrFull(int i)//Full:1  empty:0  normal:-1
{
	//james++
	if (i<0 || i>=MAX_TMC_TERMINAL_COUNT) return -1;	//index error
	//james--

	if( ((RB_head[i]+1)%NumOfRingBuff) == RB_tail[i] )	//RingBuff  Full
		return 1;	
	else if( RB_head[i] == RB_tail[i] )					//RingBuff  Empty 
		return 0;
	else 											//Normal case , have data
		return -1;					
}

int CVideoBuff::GetIndex_head(int i, char** pHdr)
{	
	//james++
	if (i<0 || i>=MAX_TMC_TERMINAL_COUNT) 
		return -1;	//index error
	*pHdr = RingBuff[i][RB_head[i]].buf;
	//james--
	return RB_head[i];
}

int CVideoBuff::GetIndex_tail(int i)
{
	//james++
	if (i<0 || i>=MAX_TMC_TERMINAL_COUNT) return -1;	//index error
	//james--
	return RB_tail[i];
}

int CVideoBuff::AddToRingBuff(int id, char *DataIn,int length)
{
	//HANDLE hMutexBuff;
	//hMutexBuff=OpenMutex(SYNCHRONIZE,FALSE,"MutexToProtectDatabase");//access flag,inherit flag,pointer to mutex-object name
	//WaitForSingleObject(hMutexBuff, INFINITE);

	//james++
	if (id<0 || id>=MAX_TMC_TERMINAL_COUNT) 
	{
		if( ptr_video_des )
		{	
			fprintf(ptr_video_des,"*********************** AddToRingBuff error: id:%d \n",id);
			//fwrite(RingBuff[id][RB_head[id]].buf,1,sizeof( unsigned char)*length,ptr_video);
		}
		return -1;
	}
	//james--
	if( ((RB_head[id]+1)%NumOfRingBuff) != RB_tail[id] )	//RingBuff not Full
	{


		if( ptr_video )
		{	
			//fprintf("%s",RingBuff[id][RB_head[id]].buf);
			fwrite(RingBuff[id][RB_head[id]].buf,1,sizeof( unsigned char)*length,ptr_video);
		}

		if( ptr_video_des )
		{
			fprintf(ptr_video_des,"id:%d RB_head[%d]:%d leng:%d buf:%x %x %x %x %x %x\n",
															id,id,RB_head[id],
															length,
															RingBuff[id][RB_head[id]].buf[0]&0xff,
															RingBuff[id][RB_head[id]].buf[1]&0xff,
															RingBuff[id][RB_head[id]].buf[2]&0xff,
															RingBuff[id][RB_head[id]].buf[3]&0xff,
															RingBuff[id][RB_head[id]].buf[4]&0xff,
															RingBuff[id][RB_head[id]].buf[5]&0xff );
		}

//		memcpy(RingBuff[id][RB_head[id]].buf,DataIn,length);
		RingBuff[id][RB_head[id]].leng=length;
		RB_head[id] = ( (RB_head[id]+1) % NumOfRingBuff ) ;



		
		//RingBuff[id][RB_head[id]].buf

		//ReleaseSemaphore(RingBuff_Semaphore,1,NULL);
//		if(ReleaseSemaphore(RingBuff_Semaphore[id],1,NULL)==0)	//marked by james
//			return 1;	//marked by james
		//ReleaseMutex(hMutexBuff);
	    //CloseHandle(hMutexBuff);
//		if (((RB_head[id] + NumOfRingBuff - RB_tail[id]) % NumOfRingBuff)>(int)dwMaxQueueSize[id])	//james++
//			dwMaxQueueSize[id] = (RB_head[id] + NumOfRingBuff - RB_tail[id]) % NumOfRingBuff;
		return 1;
	}	
	/*else if(((RB_head+1)%NumOfRingBuff) == RB_tail){//RingBuff  is Empty
		memcpy(RingBuff[RB_head].buf,DataIn,length);
		RingBuff[RB_head].leng=length;
		RB_head = ( (RB_head+1) % NumOfRingBuff ) ;
		RB_tail = ( (RB_tail+1) % NumOfRingBuff ) ;
		//ReleaseMutex(hMutexBuff);
	    //CloseHandle(hMutexBuff);
		return 0;									
	}*/
	else {											//RingBuff not do  anything
		//ReleaseMutex(hMutexBuff);
	    //CloseHandle(hMutexBuff);
		if( ptr_video_des )
		{	
			//((RB_head[id]+1)%NumOfRingBuff) != RB_tail[id]
			fprintf(ptr_video_des,"*********************** (id:%d) (RB_head[id]+1) (NumOfRingBuff) (RB_tail[id]) \n",
								id,RB_head[id]+1,NumOfRingBuff,RB_tail[id]);
		}
		
		return -1;	
	}
}

int *pzero=0;
//int CVideoBuff::GetFromRingBuff(int id, char *DataOut,int *OutLength)	//james--
int CVideoBuff::GetFromRingBuff(int id, char** DataOut,int *OutLength)	//james++
{
	//HANDLE hMutexBuff;
	//hMutexBuff=OpenMutex(SYNCHRONIZE,FALSE,"MutexToProtectDatabase");//access flag,inherit flag,pointer to mutex-object name
	//WaitForSingleObject(hMutexBuff, INFINITE);
//	WaitForSingleObject(RingBuff_Semaphore,INFINITE);	//marked by james

	//james++
	if (id<0 || id>=MAX_TMC_TERMINAL_COUNT) return -1;
	//james--

	if( RB_head[id] != RB_tail[id] ){			//RingBuff is not Empty
//		memcpy(DataOut, RingBuff[id][RB_tail[id]].buf, RingBuff[id][RB_tail[id]].leng);//james--
		*DataOut = RingBuff[id][RB_tail[id]].buf;
		*OutLength=RingBuff[id][RB_tail[id]].leng;
//		RB_tail[id] = (RB_tail[id]+1)%NumOfRingBuff;	//james--
		//ReleaseMutex(hMutexBuff);
	    //CloseHandle(hMutexBuff);
		return 1;
	}
	else 
	{
		//OutLength=pzero;
		*OutLength=NULL;
		//memset(DataOut,0,2048);
		//memset(DataOut,0,RING_BUF_UNIT_SIZE);	//james--
		*DataOut = NULL;	//james++
		//ReleaseMutex(hMutexBuff);
	    //CloseHandle(hMutexBuff);
		return -1;						//empty
	}
}

int CVideoBuff::PushRingBuffTail(int id)	//james++
{
	if (id<0 || id>=MAX_TMC_TERMINAL_COUNT) return -1;

	if( RB_head[id] != RB_tail[id] ){			//RingBuff is not Empty
		RB_tail[id] = (RB_tail[id]+1)%NumOfRingBuff;
		return 1;
	}
	else 
		return -1;						//empty
}

int CVideoBuff::GetQueueInfo(unsigned int* pInfo)	//james++
{
	unsigned int dwQueue[MAX_TMC_TERMINAL_COUNT];
	for (int i=0; i<MAX_TMC_TERMINAL_COUNT; i++)
	{
		dwQueue[i] = (RB_head[i] + NumOfRingBuff - RB_tail[i]) % NumOfRingBuff;
//		dwQueue[i][1] = dwMaxQueueSize[i];
	}
	memcpy(pInfo, dwQueue, MAX_TMC_TERMINAL_COUNT);
	return 1;
}

int CVideoBuff::ReFlush(int id)
{
	RB_head[id] =0;
	RB_head[id] =0;
	return 1;
}