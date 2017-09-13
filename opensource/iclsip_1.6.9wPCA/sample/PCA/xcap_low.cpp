#include "stdafx.h"

#include <windows.h>
#include <winsock.h>
#include <string>
#include <sstream>
#include "xcap_low.h"
#include "StringTokenizer.h"
#include "EscapeCoding.h"

#pragma warning(disable:4786)	// disable VC warnings about STL

#include <iostream>
#include <string>
#include <vector>

using namespace std;

#define BUF_SIZE	65536
#define WEB_HOST	"140.96.102.162"
#define WEB_PORT	80
#define DEFAULT_FILENAME "index.xml"
#define DEFAULT_AUID	"resource-lists"
#define DEFAULT_ROOTURI "xcap"

char ServerIP[32];
int  ServerPort;
bool isServerCfg = false;
bool isWrite = false;

bool GetINISetting(char *s_ip, int *s_port)
{
	CWinApp* pApp = AfxGetApp();

	CString strLMSIP =  pApp->GetProfileString( "", "LMSAddress", WEB_HOST);
	
	strcpy(s_ip, strLMSIP);
	*s_port = pApp->GetProfileInt( "","LMSPort", WEB_PORT );

	return true;
}

void SetXCAPServer(char *s_ip, unsigned int s_port)
{
	if (s_ip != NULL) {
		strcpy(ServerIP,s_ip);
		ServerPort = s_port;
	}
	else {
		if (isServerCfg == true)
			return;
		if (!GetINISetting(ServerIP, &ServerPort)) {
			memset(ServerIP,0,32);
			strcpy(ServerIP,WEB_HOST);
			ServerPort = WEB_PORT;
		}
		isServerCfg = true;
	}
}

int string_split(const std::string &delim, const std::string &str,
                 std::vector<std::string> *v)
{
	if (str.size() == 0)
		return 0;

	if (v)
		v->clear();

	int	n = 0;
	string::size_type	p1, p2;

	for (p1 = 0, n = 0; p1 != string::npos; n++) {
		p2 = str.find_first_not_of(delim, p1);
		if (p2 == string::npos)
			break;

		p1 = str.find_first_of(delim, p2);

		if (v)
			v->push_back(str.substr(p2, p1 - p2));
	}

	return n;
}

bool init()
{
	WSADATA   wsaData;

	//set default xcap server info
	SetXCAPServer(NULL,0);
	
	// Setup Winsock 1.1
	if( WSAStartup( MAKEWORD(1, 1), &wsaData ) )
		return false;

	if( LOBYTE(wsaData.wVersion) != 1 ||
	    HIBYTE(wsaData.wVersion) != 1 ) {
		// Couldn't find an acceptable WinSock DLL
		return false;
	}
	
	return true;
}

bool closeSock(int sd)
{
	closesocket(sd);
	return true;	
}

