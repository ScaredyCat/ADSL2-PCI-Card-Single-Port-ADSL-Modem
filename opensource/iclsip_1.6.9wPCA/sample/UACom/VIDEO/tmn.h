
#define PAYLOAD_TYPE			int
#define PAYLOAD_VIDEO_NULL		0				//M
#define PAYLOAD_VIDEO_H261		31
#define PAYLOAD_VIDEO_H263		34
#define PAYLOAD_VIDEO_MPEG4     96

#ifdef STATISTICS
#define STATISTICS_PERIOD	5 //The unit is second
#endif

extern "C" int Video_CallBack(int channel,const char* buff, int len);//CCKAO
int ProcessRTPVideoPacket(unsigned char *Outdata,char *data,int RTP_pkt_len,PAYLOAD_TYPE VideoPayLoadType);

