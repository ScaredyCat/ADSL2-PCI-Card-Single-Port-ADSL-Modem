
#include "VideoBuff.h"		//add by cckao
#include "tmn.h"
#include "cclRtpVideo.h"
#include "utility.h"


#ifdef STATISTICS
#include <winbase.h>
#include <stdio.h>

LONGLONG g_preRecvCounter;
FILE *recv_file;
int total_recv_bits;
int total_loss_packets;

int total_packets;
int total_frame_num;

extern LARGE_INTEGER g_videoCounterFreq;
char recv_out_str[3000];
char *recv_out_ptr;
#endif


static PAYLOAD_TYPE VideoPayLoadType = PAYLOAD_VIDEO_NULL;

#if 0
CVideoBuff *RingBuffer = new CVideoBuff(50);
#else
CVideoBuff RingBuffer;
#endif

unsigned int Info_PacketLose[4]={0,0,0,0}; //add by cckao 0804

HANDLE	g_hVideoDecSemaphore_2631[MAX_TMC_TERMINAL_COUNT];	//Decoder HANDLE
char H263DecSemaphore_lpName[4/*MAX_TMC_TERMINAL_COUNT*/][80]={	//semaphore-object
		"Video_263_C1",		"Video_263_C2",		"Video_263_C3",		"Video_263_C4"}; 

int firstp0 = 0;//hola 0708

///////////////add by Alan 941005
int pre_seq_num;
int cur_seq_num;
int is_packet_loss;
int is_I_frame;
/////////////////////////////////

#ifdef SAVE_FILE
FILE *bs_file_3 = NULL;
#endif

#define TEST_DEBUG 0
#define TEST_DEBUG1 0

FILE *ptr_test=NULL;
FILE *ptr_video=NULL;
FILE *ptr_video_des=NULL;
FILE *ptr_video_first=NULL;

