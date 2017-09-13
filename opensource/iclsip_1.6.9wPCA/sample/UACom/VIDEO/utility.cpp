
#include <stdlib.h>
#include <malloc.h>
#include "stdafx.h"
#include "utility.h"

void RemoteVideoInit(HWND hWnd);
void RemoteVideoDeInit();
void SetVideoPosition(int ID,int XDest,int YDest);
void GetVideoPosition(int ID,int *XDest,int *YDest);
void RemoteVideoShow(int ID,int Width,int Height,unsigned char *lpBits);

// Remote video function ++

HDC hdc=NULL;
int Xpos[4]={0,0,0,0},Ypos[4]={0,0,0,0};

void RemoteVideoInit(HWND hWnd)
{
	hdc =GetWindowDC(hWnd);   // handle of window
}

void RemoteVideoDeInit()
{
}

void SetVideoPosition(int ID,int XDest,int YDest)
{
	Xpos[ID]=XDest;	Ypos[ID]=YDest;
}

void GetVideoPosition(int ID,int *XDest,int *YDest)
{
	*XDest=Xpos[ID];	*YDest=Ypos[ID];
}

static int g_scale =1;					//CCKAO 0428
//extern float g_scale;

void ImageScalePercent(int scale)//CCKAO 0428
{  
	if(scale<=0 || scale > 4)
	return;
	g_scale=scale;
}


void RemoteVideoShow(int ID,int Width,int Height,unsigned char *lpBits)
{
	if(hdc==NULL)	return;

	BITMAPINFOHEADER bmih;
	bmih.biSize		= sizeof(BITMAPINFOHEADER);
	bmih.biWidth	= Width;
	bmih.biHeight	= Height;
	bmih.biPlanes	= 1;	
	bmih.biBitCount = 24;
	bmih.biCompression	= 0;
	bmih.biSizeImage	= Width*Height*3;
	bmih.biXPelsPerMeter= 0;
	bmih.biYPelsPerMeter= 0;
	bmih.biClrUsed = 0;
	bmih.biClrImportant = 0;
	
	//StretchDIBits( hdc ,Xpos[ID],Ypos[ID]+Height,Width,-Height,0,0,Width,Height,lpBits,(BITMAPINFO *)&bmih,DIB_RGB_COLORS,SRCCOPY);
	StretchDIBits( hdc ,Xpos[ID],Ypos[ID]+Height*g_scale,(Width*g_scale),-(Height*g_scale),0,0,Width,Height,lpBits,(BITMAPINFO *)&bmih,DIB_RGB_COLORS,SRCCOPY);
}


BOOL FlushWindows(){
	//::PostMessage( hWnd, WM_PAINT,NULL, NULL );
	//InvalidateRect(&rect,FALSE);
	//	void ValidateRect( LPCRECT lpRect );
	//::RedrawWindow(hWnd,NULL,NULL,RDW_INTERNALPAINT );
	//::UpdateWindow(hWnd);
#if 0
	return InvalidateRect(hWnd,NULL,FALSE);//HWND hWnd,CONST RECT *lpRect,BOOL bErase:erase-background flag
#endif
	return 1;
}