// return sd
int doConnect()
{
	int sd, res;

	// Open a TCP socket (an Internet stream socket)
	if( (sd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
		cout << "ERROR! socket failed." << endl;
		return -1;
	}

	struct sockaddr_in saddr;
	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = inet_addr(ServerIP);
	saddr.sin_port = htons((u_short)ServerPort);

	// Connect to WEB_HOST:WEB_PORT
	res = connect(sd, (struct sockaddr *)&saddr, sizeof(saddr));
	if( res == SOCKET_ERROR ) {
		cout << "ERROR! connect failed." << endl;
		closesocket(sd);
		return -1;
	}

	return sd;
}

//compose URI Document Selector part
void composeURIDOCSel(char *retBuf,char *rootURI,char *AUID,char *owner,char *filename)
{
	if (rootURI == NULL)
		strcpy(retBuf,DEFAULT_ROOTURI);
	else
		strcpy(retBuf,rootURI);
	strcat(retBuf,"/");

	if (AUID!=NULL)
		strcat(retBuf,AUID);
	else
		strcat(retBuf,DEFAULT_AUID);
	strcat(retBuf,"/");
	if (owner!=NULL) {
		strcat(retBuf,"users/");
		strcat(retBuf,owner);
		strcat(retBuf,"/");
	}
	if (filename!=NULL)
		strcat(retBuf,filename);
	else
		strcat(retBuf,DEFAULT_FILENAME);
}

//compose URI Node Selector
void composeURINodeSel(char *retBuf,char *AUID,char *listname,char *entry,char *attr)
{
	if (AUID!=NULL)
		strcpy(retBuf,AUID);
	else
		strcpy(retBuf,DEFAULT_AUID);
	

	if (listname!=NULL) {
		strcat(retBuf,"/");
		strcat(retBuf,listname);
		if (entry!=NULL){
			strcat(retBuf,"/");
			strcat(retBuf,entry);
			if (attr!=NULL) {
				strcat(retBuf,"/");
				strcat(retBuf,attr);
			}
		}
			
	}
	std::string tmpString = retBuf, retString;
	
	retString = Escape(tmpString);
	strcpy(retBuf,retString.c_str());
} 

//return msg len
int fillPUTReqHdr( char *msg, char *reqURI, unsigned int contentLen)
{
	// Request-URI
	char buf[256];

	if (reqURI!=NULL)
		strcat(msg, reqURI);

	// Accept
	strcat(msg, "Accept: */*\r\n");

	// Accept-Language
	strcat(msg, "Accept-Language: zh-tw\r\n");
	
	// Accept-Encoding
	strcat(msg, "Accept-Encoding: gzip, deflate\r\n");

	// User-Agent
	strcat(msg, "User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; .NET CLE 1.1.4322)\r\n");

	// Host
	memset(buf, 0, sizeof(buf));
	sprintf(buf,"Host: %s\r\n",ServerIP);
	strcat(msg, buf);

	// Content-Length
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "Content-Length: %d\r\n", contentLen);
	strcat(msg, buf);

	// Connection
	strcat(msg, "Connection: keep-alive\r\n");

	// Cache-Control
	strcat(msg, "Cache-Control: no-cache\r\n");

	// End of message header
	strcat(msg, "\r\n");
	return strlen(msg);
}

//return msg len
int fillGETReqHdr( char *msg, char *reqURI)
{
	// Request-URI
	char buf[256];

	if(reqURI!=NULL)
		strcat(msg,reqURI);

	// Accept
	strcat(msg, "Accept: */*\r\n");

	// Accept-Language
	strcat(msg, "Accept-Language: zh-tw\r\n");
	
	// Accept-Encoding
	strcat(msg, "Accept-Encoding: gzip, deflate\r\n");

	// User-Agent
	strcat(msg, "User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; .NET CLE 1.1.4322)\r\n");

	// Host
	memset(buf, 0, sizeof(buf));
	sprintf(buf,"Host: %s\r\n",ServerIP);
	strcat(msg, buf);

	// Connection
	strcat(msg, "Connection: keep-alive\r\n");

	// Cache-Control
	strcat(msg, "Cache-Control: no-cache\r\n");

	// End of message header
	strcat(msg, "\r\n");
	return strlen(msg);
}

int fillDelReqHdr( char *msg,char *reqURI)
{
	// Request-URI
	char buf[256];

	if (reqURI!=NULL)
		strcat(msg, reqURI);

	// Accept
	strcat(msg, "Accept: */*\r\n");

	// Accept-Language
	strcat(msg, "Accept-Language: zh-tw\r\n");
	
	// Accept-Encoding
	strcat(msg, "Accept-Encoding: gzip, deflate\r\n");

	// User-Agent
	strcat(msg, "User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; .NET CLE 1.1.4322)\r\n");

	// Host
	memset(buf, 0, sizeof(buf));
	sprintf(buf,"Host: %s\r\n",ServerIP);
	strcat(msg, buf);
	
	// Connection
	strcat(msg, "Connection: keep-alive\r\n");

	// Cache-Control
	strcat(msg, "Cache-Control: no-cache\r\n");

	// End of message header
	strcat(msg, "\r\n");
	return strlen(msg);
}