extern "C"{			//relational with C
int Video_CallBack(int channel,const char* buff, int len)//CCKAO
{							//channel is not used 
	static	int		iFrameSize = 0;
	static	unsigned int 	wCurSeqNo = 0;
	static	bool	bFirstPacket = true;
	static	bool	bLastPacket  = false;
	static  rtp_hdr_t *pRtpHdr;
	static	char*	pRBHdr;
	static  int		rpt_rtppl_len = 0;
	int			i=0, j=0;



#ifdef STATISTICS
	LARGE_INTEGER currCounter;
	int statistics_bitrate;
#endif
	
#if TEST_DEBUG
	if( ptr_test==NULL )
	{
		ptr_test = fopen("C:\\zz_video_callback.txt","w");
		fprintf(ptr_test,"open file \n\n");
	}

	
#endif

#if TEST_DEBUG1
	if( ptr_video==NULL )
	{
		ptr_video = fopen("C:\\zz_video_rawdata.txt","wb");
		//fprintf(ptr_video,"open file \n\n");
	}
	if( ptr_video_des==NULL )
	{
		ptr_video_des = fopen("C:\\zz_video_frame.txt","w");
		if( ptr_video_des )
			fprintf(ptr_video_des,"open file \n\n");
	}
	if( ptr_video_first==NULL )
	{
		ptr_video_first = fopen("c:\\zz_video_first_raw.txt","wb");
	}
#endif

	if ( len <= 0 || buff==NULL )	
	{
		if( ptr_video_des )
		{
			fprintf(ptr_video_des,"*************** len <= 0 || buff==NULL \n");
		}

		return 0;
	}


	if( ptr_video_first )
	{
		const char *tmp;
		tmp = &buff[12];
		fwrite(tmp,1,len-12,ptr_video_first);
	}


#if TEST_DEBUG	
	if( ptr_test )
	{
		fprintf(ptr_test,"len:%d \n",len);
	}
#endif

	pRtpHdr = (rtp_hdr_t*)buff;

#ifdef ALAN_DEBUG
	if(pRtpHdr==NULL)
		i=0;
#endif

#ifdef STATISTICS
	total_recv_bits = total_recv_bits + 8*len;

	QueryPerformanceCounter(&currCounter);

	if(currCounter.QuadPart -  g_preRecvCounter > STATISTICS_PERIOD*g_videoCounterFreq.QuadPart){ //Time preiod has exceeded
		statistics_bitrate = (int)((float)total_recv_bits/((float)(currCounter.QuadPart - g_preRecvCounter)/(float)g_videoCounterFreq.QuadPart));
		g_preRecvCounter = currCounter.QuadPart;
		total_recv_bits = 0;
		sprintf(recv_out_ptr, "frame_rate = %f, bitrate = %d bps, loss_packets = %d, packet loss rate = %f=\n", (float)total_frame_num/(float)STATISTICS_PERIOD, statistics_bitrate, total_loss_packets, (float)total_loss_packets/(float)total_packets);
		recv_out_ptr = recv_out_str + strlen(recv_out_str);
		total_loss_packets = 0;
		total_packets = 0;	
		total_frame_num = 0;
	}
#endif

#ifdef ALAN_DEBUG
	if(pRtpHdr==NULL)
		i=0;
#endif

	//for compoute packet loss
	cur_seq_num = ntohs(pRtpHdr->seq);

#ifdef STATISTICS
	if(pRtpHdr->m==1)
		total_frame_num++;
#endif

	if( ptr_video_des )
	{
		fprintf(ptr_video_des,"--> pre_seq_num:%d cur_seq_num:%d \n",pre_seq_num,cur_seq_num);
	}

	//////////////add by Alan 941005
	if(pre_seq_num>=0){
		if(cur_seq_num != pre_seq_num + 1){
#ifdef STATISTICS
			total_loss_packets += (cur_seq_num - pre_seq_num - 1);
#endif
			is_packet_loss = 1;

			if( ptr_video_des )
			{
				fprintf(ptr_video_des,"*********** pre_seq_num:%d cur_seq_num:%d \n",pre_seq_num,cur_seq_num);
			}
		}
		else // added by sjhuang
			is_packet_loss = 0;
	}

	pre_seq_num = cur_seq_num;

#ifdef STATISTICS
	total_packets++;
#endif

	//if(enc_vidType == VIDEO_H263) // hola 0709
	if((buff[1]&0x77)== 34)
	{
		VideoPayLoadType = PAYLOAD_VIDEO_H263;
		if( (buff[12] & 0x80) ==0 )	//RTP Mode A  -- rtp (12) + payload (4)
			rpt_rtppl_len = 16;
		else								//RTP Mode B  -- rtp (12) + payload (8)
			rpt_rtppl_len = 20;


		if (  (buff[rpt_rtppl_len]==0) && (buff[rpt_rtppl_len+1]==0) && ((buff[rpt_rtppl_len+2]&0xfc)==0x80) )//H263
			bFirstPacket = true;
		else	
			bFirstPacket = false;

		////////////////add by Alan 941113
		if(bFirstPacket){
			if((buff[rpt_rtppl_len+4]&0x02)==0)
				is_I_frame = 1;
			else
				is_I_frame = 0;
		}
		//////////////////////////////////

	}

	if((buff[1]&0x7f)== 96)//hola 0918 marked
	{
		VideoPayLoadType = 	PAYLOAD_VIDEO_MPEG4;

		if( ptr_video_des )
			{
				fprintf(ptr_video_des," MPEG4 packet:(%x) (%x) (%x) %x (%x) %x\n",
										buff[12]&0xff,buff[13]&0xff,buff[14]&0xff,
										buff[15]&0xff,buff[16]&0xff,buff[17]&0xff);
			}


		if ( (buff[12]==0) && (buff[13]==0) && ( buff[14] == 1) ||
			 (buff[13]==0) && (buff[14]==0) && ( buff[15] == 1) ) // PDA compatible code, by sjhuang 2006/07/20
		{//MP4 Video_object start code(VO_start code or VOP_start_code); buf[12] = 127(padding); //add by Alan 941111
			bFirstPacket = true;
			if( ptr_video_des )
			{
				fprintf(ptr_video_des,"\n ************** bFirstPacket = true \n");
			}
		}
		else	
		{
			if( ptr_video_des )
			{
				fprintf(ptr_video_des,"\n ************** bFirstPacket = false \n");
			}
			bFirstPacket = false;
		}

		////////////////add by Alan 941006
		if(bFirstPacket){
			
			if( (buff[16]&0xC0)==0 ||
				(buff[17]&0xC0)==0 ) // PDA compatible code, by sjhuang 2006/07/20
			{
				is_I_frame = 1;
				if( ptr_video_des )
				{
					fprintf(ptr_video_des,"\n ************** is_I_frame = 1; \n");
				}
			}
			else
			{
				if( ptr_video_des )
				{
					fprintf(ptr_video_des,"\n ************** is_I_frame = 0; \n");
				}
				is_I_frame = 0;
			}
		}

	}
	else
	{
		if( ptr_video_des )
			{
				fprintf(ptr_video_des,"************** %x (buff[1]&0x7f) == 96 error \n",buff[1]);
			}
	}

	if(is_packet_loss==1){
		if((bFirstPacket)&&(is_I_frame==1))
			is_packet_loss = 0;
		else
		{
			if( ptr_video_des )
			{
				fprintf(ptr_video_des,"************** is_packet_loss, bFirstPacket:%d I:%d\n",bFirstPacket,is_I_frame);
			}
			
			return 0;
		}
	}


#ifdef ALAN_DEBUG
	if(pRtpHdr==NULL)
		i=0;
#endif

	// merker bit
		if(pRtpHdr->m)	
			bLastPacket = true; 
		else			
			bLastPacket = false;
		
	//int seqno = htons(pRtpHdr->seq);
		if (bFirstPacket)	//only run at the time that first packet comes or reorder the packet
		{
			RingBuffer.GetIndex_head(i, &pRBHdr);
			wCurSeqNo = ntohs(pRtpHdr->seq);

			iFrameSize = 0;
			//iFrameSize = ProcessRTPVideoPacket((unsigned char *)pRBHdr, wsaBuf.buf, wsaBuf.len, 0);
			iFrameSize = ProcessRTPVideoPacket((unsigned char *)pRBHdr,(char *) buff, len,VideoPayLoadType);

#if TEST_DEBUG			
			if( ptr_test )
			{
				fprintf(ptr_test,"after ProcessRTPVideoPacket:%d \n",iFrameSize);
			}
#endif			

			if (bLastPacket)	//last packet of one frame and ready to add the entire frame to ring buffer
			{
				if (RingBuffer.AddToRingBuff(i, NULL, iFrameSize) >= 0)
				{
					
					

#if TEST_DEBUG
					if( ptr_test )
					{
						fprintf(ptr_test,"first AddToRingBuff success \n");
					}
#endif

					ReleaseSemaphore(g_hVideoDecSemaphore_2631[i], 1, NULL);
				}
				else //del by Alan 941006
				{
#if TEST_DEBUG
					if( ptr_test )
					{
						fprintf(ptr_test," ********** first AddToRingBuff error \n");
					}
#endif
					if( ptr_video_des )
					{
						fprintf(ptr_video_des,"*************** first AddToRingBuff error \n");
					}

					wCurSeqNo = 0; //Next packet must be the first packet
				}

				iFrameSize = 0;
			}
			else
			{
				if( ptr_video_des )
					{
						fprintf(ptr_video_des,"*************** first bLastPacket error \n");
					}
			}

			

			return 0;
		}

	if ( (ntohs(pRtpHdr->seq) == wCurSeqNo+1) & !bFirstPacket)	//right order
	//if ( (pRtpHdr->seq == wCurSeqNo+1) & !bFirstPacket)	//right order
	{
		wCurSeqNo++;
		if(iFrameSize+len>RING_BUF_UNIT_SIZE){
			wCurSeqNo = 0; //Next packet must be	the first packet
			iFrameSize = 0;	
			bFirstPacket = false; //del by Alan 941006

#if TEST_DEBUG
			if( ptr_test )
			{
				fprintf(ptr_test," ********** iFrameSize+len>RING_BUF_UNIT_SIZE \n");
			}
#endif

			if( ptr_video_des )
			{
				fprintf(ptr_video_des,"************** iFrameSize+len>RING_BUF_UNIT_SIZE \n");
			}

			return 0;
		}

		//iFrameSize += ProcessRTPVideo	Packet((unsigned char *)(pRBHdr+iFrameSize),wsaBuf.buf,wsaBuf.len,0);
		iFrameSize += ProcessRTPVideoPacket((unsigned char *)(pRBHdr+iFrameSize),(char *)buff,len,VideoPayLoadType);
			
		if (bLastPacket)	//last packet of one frame and ready to add the entire frame to ring buffer
		{
			if (RingBuffer.AddToRingBuff(i, NULL, iFrameSize) >= 0)		//CCKAO
			{

#if TEST_DEBUG
				if( ptr_test )
					{
						fprintf(ptr_test,"bLastPacket AddToRingBuff success \n");
					}
#endif
				
				ReleaseSemaphore(g_hVideoDecSemaphore_2631[i], 1, NULL);
			}
			else //del by Alan 941006
			{

#if TEST_DEBUG
				if( ptr_test )
					{
						fprintf(ptr_test," ********** bLastPacket AddToRingBuff error \n");
					}
#endif
				
				if( ptr_video_des )
					{
						fprintf(ptr_video_des,"*************** bLastPacket AddToRingBuff error \n");
					}

				wCurSeqNo = 0; //Next packet must be the first packet
			}
			iFrameSize = 0;
			bFirstPacket = false; //del by Alan 941006
		}
		else
		{
			
			if( ptr_video_des )
					{
						fprintf(ptr_video_des,"*************** bLastPacket error \n");
					}
		}
		return 0;
	}
	else	//disorder
	{
		if( ptr_video_des )
			{
				fprintf(ptr_video_des,"************** disorder %d == %d+1 !%d \n",ntohs(pRtpHdr->seq),
										wCurSeqNo,bFirstPacket);
			}
		/*
		(ntohs(pRtpHdr->seq) == wCurSeqNo+1) & !bFirstPacket
		*/
		
		wCurSeqNo = 0; //Next packet must be the first packet
		iFrameSize = 0;
		Info_PacketLose[i]++; //add by cckao 0804

#if TEST_DEBUG
		if( ptr_test )
					{
						fprintf(ptr_test," ********** disorder error \n");
					}
#endif
	
		
		return 0;
	}
	}

}

