// VideoBuff.h: interface for the CVideoBuff class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_VIDEOBUFF_H__47B83CC1_366D_11D6_B124_0010B53C4B1A__INCLUDED_)
#define AFX_VIDEOBUFF_H__47B83CC1_366D_11D6_B124_0010B53C4B1A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/*typedef struct __WSABUF {
    u_longlen;     // buffer length
    char FAR *buf; // pointer to buffer
} WSABUF, FAR * LPWSABUF;*/
typedef struct _RINGBUFF {
    unsigned int	leng;     // buffer length
    char  *buf; // pointer to buffer
}RINGBUFF;

#define MAX_TMC_TERMINAL_COUNT 1
#define RING_BUF_SIZE	30
#define RING_BUF_UNIT_SIZE	150000//hola 0806

class CVideoBuff  
{
private:

int		NumOfRingBuff;
//RINGBUFF	RingBuff[200];//[NumOfRingBuff];	//marked by James
RINGBUFF	RingBuff[MAX_TMC_TERMINAL_COUNT][RING_BUF_SIZE];//[NumOfRingBuff];	//James++
RINGBUFF	*TempBuff;
//char RingBuff[20][2048];
//char	RingBuff[NumOfRingBuff][2048];	//maximun 200 packets. each 2048 size
//int		RingBuff_Length[NumOfRingBuff];	//each 	packet's length
int		RB_head[MAX_TMC_TERMINAL_COUNT],RB_tail[MAX_TMC_TERMINAL_COUNT];
int		iToken_Semaphore;
//HANDLE RingBuff_Semaphore;	//marked by james
//HANDLE hMutexBuff; 

//unsigned int	dwMaxQueueSize[MAX_TMC_TERMINAL_COUNT];

	

public:
	CVideoBuff();
	CVideoBuff(int);
	virtual ~CVideoBuff();
	void Initial();
//	int EmptyOrFull();	//marked by james
	int EmptyOrFull(int);	//james++
//	int GetIndex_head();	//marked by james
	int GetIndex_head(int, char**);	//james++
//	int GetIndex_tail();	//marked by james
	int GetIndex_tail(int);	//james++
	int AddToRingBuff(int, char *,int);
	//int GetFromRingBuff(int, char*,int *);//james--
	int GetFromRingBuff(int, char**,int *);//james++
	int PushRingBuffTail(int);
	int GetQueueInfo(unsigned int*);
	int ReFlush(int id);
};

#endif // !defined(AFX_VIDEOBUFF_H__47B83CC1_366D_11D6_B124_0010B53C4B1A__INCLUDED_)