int sendPUTReq( char *hdr,char *content, char *recvbuf,int sd)
{
	static char buf[1500];
	struct sockaddr_in addr;
	int addrlen,n;
	
	if(sd <= 0)
		return -1;

	// if another thread is sending now....wait...
	while (isWrite) {
		Sleep(100);
	}
	// set write flag
	isWrite = true;

	if ( (n = send(sd, hdr, strlen(hdr), 0)) == SOCKET_ERROR ) {
		isWrite = false;
		return -1;
	}

	if (content!=NULL) {
		memset(buf, 0, sizeof(buf));
		strcpy(buf,content);
		int len = strlen(buf);
		
		int count=0;
		int i=0;
		while ((n=send(sd, buf+i, len-i, 0)) > 0) {
			i += n;
			count += n;
			if (i == len) {
			}
		}
		if (n==SOCKET_ERROR) {
			isWrite = false;
			return -1;
		}
	}
	
	/*
	 * read a complete HTTP response - header and body
	 */
	int iRet = -1;
	memset(recvbuf, 0, sizeof(recvbuf));
	if ((n = recvfrom(sd,recvbuf,BUF_SIZE,0,(struct sockaddr *)&addr,&addrlen)) 
	    == SOCKET_ERROR )
	{
		isWrite = false;
		return -1;
	}
	else {
		recvbuf[n] = '\0';
		char* rspcode= strstr(recvbuf,"HTTP");
		if(rspcode==NULL) return -1;//no response code..
		char tmp2[16];
		strncpy(tmp2, rspcode+9, 3);
		tmp2[3] = '\0';
		iRet = atoi(tmp2);
	
	}
	isWrite = false;
	return iRet;
}

int sendGETReq( char *hdr, char *recvbuf, int sd)
{
	if (sd<=0)
		return -1;

	// if another thread is sending now....wait...
	while (isWrite) {
		Sleep(100);
	}
	isWrite = true;

	int n, sock_err;
	char msg[BUF_SIZE], buf[BUF_SIZE], tmpbuf[BUF_SIZE];
	msg[0] = '\0';

	if( (n = send(sd, hdr, strlen(hdr), 0)) == SOCKET_ERROR ) {
		sock_err = WSAGetLastError();
		isWrite = false;
		return -1;
	}

	int content_len, rsp_code, count=0;
	memset(buf, 0, sizeof(buf));
	bool isFirst = true;
	while ((n = recv(sd, buf, BUF_SIZE, 0)) > 0) {
		if (isFirst) {
			/* find the response code */
			char* str1= strstr(buf,"HTTP");
			if (str1==NULL) return -1;//no response code..
			char tmp2[16];
			strncpy(tmp2, str1+9, 3);
			tmp2[3] = '\0';
			rsp_code = atoi(tmp2);
			
			/* find content-length */
			char* lenpos = strstr(buf, "Content-Length");
			if (lenpos == NULL) break;//no content
			char* endpos = strstr(lenpos,"\r\n");

			int lenoffset = (int)(endpos - lenpos)-16;
			char tmp1[16];
			strncpy(tmp1, lenpos+16, lenoffset);
			tmp1[lenoffset] = '\0';
			content_len = atoi(tmp1);

			char* str2 = strstr(buf, "\r\n\r\n");
			int pos = (int)(str2 - buf);
			memset(tmpbuf, 0, BUF_SIZE);
			strncpy(tmpbuf, str2+4, (n-pos-4)*sizeof(char));
			strcat(recvbuf, tmpbuf);
			
			count += n-pos-4;
			isFirst = false;
		}
		else {
			memset(tmpbuf,0,BUF_SIZE);
			strncpy(tmpbuf,buf,n*sizeof(char));
			strcat(recvbuf, tmpbuf);
			count += n;
		}
		if (content_len == count) {
			recvbuf[count]='\0';
			break;
		}
		memset(buf, 0, sizeof(buf));
	}

	if (n == SOCKET_ERROR) {
		sock_err = WSAGetLastError();
		isWrite = false;
		return -1;
	}
	isWrite = false;
	return rsp_code;
}

int sendDELReq(char *hdr, char *recvbuf, int sd)
{
	return sendPUTReq(hdr, NULL, recvbuf,sd);
}

int getFileSize(const char* filename)
{
	HANDLE hFile = CreateFile(filename,GENERIC_READ,FILE_SHARE_READ,
                                  NULL,OPEN_EXISTING,NULL,NULL);
        int sz = GetFileSize(hFile, NULL); 
        if (sz == 0xFFFFFFFF) { 
        }
        CloseHandle(hFile);
	return sz;
}

void testPUTReq()
{
}

void testGETReq()
{
}