int ProcessRTPVideoPacket(unsigned char *Outdata,char *data,int RTP_pkt_len,PAYLOAD_TYPE VideoPayLoadType)
{						//Out data is the RingBuffer 		

	static int decoder_ebits=0,decoder_sbits=0;
	//static int decoder_pre_ebits=0; //del by Alan 941113
	int hdrlen=12;//not include payload length
	unsigned char h263payloaddata = (unsigned char)data[hdrlen];

#ifdef SAVE_FILE
	if(bs_file_3==NULL)
		bs_file_3 = fopen("bs_3.cmp", "wb");
#endif
    
	//if(enc_vidType == VIDEO_H263) // hola 0709
	if(VideoPayLoadType == PAYLOAD_VIDEO_H263) // hola 0709
	{
	
		decoder_ebits=h263payloaddata & 0x07;
		decoder_sbits=(h263payloaddata & 0x38)>>3;
		
		if( (h263payloaddata & 0x80) ==0 )
			hdrlen += 4; //mode A
		else 
			hdrlen += 8; //mode B
	
		/*if( (data[hdrlen]==0) && (data[hdrlen+1]==0) && ((data[hdrlen+2]&0xfc)==0x80) ){
			memcpy(Outdata,data+hdrlen,RTP_pkt_len-hdrlen);
			decoder_pre_ebits=decoder_ebits;
			return RTP_pkt_len-hdrlen;
		}*/ //del by Alan 941113
		//if((d_sbits+d_pre_ebits)>0){
		//if((decoder_sbits+decoder_pre_ebits)>0){ //del by Alan 941113
		if(decoder_sbits>0){ //add by Alan 941113
			memcpy(Outdata,data+hdrlen+1,RTP_pkt_len-hdrlen-1);
			//decoder_pre_ebits=decoder_ebits; //del by Alan 941113
			return RTP_pkt_len-hdrlen-1;
		}
		else{
			memcpy(Outdata,data+hdrlen,RTP_pkt_len-hdrlen);
			//decoder_pre_ebits=decoder_ebits; //del by Alan 941113
			return RTP_pkt_len-hdrlen;
		}

		return 0;
	}

	//if(enc_vidType == VIDEO_MPEG4) // hola 0709
	if(VideoPayLoadType == PAYLOAD_VIDEO_MPEG4) // hola 0709
	{
//		int i;

#ifdef ALAN_DEBUG
		if(Outdata==NULL)
			i=0;
#endif

		memcpy(Outdata,data+hdrlen,RTP_pkt_len-hdrlen);

#ifdef SAVE_FILE
		fwrite(Outdata, 1, RTP_pkt_len-hdrlen, bs_file_3);
#endif

		return RTP_pkt_len-hdrlen;
	}
	
	return 0;
}

int	 bitrate =200000;
int  framerate =15; 