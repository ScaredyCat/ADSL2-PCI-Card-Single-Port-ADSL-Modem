
void RemoteVideoInit(HWND hWnd);
void RemoteVideoDeInit();
void SetVideoPosition(int ID,int XDest,int YDest);
void GetVideoPosition(int ID,int *XDest,int *YDest);
void RemoteVideoShow(int ID,int Width,int Height,unsigned char *lpBits);
void RemoteVideoShowYUV(int ID,int Width,int Height,unsigned char *Y,unsigned char *Cr,unsigned char *Cb);
void ImageScalePercent(int scale);	//CCKAO 0428
