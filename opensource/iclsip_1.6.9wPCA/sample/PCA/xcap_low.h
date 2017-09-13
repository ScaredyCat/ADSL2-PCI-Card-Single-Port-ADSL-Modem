#ifndef _XCAPLOW
#define _XCAPLOW

#include <windows.h>
#include <winsock.h>

bool	GetINISetting(char *s_ip,int *s_port);
bool	init();
void	idle();
bool	closeSock(int sd);
int	doConnect();
int	fillPUTReqHdr( char *msg,char *reqURI,unsigned int contentLen);
int	fillGETReqHdr( char *msg,char *reqURI);
int	fillDelReqHdr( char *msg,char *reqURI);
int	sendPUTReq(char *hdr,char *content,char *recvbuf,int sd);
int	sendGETReq(char *hdr,char *recvbuf, int sd);
int	sendDELReq(char *hdr,char *recvbuf, int sd);
int	getFileSize(const char*);
void	SetXCAPServer(char *s_ip,unsigned int s_port);
void	composeURINodeSel(char *retBuf,char *AUID,char *listname,char *entry,char *attr);
void	composeURIDOCSel(char *retBuf,char *rootURI,char *AUID,char *owner,char *filename);

//for testing..
void	testPUTReq();
void	testGETReq();

#endif 
