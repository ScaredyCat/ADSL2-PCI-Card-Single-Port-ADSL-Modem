/* 
 * Copyright (C) 2000-2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/* 
 * ua_sdp.c
 *
 * $Id: ua_sdp.c,v 1.71 2005/05/04 08:56:19 tyhuang Exp $
 */
#include "ua_cm.h"
#include "ua_mgr.h"
#include "ua_sdp.h"
#include "ua_int.h"
#include <adt/dx_str.h>
#include <stdio.h>

struct uaCodecObj{
	int		codec;		/*specify codec id*/
	int		codecAttr;	/*specify attribute for this codec*/
	int		pTime;		/*specify pTime for this codec */
	int		samplerate;	/*specify codec sample rate add by tyhuang */
	int		channel;	/*specify channel of this codec add by tyhuang */
	char	*codename;	/*specify codec name from sdp add by tyhuang */
	char	*fmtp;		/*specify fmtp for this codec 2003.10.21-add by tyhuang  */
};

struct uaMediaObj{
	UAMediaType	 type;
	char*		 address;
	unsigned int numberofaddr;
	unsigned int port;
	unsigned int bandwidth;
	UAMediaAttr	 MediaAttr;	/*specify attribute for this media*/
	int			 pTime;		/*specify pTime for this media */
	CodecLst	 codeclist; /*specify codec(s) for this media*/
	DxLst		 fmtplist;	/*soecify fmtp*/
	UaSDPMedia	 next; 
};


/*create a new SDP */
/*input the rtp address and port number */

CCLAPI 
SdpSess uaSDPNew(UaMgr		_uamgr,
  	     const char*	addr,		/*rtp address*/
		 CodecLst	audioLst,	/*audio codec list*/
		 const int	audioport,	/*audio port number*/
		 CodecLst	videoLst,	/*video codec list*/
		 const int	videoport,	/*video port number*/
		 const char*	bandwidth)	/*setting for bandwidth */	
		
{
	DxStr tmp;
	RCODE rCode=UASDP_ERROR;
	SdpSess sdp=NULL;
	UaCodec media=NULL;
	int codecnum=0,pos=0,j;
	UaUser user;
	char format[240]={'\0'},*username=NULL;
	char codectype[32]={'\0'};
	unsigned long ticktime=uagettickcount();
	
	tmp=dxStrNew();
	dxStrCat(tmp,"v=0\r\n"); /* for sdp version, default =0 */
	user=uaMgrGetUser(_uamgr);
	
	/* get username if null,just asign a default value*/
	username=(char*)uaUserGetName(user);
	if(username==NULL ||(strlen(username)==0))
		username=USER_AGENT;

	if(strchr(addr,':'))
		sprintf(format,"o=%s %d %d IN IP6 %s\r\n",username,(int)ticktime,(int)ticktime,addr);
	else
		sprintf(format,"o=%s %d %d IN IP4 %s\r\n",username,(int)ticktime,(int)ticktime,addr);

	dxStrCat(tmp,format);
	/*session name*/
	/*assume audio and video using the same connection address*/
	dxStrCat(tmp,"s=Session SDP\r\n");
	if(strchr(addr,':'))
		sprintf(format,"c=IN IP6 %s\r\n",addr);
	else
		sprintf(format,"c=IN IP4 %s\r\n",addr);
	dxStrCat(tmp,format);
	/* bandwidth add by tyhuang 2003.7.15*/
	if(bandwidth){
		sprintf(format,"b=%s\r\n",bandwidth);
		dxStrCat(tmp,format);
	}
	/*time stamp*/
	dxStrCat(tmp,"t=0 0 \r\n");
	/*audio codec */
	if(audioLst != NULL){
		rCode=MediaCodectoStr(audioLst,codectype,32);

		sprintf(format,"m=audio %d RTP/AVP %s 101\r\n",audioport,codectype);
		dxStrCat(tmp,format);

		codecnum=dxLstGetSize(audioLst);
		if(codecnum >0){
			for(pos=0;pos<codecnum;pos++){
				media=(UaCodec)dxLstPeek(audioLst,pos);
				if(NULL == media)
					break;
				/* modify by tyhuang */
				if((media->codecAttr != UAMEDIA_SENDRECV) || (media->pTime >0)){
					if(MediaCodectoPayload(media->codec)!=NULL)
						j=sprintf(format,"a=rtpmap:%s %s/%d",\
						MediaCodectoPayload(media->codec),
						/* MediaCodectoExplan(media->codec),8000); */
						uaCodecGetCodecname(media),media->samplerate);
					else
						j=sprintf(format,"a=rtpmap:%d %s/%d",\
						media->codec,
						/* MediaCodectoExplan(media->codec),8000); */
						uaCodecGetCodecname(media),media->samplerate);
					if(media->channel>0)
						sprintf(format+j,"/%d\r\n",media->channel);
					else
						sprintf(format+j,"\r\n");
					dxStrCat(tmp,format);
				}
				/* add by tyhuang */
				/*sendrecv is the default attribute*/
				if(pos==codecnum-1){
					if(media->codecAttr != UAMEDIA_SENDRECV){
						char* tmpAttr;
						tmpAttr=MediaAttributetoStr(media->codecAttr);
						if(tmpAttr!=NULL){
							sprintf(format,"a=%s\r\n",tmpAttr);
							dxStrCat(tmp,format);
						}
					}
					if(media->pTime >0){
						sprintf(format,"a=ptime:%d\r\n",media->pTime);
						dxStrCat(tmp,format);
					}
					sprintf(format,"a=rtpmap:101 telephone-event/8000/1\r\n");
					dxStrCat(tmp,format);
				}
				if(media->fmtp){
					sprintf(format,"a=fmtp:%s\r\n",media->fmtp);
					dxStrCat(tmp,format);
				}
				
				/* end of add */
				media=NULL;
			}
		}

	}
	/*video codec */
	if(videoLst != NULL){
		rCode=MediaCodectoStr(videoLst,codectype,32);
		sprintf(format,"m=video %d RTP/AVP %s\r\n",videoport,codectype);
		dxStrCat(tmp,format);

		codecnum=dxLstGetSize(videoLst);
		if(codecnum >0){
			for(pos=0;pos<codecnum;pos++){
				media=(UaCodec)dxLstPeek(videoLst,pos);
				if(NULL == media)
					break;
				/* modify by tyhuang */
				if((media->codecAttr != UAMEDIA_SENDRECV) || (media->pTime >=0)){
					if(MediaCodectoPayload(media->codec)!=NULL)
						j=sprintf(format,"a=rtpmap:%s %s/%d",\
						MediaCodectoPayload(media->codec),
						/* MediaCodectoExplan(media->codec),90000);*/
						MediaCodectoExplan(media->codec),media->samplerate);
					else
						j=sprintf(format,"a=rtpmap:%d %s/%d",\
						media->codec,
						/* MediaCodectoExplan(media->codec),90000);*/
						media->codename,media->samplerate);
					if(media->channel>0)
						sprintf(format+j,"/%d\r\n",media->channel);
					else
						sprintf(format+j,"\r\n");
					dxStrCat(tmp,format);
				}
				/* add by tyhuang */
				/*video attribute*/
				if(pos==codecnum-1){
					if(media->codecAttr != UAMEDIA_SENDRECV){
						char* tmpAttr;
						tmpAttr=MediaAttributetoStr(media->codecAttr);
						if(tmpAttr!=NULL){
							sprintf(format,"a=%s\r\n",tmpAttr);
							dxStrCat(tmp,format);
						}
					}
					if(media->pTime >0){
						sprintf(format,"a=ptime:%d\r\n",media->pTime);
						dxStrCat(tmp,format);
					}
					
				}
				if(media->fmtp){
					sprintf(format,"a=fmtp:%s\r\n",media->fmtp);
					dxStrCat(tmp,format);
				}
				/* end of add */
				media=NULL;
			}
		}
	}
	if(rCode == RC_OK ){
		sdp=sdpSessNewFromText(dxStrAsCStr(tmp));
	}
	dxStrFree(tmp);
	return sdp;
}

/*get rtp address contained in sdp */
CCLAPI 
RCODE uaSDPGetDestAddress(IN SdpSess _this,IN OUT char* buf,IN int buflen)
{
	RCODE rCode=UASDP_ERROR;
	char  addr[MAXADDRESIZE]={'\0'};
	DxStr tmp;
	SdpTc pConnData;
	int iMedia,i,j,MediaConn;
	SdpMsess Media;
	char *ipaddr=NULL; 

	tmp=dxStrNew();
	if((rCode=sdpSessGetTc(_this,&pConnData)) == RC_OK){
		/*sprintf(addr,pConnData.pConnAddr_);*/
		memcpy(addr, pConnData.pConnAddr_, strlen(pConnData.pConnAddr_) );
		/*dxStrCat(tmp,addr);*/
		ipaddr=strDup(addr);
		/* Do DNS query */
		if ( !ResolveName( &ipaddr ) ){
			UaCoreERR("[uaSDPGetDestAddress] Address resolution fail !\n");
			if(ipaddr) free(ipaddr);
			return RC_ERROR;
		}else{
			dxStrCat(tmp,ipaddr);
			free(ipaddr);
		}
		if(dxStrLen(tmp)<buflen) {
			strcpy(buf,dxStrAsCStr(tmp));
			dxStrFree(tmp);
			return RC_OK;
		} else {
			dxStrFree(tmp);
			return UABUFFER_TOO_SMALL;
		}
	}
	if((dxStrLen(tmp)==0)&&(rCode==UASDP_ERROR)){
		iMedia=sdpSessGetMsessSize(_this);
		for(i =0;i<iMedia;i++){
			Media=sdpSessGetMsessAt(_this,i);
			MediaConn=sdpMsessGetTcSize(Media);
			for(j=0;j<MediaConn;j++){
				rCode=sdpMsessGetTcAt(Media,j,&pConnData);
				sprintf(addr,pConnData.pConnAddr_);
				/*dxStrCat(tmp,addr);*/
				ipaddr=strDup(addr);
				/* Do DNS query */
				if ( !ResolveName( &ipaddr ) ){
					UaCoreERR("[uaSDPGetDestAddress] Address resolution fail !\n");
					if(ipaddr) free(ipaddr);
					return RC_ERROR;
				}else{
					dxStrCat(tmp,ipaddr);
					free(ipaddr);
				}
				if((dxStrLen(tmp)<buflen)&&(rCode==RC_OK)&&(dxStrLen(tmp)!=0)) {
					strcpy(buf,dxStrAsCStr(tmp));
					dxStrFree(tmp);
					return RC_OK;
				}
			}
		}
	}
	dxStrFree(tmp);
	return rCode;
}

/*get audio rtp listen port contained in sdp */
CCLAPI 
int uaSDPGetMediaPort(IN SdpSess _this,IN UAMediaType _mediatype)
{
	SdpMsess Media;
	SdpTm  MediaAnn;
	int iMedia, i;
	
	iMedia=sdpSessGetMsessSize(_this);
	for(i=0;i<iMedia;i++){
		Media=sdpSessGetMsessAt(_this,i);
		sdpMsessGetTm(Media,&MediaAnn);
		switch(_mediatype){
		case UA_MEDIA_AUDIO:
			if(MediaAnn.Tm_type_ == SDP_TM_MEDIA_TYPE_AUDIO)
				return MediaAnn.port_;
			break;
		case UA_MEDIA_VIDEO:
			if(MediaAnn.Tm_type_ == SDP_TM_MEDIA_TYPE_VIDEO)
				return MediaAnn.port_;
			break;
		default:
			break;
		}

	}
	return -1;
}

/*get number of port of media in sdp */
CCLAPI 
int uaSDPGetMediaNumOfPort(IN SdpSess _this,IN UAMediaType _mediatype)
{
	SdpMsess Media;
	SdpTm  MediaAnn;
	int iMedia, i;
	
	iMedia=sdpSessGetMsessSize(_this);
	for(i=0;i<iMedia;i++){
		Media=sdpSessGetMsessAt(_this,i);
		sdpMsessGetTm(Media,&MediaAnn);
		switch(_mediatype){
		case UA_MEDIA_AUDIO:
			if(MediaAnn.Tm_type_ == SDP_TM_MEDIA_TYPE_AUDIO)
				return MediaAnn.numOfPort_;
			break;
		case UA_MEDIA_VIDEO:
			if(MediaAnn.Tm_type_ == SDP_TM_MEDIA_TYPE_VIDEO)
				return MediaAnn.numOfPort_;
			break;
		default:
			break;
		}

	}
	return -1;
}
/* get media ip address 
 * add by tyhuang
 */
CCLAPI 
RCODE uaSDPGetMediaAddr(IN SdpSess _this, IN UAMediaType _mediatype ,IN OUT char* buf,IN int buflen)
{
	SdpMsess Media;
	SdpTm  MediaAnn;
	SdpTc  MediaAddr;
	RCODE rCode=UASDP_ERROR;
	char  addr[MAXADDRESIZE]={'\0'};
	DxStr tmp;
	int iMedia, i,iAddr,j;
	char *ipaddr=NULL;

	tmp=dxStrNew();
	iMedia=sdpSessGetMsessSize(_this);
	for(i=0;i<iMedia;i++){
		Media=sdpSessGetMsessAt(_this,i);
		sdpMsessGetTm(Media,&MediaAnn);

		switch(_mediatype){
		case UA_MEDIA_AUDIO:
			if(MediaAnn.Tm_type_ == SDP_TM_MEDIA_TYPE_AUDIO){
				iAddr=sdpMsessGetTcSize(Media);
				for(j=0;j<iAddr;j++){
					rCode=sdpMsessGetTcAt(Media,j,&MediaAddr);
					sprintf(addr,MediaAddr.pConnAddr_);
					/*dxStrCat(tmp,addr);*/
					ipaddr=strDup(addr);
					/* Do DNS query */
					if ( !ResolveName( &ipaddr ) ){
						UaCoreERR("[uaSDPGetDestAddress] Address resolution fail !\n");
						if(ipaddr) free(ipaddr);
						return RC_ERROR;
					}else{
						dxStrCat(tmp,ipaddr);
						free(ipaddr);
					}
					if((dxStrLen(tmp)<buflen)&&(rCode==RC_OK)&&(dxStrLen(tmp)!=0)) {
						strcpy(buf,dxStrAsCStr(tmp));
						dxStrFree(tmp);
						return RC_OK;
					}
				}
			}
			break;
		case UA_MEDIA_VIDEO:
			if(MediaAnn.Tm_type_ == SDP_TM_MEDIA_TYPE_VIDEO){
				iAddr=sdpMsessGetTcSize(Media);
				for(j=0;j<iAddr;j++){
					rCode=sdpMsessGetTcAt(Media,j,&MediaAddr);
					sprintf(addr,MediaAddr.pConnAddr_);
					/*dxStrCat(tmp,addr);*/
					ipaddr=strDup(addr);
					/* Do DNS query */
					if ( !ResolveName( &ipaddr ) ){
						UaCoreERR("[uaSDPGetDestAddress] Address resolution fail !\n");
						if(ipaddr) free(ipaddr);
						return RC_ERROR;
					}else{
						dxStrCat(tmp,ipaddr);
						free(ipaddr);
					}
					if((dxStrLen(tmp)<buflen)&&(rCode==RC_OK)&&(dxStrLen(tmp)!=0)) {
						strcpy(buf,dxStrAsCStr(tmp));
						dxStrFree(tmp);
						return RC_OK;
					}
				}	
			}
			break;
		default:
			break;
		}
	}
	if(tmp != NULL)
		dxStrFree(tmp);
	return rCode;

}

/* get media codec parameter for media attribute 
 * this function will alloc memory
 * add by tyhuang 
 */
CCLAPI 
UaCodec uaSDPGetMediaCodec(IN SdpSess _this, IN UAMediaType _mediatype ,IN const char *_type)
{
	SdpMsess Media;
	SdpTm  MediaAnn;
	SdpTa  MediaAttr;
	UAMediaAttr returnAttr=UAMEDIA_UNKNOWN;
	UaCodec rCodec=NULL;
	int iMedia, i,iAttr,j,cflag=0;
	char *buf,*tmp;

	iMedia=sdpSessGetMsessSize(_this);
	for(i=0;i<iMedia;i++){
		Media=sdpSessGetMsessAt(_this,i);
		sdpMsessGetTm(Media,&MediaAnn);
		switch(_mediatype){
		case UA_MEDIA_AUDIO:
			if(MediaAnn.Tm_type_ == SDP_TM_MEDIA_TYPE_AUDIO){
				iAttr=sdpMsessGetTaSize(Media);
				rCodec=uaCodecNew(MediaCodectoNum(_type),UAMEDIA_SENDRECV,20,8000,1,NULL);
				/* if there are parameter for codec */
				for(j=0;j<iAttr;j++){
					sdpMsessGetTaAt(Media,j,&MediaAttr);
					/*parse codec parameters*/
					if((strICmp("rtpmap",MediaAttr.pAttrib_))==0){
						buf=strchr(MediaAttr.pValue_,':');
						/*check if description exist*/
						if(buf != NULL)
							buf++;
						else
							break;
						/* get codec type */
						tmp=strstr(buf," ");
						if (tmp) 
							*tmp='\0';
						/*check codec type */
					
						if(strICmp(_type,buf)==0)
							cflag=1;
						else
							cflag=0;
				
						if(cflag==1){
							rCodec->codec=MediaCodectoNum(_type);
							buf=(char *)trimWS(tmp+1);
							tmp=strchr(buf,'/');
							/*find sample rate*/
							if(tmp!=NULL)
								*tmp='\0';
							if(rCodec->codename)
								free(rCodec->codename);
							rCodec->codename=strDup(buf);

							if(tmp!=NULL)
								buf=tmp+1;
							/*check if channel exist*/
							tmp=strchr(buf,'/');
							if(tmp!=NULL){
								*tmp++='\0';
								rCodec->channel=atoi(tmp);
							}
							rCodec->samplerate=atoi(buf);
						}

					}/* end if rtpmap*/
					if(cflag==1){
						/*get ptime from attribute */
						if((strICmp("ptime",MediaAttr.pAttrib_))==0)
							rCodec->pTime=atoi(MediaAttr.pValue_+1);
						if((strICmp("fmtp",MediaAttr.pAttrib_))==0)
							rCodec->fmtp=strDup(MediaAttr.pValue_);
						/*get send/receive */
						if((returnAttr=uaSdpGetMediaAttr(MediaAttr.pAttrib_))!=UAMEDIA_UNKNOWN)
							rCodec->codecAttr=returnAttr;
						
					}
						
				}
			
			}
			break;
		case UA_MEDIA_VIDEO:
			if(MediaAnn.Tm_type_ == SDP_TM_MEDIA_TYPE_VIDEO){
				iAttr=sdpMsessGetTaSize(Media);	
				rCodec=uaCodecNew(MediaCodectoNum(_type),UAMEDIA_SENDRECV,20,8000,1,NULL);
				/* if there are parameter for codec */
				for(j=0;j<iAttr;j++){
					/* get Ta */
					sdpMsessGetTaAt(Media,j,&MediaAttr);
					/*parse codec parameters*/
					if((strICmp("rtpmap",MediaAttr.pAttrib_))==0){
						buf=strchr(MediaAttr.pValue_,':');
						/*check if description exist*/
						if(buf != NULL)
							buf++;
						else
							break;
						/* get codec type */
						tmp=strstr(buf," ");
						if (tmp) 
							*tmp='\0';
						
						/*check codec type if match set cflag to 1*/
						
						if(strICmp(_type,buf)==0)
							cflag=1;
						else
							cflag=0;
						
						if(cflag==1){
							/*rCodec->codec=atoi(buf);*/
							rCodec->codec=MediaCodectoNum(_type);
							buf=(char *)trimWS(tmp+1);
							tmp=strchr(buf,'/');
							/*find sample rate*/
							if(tmp!=NULL)
								*tmp='\0';
							if(rCodec->codename)
								free(rCodec->codename);
							rCodec->codename=strDup(buf);

							if(tmp!=NULL)
								buf=tmp+1;
							/*check if channel exist*/
							tmp=strchr(buf,'/');
							if(tmp!=NULL){
								*tmp++='\0';
								rCodec->channel=atoi(tmp);
							}
							rCodec->samplerate=atoi(buf);
						}

					}/* end if rtpmap*/
					if(cflag==1){
						/*get ptime from attribute */
						if((strICmp("ptime",MediaAttr.pAttrib_))==0)
							rCodec->pTime=atoi(MediaAttr.pValue_+1);
						if((strICmp("fmtp",MediaAttr.pAttrib_))==0)
							rCodec->fmtp=strDup(MediaAttr.pValue_);		
						/*get send/receive */
						if((returnAttr=uaSdpGetMediaAttr(MediaAttr.pAttrib_))!=UAMEDIA_UNKNOWN)
							rCodec->codecAttr=returnAttr;
				
					}
				}
			
			}
			break;
		default:
			break;
		}
	}
	return rCodec;

}

/*get media atrribute*/
CCLAPI 
UAMediaAttr uaSDPGetMediaAttr(IN SdpSess _this,IN int _codectype, IN UAMediaType _mediatype)
{
	/*each media type have several attribute*/
	/*which one is needed ??*/
	/*sendonly, recvonly, sendrecv, inactive ??*/
	/*in this fucntion, only check attribute conatin the above four types*/
	SdpMsess Media;
	SdpTm  MediaAnn;
	SdpTa  MediaAttr;
	UAMediaAttr returnAttr=UAMEDIA_UNKNOWN;
	int iMedia, i,iAttr,j;

	iMedia=sdpSessGetMsessSize(_this);
	for(i=0;i<iMedia;i++){
		Media=sdpSessGetMsessAt(_this,i);
		sdpMsessGetTm(Media,&MediaAnn);

		switch(_mediatype){
		case UA_MEDIA_AUDIO:
			if(MediaAnn.Tm_type_ == SDP_TM_MEDIA_TYPE_AUDIO){
				iAttr=sdpMsessGetTaSize(Media);
				for(j=0;j<iAttr;j++){
					sdpMsessGetTaAt(Media,j,&MediaAttr);
					returnAttr=uaSdpGetMediaAttr(MediaAttr.pAttrib_);
					/*should check codec type, not check here*/
					if(returnAttr!=UAMEDIA_UNKNOWN)
						return returnAttr;
				}
			}
			break;
		case UA_MEDIA_VIDEO:
			if(MediaAnn.Tm_type_ == SDP_TM_MEDIA_TYPE_VIDEO){
				iAttr=sdpMsessGetTaSize(Media);
				for(j=0;j<iAttr;j++){
					sdpMsessGetTaAt(Media,j,&MediaAttr);
					returnAttr=uaSdpGetMediaAttr(MediaAttr.pAttrib_);
					/*should check codec type, not check here*/
					if(returnAttr!=UAMEDIA_UNKNOWN)
						return returnAttr;
				}

			}
			break;
		default:
			break;
		}
	}
	return returnAttr;

}

/* */
CCLAPI CodecLst uaSDPGetCodecList(IN SdpSess _this,IN UAMediaType _mediaType)
{
	SdpMsess Media;
	SdpTm  MediaAnn;
	int iMedia,i,j,rc;
	CodecLst _rcodeclst=NULL;
	UaCodec tmpcodec=NULL;


	iMedia=sdpSessGetMsessSize(_this);
	for(i=0;i<iMedia;i++){
		Media=sdpSessGetMsessAt(_this,i);
		sdpMsessGetTm(Media,&MediaAnn);
		switch(_mediaType){
		case UA_MEDIA_AUDIO:
			if(MediaAnn.Tm_type_ == SDP_TM_MEDIA_TYPE_AUDIO){
				if(MediaAnn.fmtSize_>0) _rcodeclst=uaCodecLstNew();
				for(j=0;j<MediaAnn.fmtSize_;j++){
					/*if((rtype=MediaCodectoNum(MediaAnn.fmtList_[j]))==UA_VIDEO_UNKNOWN)
						tmpcodec=uaSDPGetMediaCodec(_this,_mediaType,(const char*)MediaAnn.fmtList_[j]);
					else
						tmpcodec=uaSDPGetMediaCodec(_this,_mediaType,rtype);*/
					tmpcodec=uaSDPGetMediaCodec(_this,_mediaType,(const char*)MediaAnn.fmtList_[j]);
					rc=uaCodecLstAddTail(_rcodeclst,tmpcodec);
					uaCodecFree(tmpcodec);
				}/* end of for loop */
			}/* end of if AUDIO */
			break;
		case UA_MEDIA_VIDEO:
			if(MediaAnn.Tm_type_ == SDP_TM_MEDIA_TYPE_VIDEO){
				if(MediaAnn.fmtSize_>0) _rcodeclst=uaCodecLstNew();
				for(j=0;j<MediaAnn.fmtSize_;j++){
					/*if((rtype=MediaCodectoNum(MediaAnn.fmtList_[j]))==UA_VIDEO_UNKNOWN)
						tmpcodec=uaSDPGetMediaCodec(_this,_mediaType,MediaAnn.fmtList_[j]);
					else
						tmpcodec=uaSDPGetMediaCodec(_this,_mediaType,rtype);
					*/
					tmpcodec=uaSDPGetMediaCodec(_this,_mediaType,(const char*)MediaAnn.fmtList_[j]);
					rc=uaCodecLstAddTail(_rcodeclst,tmpcodec);
					uaCodecFree(tmpcodec);
				}/* end of for loop */
			}/*end of if VIDEO*/
			break;
		default:
			break;
		}/*end of switch case */
	}/* end of for loop */
	return _rcodeclst;
}

/*get one acceptable audio type from sdp */
/*input SdpSess  and user acceptable audiotype combination */
CCLAPI 
int uaSDPGetFirstAcceptCodec(IN SdpSess _this,IN UAMediaType _mediaType,IN const int _type)
{
	SdpMsess Media;
	SdpTm  MediaAnn;
	int iMedia,i,j;

	iMedia=sdpSessGetMsessSize(_this);
	for(i=0;i<iMedia;i++){
		Media=sdpSessGetMsessAt(_this,i);
		sdpMsessGetTm(Media,&MediaAnn);
		switch(_mediaType){
		case UA_MEDIA_AUDIO:
			if(MediaAnn.Tm_type_ == SDP_TM_MEDIA_TYPE_AUDIO){
				for(j=0;j<MediaAnn.fmtSize_;j++){
					switch(MediaCodectoNum(MediaAnn.fmtList_[j])){
					case 0: /*PCM U */
						if(_type == UA_WAV_PCMU)
							return UA_WAV_PCMU;
						break;
					case 8: /*PCM A*/
						if(_type == UA_WAV_PCMA)
							return UA_WAV_PCMA;
						break;
					case 3: /*GSM */
						if(_type == UA_WAV_GSM)
							return UA_WAV_GSM;
						break;
					case 4: /*723 */
						if(_type == UA_WAV_723)
							return UA_WAV_723;
						break;
					case 18: /*729 */
						if(_type & UA_WAV_729)
							return UA_WAV_729;
						break;
					default:
						break;
					}/*end switch case */
				}/*end for loop*/
			} /*if MEDIA_TYPE_AUDIO */
			break;
		case UA_MEDIA_VIDEO:
			if(MediaAnn.Tm_type_ == SDP_TM_MEDIA_TYPE_VIDEO){
				for(j=0;j<MediaAnn.fmtSize_;j++){
					switch(MediaCodectoNum(MediaAnn.fmtList_[j])){
					case 31: /*H.261 */
						if(_type == UA_VIDEO_H261)
							return UA_VIDEO_H261;
						break;
					case 34: /*H.263*/
						if(_type == UA_VIDEO_H263)
							return UA_VIDEO_H263;
						break;
					default:
						/* modified by tyhuang*/
						if(_type == MediaCodectoNum(MediaAnn.fmtList_[j]))
							return _type;
						break;
					}/*end switch case */
				}/*end for loop*/
			} /*if MEDIA_TYPE_AUDIO */
			break;		}
	}
	return UA_WAV_UNKNOWN;
}
/*duplicate a sdp object .add by tyhuang 2003.7.4*/
CCLAPI SdpSess uaSDPDup(IN SdpSess _sdp)
{
	SdpSess newsdp=NULL;

	if(_sdp)
		newsdp=sdpSessDup(_sdp);

	return newsdp;
}

/*free sdp object*/
CCLAPI RCODE uaSDPFree(SdpSess _this)
{
	RCODE rCode=UASDP_NULL;

	if(_this != NULL){
		sdpSessFree(_this);
		rCode=RC_OK;
	}
	return rCode;
}


/*-------------- CODEC operation -------------*/

/*create a new List store Codec*/
CCLAPI 
CodecLst uaCodecLstNew(void)
{
	return dxLstNew(DX_LST_POINTER);
}

/*Free Codec List*/
CCLAPI 
void uaCodecLstFree(IN CodecLst _this)
{
	if(_this != NULL){
		dxLstFree(_this,uaCodecFree);
		_this=NULL;
	}
}

/*Add a Codec into List, put at tail*/
CCLAPI 
RCODE uaCodecLstAddTail(IN CodecLst _this,IN UaCodec _codecobj)
{
	RCODE rCode= UACODEC_LST_NULL;

	if(_this != NULL){
		dxLstPutTail(_this, uaCodecDup(_codecobj) );
		rCode=RC_OK;
	}

	return rCode;
}

/*Add a Codec into List, put at pos*/
/* input (codeclist, pos, codectype)*/
CCLAPI 
RCODE uaCodecLstAddAt(IN CodecLst _this,IN int _pos,IN UaCodec _codecobj)
{
	RCODE rCode=UACODEC_LST_NULL;
	int num=0;

	if(_this != NULL){
		num=dxLstGetSize(_this);
		if(_pos<num)
			dxLstInsert(_this,_codecobj,_pos);
		else
			dxLstPutTail(_this,_codecobj);

		rCode=RC_OK;
	}
	return rCode;
}

/*get a Codec into List, put at pos*/
/* input (codeclist, pos)*/
CCLAPI 
UaCodec uaCodecLstGetAt(IN CodecLst _this,IN int _pos)
{
	
	int num=0;
	UaCodec _rcodec=NULL;
	if(_this != NULL){
		num=dxLstGetSize(_this);
		if(_pos<num)
			_rcodec=dxLstPeek(_this,_pos);
	}
	return _rcodec;
}

/*Remove a Codec from List, remove UAAudioType or UAVideoType*/
CCLAPI 
RCODE uaCodecLstRemove(IN CodecLst _this,IN int mediatype)
{
	RCODE rCode=UACODEC_LST_NULL;
	int num=0,pos=0;
	UaCodec codec=NULL;

	if(_this != NULL){
		num=dxLstGetSize(_this);
		for(pos=0;pos<num;pos++){
			/*find media type and remove it*/
			codec=dxLstPeek(_this,pos);
			if(codec->codec == mediatype){
				codec = (UaCodec)dxLstGetAt(_this,pos);
				uaCodecFree(codec);
				rCode=RC_OK;
				break;
			}
		}
		/*can't find mediatype*/
		rCode=UACODEC_MEDIA_NOT_FOUND;
	}
	return rCode;
}

/*Get Count*/
CCLAPI int uaCodecLstGetCount(IN CodecLst _this)
{
	if(_this != NULL)
		return dxLstGetSize(_this);
	else
		return 0;
}


/*duplicate a sdp and change into hold sdp*/ 
/* int: 0 based on rfc2543, 1 based on rfc3264*/
SdpSess uaSDPHoldSDP(IN SdpSess _sdp,IN int _type)
{
	SdpSess newsdp=NULL;
	SdpTc sdpConnect;
	SdpTo sdpOwner;
	SdpMsess Media;
	SdpTm MediaAnn;
	SdpTa MediaAttr;
	UAMediaAttr returnAttr=UAMEDIA_UNKNOWN;
	int iMedia, i,iAttr,j;

	if(_sdp == NULL){
		UaCoreERR("[uaSDPHoldSDP] sdp is NULL!\n");
		return NULL;
	}
	switch(_type){
	case 0:/*only change connection into 0.0.0.0 */
		/*duplicate a new sdp*/
		newsdp=sdpSessDup(_sdp);
		if(newsdp==NULL)
			return NULL;

		/*get original connection data*/
		if(sdpSessGetTc(newsdp,&sdpConnect)==RC_OK){
			strcpy(sdpConnect.pConnAddr_,"0.0.0.0");
			sdpSessSetTc(newsdp,&sdpConnect);
		}else{
			UaCoreERR("[uaSDPHoldSDP] connection data in sdp is NULL!\n");
			return NULL;
		}
		break;
	case 1:
		/*duplicate a new sdp*/
		newsdp=sdpSessDup(_sdp);
		if(newsdp==NULL)
			return NULL;

		/*get original o=*/
		if(sdpSessGetTo(newsdp,&sdpOwner)==RC_OK){
			sdpOwner.version_++;
			sdpSessSetTo(newsdp,&sdpOwner);
		}else{
			UaCoreERR("[uaSDPHoldSDP] original in sdp is NULL!\n");
			return NULL;
		}
		/*add media sendonly*/
		iMedia=sdpSessGetMsessSize(newsdp);
		for(i=0;i<iMedia;i++){
			Media=sdpSessGetMsessAt(newsdp,i);
			sdpMsessGetTm(Media,&MediaAnn);
			iAttr=sdpMsessGetTaSize(Media);
			for(j=0;j<iAttr;j++){
				sdpMsessGetTaAt(Media,j,&MediaAttr);
				returnAttr=uaSdpGetMediaAttr(MediaAttr.pAttrib_);
				if(returnAttr!=UAMEDIA_UNKNOWN){
					/*contain sendonly/recvonly/.. attribute*/
					sdpMsessDelTaAt(Media,j);
					/*not sure if the index will change or not*/
					j--;/*assume it will decrease the index number*/
				}
			}
			strcpy(MediaAttr.pAttrib_,"sendonly");
			MediaAttr.sessionatt_flag_=SDP_TA_FLAG_NONE;
			sdpMsessAddTaAt(Media,&MediaAttr,j);
		}
		break;
	case 2:
		/*duplicate a new sdp*/
		newsdp=sdpSessDup(_sdp);
		if(newsdp==NULL)
			return NULL;

		/*get original o=*/
		if(sdpSessGetTo(newsdp,&sdpOwner)==RC_OK){
			sdpOwner.version_++;
			sdpSessSetTo(newsdp,&sdpOwner);
		}else{
			UaCoreERR("[uaSDPHoldSDP] original in sdp is NULL!\n");
			return NULL;
		}
		/*add media sendonly*/
		iMedia=sdpSessGetMsessSize(newsdp);
		for(i=0;i<iMedia;i++){
			Media=sdpSessGetMsessAt(newsdp,i);
			sdpMsessGetTm(Media,&MediaAnn);
			iAttr=sdpMsessGetTaSize(Media);
			for(j=0;j<iAttr;j++){
				sdpMsessGetTaAt(Media,j,&MediaAttr);
				returnAttr=uaSdpGetMediaAttr(MediaAttr.pAttrib_);
				if(returnAttr!=UAMEDIA_UNKNOWN){
					/*contain sendonly/recvonly/.. attribute*/
					sdpMsessDelTaAt(Media,j);
					/*not sure if the index will change or not*/
					j--;/*assume it will decrease the index number*/
				}
			}
			strcpy(MediaAttr.pAttrib_,"recvonly");
			MediaAttr.sessionatt_flag_=SDP_TA_FLAG_NONE;
			sdpMsessAddTaAt(Media,&MediaAttr,j);
		}
		break;
	default:
		break;
	}
	return newsdp;
}

/*-------------- Codec object operation -----------------------*/
/*Create a new codec object*/
/* uaCodecNew(codec-type, attribute, ptime ,samplerate,channel,name) 
 * modified by tyhuang	
 */
CCLAPI 
UaCodec	uaCodecNew(IN int _mediatype, IN UAMediaAttr _attr,IN int _ptime, IN int _sample, IN int _channel,IN char* _codecname)
/*UaCodec	uaCodecNew(IN int _mediatype, IN UAMediaAttr _attr, IN int _ptime)*/
{
	UaCodec _this=NULL;

	/*check input parameter*/
	if(_mediatype <0)
		return _this;
	if(_ptime < 0)
		return _this;
	if(_sample < 0)
		return _this;
	if(_channel < 0)
		return _this;
	
	_this=(UaCodec)calloc(1,sizeof(struct uaCodecObj));
	/* not specify codec */
	if( NULL != _this){
		_this->codename=NULL;
		if(MediaCodectoExplan(_mediatype)!=NULL)
			_this->codename=strDup(MediaCodectoExplan(_mediatype));
		if(_this->codename == NULL)
			if(_codecname)
				_this->codename=strDup(_codecname);
		
		_this->codec=_mediatype;
		_this->codecAttr=_attr;
		_this->pTime=_ptime;
		_this->samplerate=_sample;
		_this->channel=_channel;
		_this->fmtp=NULL;
	}else
		return _this;

	return _this;
}

CCLAPI 
UaCodec uaCodecDup(IN UaCodec _scodec)
{
	UaCodec _this=NULL;

	/*check input parameter*/
	if( NULL == _scodec )
		return NULL;

	_this=(UaCodec)calloc(1,sizeof(struct uaCodecObj));
	if( NULL != _this){
		_this->codec=_scodec->codec;
		_this->codecAttr=_scodec->codecAttr;
		_this->pTime=_scodec->pTime;
		/* set new membor in UaCodec modified by tyhuang */
		_this->samplerate=_scodec->samplerate;
		_this->channel=_scodec->channel;
		if(_scodec->codename != NULL)
			_this->codename=strDup(_scodec->codename);
		else
			_this->codename=NULL;
		
		if(_scodec->fmtp)
			_this->fmtp=strDup(_scodec->fmtp);
		else
			_this->fmtp=NULL;
	}else
		return _this;

	return _this;
}

CCLAPI 
void	uaCodecFree(IN UaCodec _codec)
{
	if(NULL != _codec){
		if(_codec->codename != NULL){
			free(_codec->codename);
			_codec->codename=NULL;
		}
		if(_codec->fmtp != NULL){
			free(_codec->fmtp);
			_codec->fmtp=NULL;
		}
		free(_codec);
		_codec=NULL;
	}
}

/*get codec type*/
CCLAPI 
int	uaCodecGetType(UaCodec _codec)
{
	if(NULL != _codec){
		return _codec->codec;
	}
	return UA_WAV_UNKNOWN;
}

/*get codec attribute*/
CCLAPI 
UAMediaAttr	uaCodecGetAttr(UaCodec _codec)
{
	if(NULL != _codec){
		return _codec->codecAttr;
	}
	return UAMEDIA_UNKNOWN;
}

/*get codec ptime value*/
CCLAPI 
int	uaCodecGetPTime(UaCodec _codec)
{
	if(NULL != _codec){
		return _codec->pTime;
	}
	return -1;
}

/*get codec sample rate 
 * add by tyhuang
 */
CCLAPI
int	uaCodecGetSamplerate(UaCodec _codec)
{
	if(NULL != _codec){
		return _codec->samplerate;
	}
	return -1;
}

/*get codec channel value 
 * add by tyhuang
 */
CCLAPI
int	uaCodecGetChannel(UaCodec _codec)
{
	if(NULL != _codec){
		return _codec->channel;
	}
	return -1;
}

/*get codec name value */
/* add by tyhuang */
CCLAPI
char *	uaCodecGetCodecname(UaCodec _codec)
{
	if(NULL != _codec){
		return _codec->codename;
	}
	return NULL;

}

/*get fmtp value */
/* add by tyhuang */
CCLAPI
char *	uaCodecGetCodecFmtp(UaCodec _codec)
{
	if(NULL != _codec){
		return _codec->fmtp;
	}
	return NULL;

}

CCLAPI 
RCODE	uaCodecSetCodecFmtp(IN UaCodec _codec,IN const char *fmtp)
{
	RCODE rCode=RC_OK;

	if(_codec){
		if(fmtp)
			_codec->fmtp=strDup(fmtp);
	}else{
		rCode=UACODEC_ERROR;
		UaCoreERR("[uaCodecSetCodecFmtp] codec object is NULL!\n");	
	}
	return rCode;
}

CCLAPI 
UaSDPMedia uaSDPMediaNew(IN UAMediaType type,
						 IN const char* address,
						 IN unsigned int numberofaddr,
						 IN unsigned int port,
						 IN unsigned int bandwidth,
						 IN unsigned int ptime,
						 IN UaCodec _codec)
{
	UaSDPMedia _this=NULL;
	/* check parameters */
	if(port == 0){
		UaCoreERR("[uaSDPMediaNew] port is fault!!!\n");
		return _this;
	}
	if(_codec==NULL){
		UaCoreERR("[uaSDPMediaNew] codec is NULL!!!\n");
		return _this;
	}
	/* new object */
	_this=calloc(1,sizeof(struct uaMediaObj));
	if(_this){
		_this->type=type;
		if(address){
			_this->address=strDup(address);
			_this->numberofaddr=numberofaddr;
		}else{
			_this->address=NULL;
			_this->numberofaddr=0;
		}
		_this->port=port;
		_this->bandwidth=bandwidth;
		_this->pTime=ptime;
		_this->codeclist=uaCodecLstNew();
		uaCodecLstAddTail(_this->codeclist,_codec);
		_this->fmtplist=NULL;
	}
	return _this;
}

CCLAPI 
void uaSDPMediaFree(IN UaSDPMedia _this)
{
	if(_this != NULL){
		if(_this->address){
			free(_this->address);
			_this->address=NULL;
		}
		if(_this->codeclist)
			uaCodecLstFree(_this->codeclist);
		if(_this->fmtplist)
			dxLstFree(_this->fmtplist,(void (*)(void *))free);
		free(_this);
		_this=NULL;
	}
}

CCLAPI 
RCODE uaSDPMediaAddCodec(IN UaSDPMedia _this, IN UaCodec _codec)
{
	if(_this == NULL){
		return RC_ERROR;
	}
	if(_codec == NULL){
		return RC_ERROR;
	}
	if(_this->codeclist==NULL)
		_this->codeclist=uaCodecLstNew();
	
	return uaCodecLstAddTail(_this->codeclist,_codec);
}

CCLAPI 
RCODE uaSDPMediaAddCodecAt(IN UaSDPMedia _this,IN int pos, IN UaCodec _codec)
{
	if(_this == NULL){
		return RC_ERROR;
	}
	if(_codec == NULL){
		return RC_ERROR;
	}
	if(pos<0)
		return RC_ERROR;
	if(_this->codeclist==NULL)
		_this->codeclist=uaCodecLstNew();
	return uaCodecLstAddAt(_this->codeclist,pos,_codec);
}

CCLAPI 
UaCodec uaSDPMediaGetCodecAt(IN UaSDPMedia _this,IN int pos)
{
	if(_this == NULL){
		return NULL;
	}
	if(pos<0)
		return NULL;
	if(_this->codeclist == NULL)
		return NULL;

	return uaCodecLstGetAt(_this->codeclist,pos);
}

CCLAPI 
RCODE uaSDPMediaRemoveCodec(IN UaSDPMedia _this,IN int pos)
{
	if(_this == NULL){
		return RC_ERROR;
	}
	if(pos<0)
		return RC_ERROR;
	return uaCodecLstRemove(_this->codeclist,pos);

}
/*Get Count*/
CCLAPI 
int uaSDPMediaGetCodecCount(IN UaSDPMedia _this)
{
	if(_this == NULL){
		return -1;
	}
	if(_this->codeclist==NULL)
		return -1;
	return uaCodecLstGetCount(_this->codeclist);
}

/*get media attribute*/
CCLAPI 
RCODE uaSDPMediaSetAttr(UaSDPMedia _this,UAMediaAttr _mediaattr)
{
	if(_this == NULL)
		return RC_ERROR;
	if((_mediaattr>UAMEDIA_UNKNOWN)||(_mediaattr<UAMEDIA_SENDRECV))
		return RC_ERROR;
	_this->MediaAttr=_mediaattr;
	return RC_OK;
}
/*get media attribute*/
CCLAPI 
UAMediaAttr	uaSDPMediaGetAttr(UaSDPMedia _this)
{
	if(NULL != _this){
		return _this->MediaAttr;
	}
	return UAMEDIA_UNKNOWN;
}

/*get media ptime value*/
CCLAPI 
int	uaSDPMediaGetPTime(UaSDPMedia _this)
{
	if(NULL != _this){
		return _this->pTime;
	}
	return -1;
}

/*get media type value*/
CCLAPI 
UAMediaType	uaSDPMediaGetType(UaSDPMedia _this)
{
	if(NULL != _this)
		return _this->type;
	
	return UA_MEDIA_AUDIO;
}

/*get media ip address value*/
CCLAPI 
char* uaSDPMediaGetAddress(UaSDPMedia _this)
{
	if(NULL != _this)
		return _this->address;
	
	return NULL;
}

/*get media ip address value*/
CCLAPI 
unsigned int uaSDPMediaGetNumOfAddress(UaSDPMedia _this)
{
	if(NULL != _this)
			return _this->numberofaddr;
	return 0;
}

/*get media port value*/
CCLAPI 
unsigned int uaSDPMediaGetPort(UaSDPMedia _this)
{
	if(NULL != _this)
		return _this->port;
	
	return 0;
}

/*get media bandwidth value*/
CCLAPI 
unsigned int uaSDPMediaGetBandwidth(UaSDPMedia _this)
{
	if(NULL != _this)
		return _this->bandwidth;
	
	return 0;
}

CCLAPI 
const char* uaSDPMediaGetFmtp(IN UaSDPMedia _this,IN int pos)
{
	if(_this==NULL)
		return NULL;
	if(_this->fmtplist==NULL)
		return NULL;
	return dxLstPeek(_this->fmtplist,pos);
}
CCLAPI 
int uaSDPMediaGetNumOfFmtp(IN UaSDPMedia _this)
{
	if(_this==NULL)
		return 0;
	if(_this->fmtplist==NULL)
		return 0;
	return dxLstGetSize(_this->fmtplist);
}
CCLAPI 
RCODE	uaSDPMediaAddFmtp(IN UaSDPMedia _this, IN const char*fmtp)
{
	if(_this==NULL)
		return RC_ERROR;
	if(fmtp==NULL)
		return RC_ERROR;
	if(_this->fmtplist==NULL)
		_this->fmtplist=dxLstNew(DX_LST_STRING);

	return dxLstPutTail(_this->fmtplist,(char*)fmtp);
	
}

/*create a new SDP */
/*input the rtp address and port number */

CCLAPI 
SdpSess uaSDPNewEx(UaMgr _uamgr,
				   const char*	addr,	 /*rtp address*/
				   int	numofaddr,
				   UAMediaAttr	attr,
				   UaSDPMedia	_media,	/*media list*/
				   unsigned int bandwidth)	/*setting for bandwidth */	
		
{
	DxStr tmp;
	RCODE rCode=UASDP_ERROR;
	SdpSess sdp=NULL;
	UaCodec pcodec=NULL;
	int codecnum,pos,j;
	UaUser user;
	char format[240]={'\0'},*username=NULL,*ipflag;
	char codectype[32]={'\0'};
	char* tmpAttr;
	unsigned long ticktime=uagettickcount();
	
	if(_uamgr==NULL)
		return NULL;
	if(addr==NULL)
		return NULL;
	if(_media==NULL)
		return NULL;

	tmp=dxStrNew();
	dxStrCat(tmp,"v=0\r\n"); /* for sdp version, default =0 */
	user=uaMgrGetUser(_uamgr);
	
	/* get username if null,just asign a default value*/
	username=(char*)uaUserGetName(user);
	if(username==NULL ||(strlen(username)==0))
		username=USER_AGENT;

	/* session owner */	
	if(strchr(addr,':')) ipflag=strDup("IP6");
	else ipflag=strDup("IP4");

	sprintf(format,"o=%s %d %d IN %s %s\r\n",username,(int)ticktime,(int)ticktime,ipflag,addr);

	dxStrCat(tmp,format);
	/*session name*/
	
	dxStrCat(tmp,"s=Session SDP\r\n");
	/*assume audio and video using the same connection address*/
	if(numofaddr>1)
		sprintf(format,"c=IN %s %s/%d\r\n",ipflag,addr,numofaddr);
	else
		sprintf(format,"c=IN %s %s\r\n",ipflag,addr);
	dxStrCat(tmp,format);
	/* bandwidth add by tyhuang 2003.7.15*/
	if(bandwidth>0){
		sprintf(format,"b=%d\r\n",bandwidth);
		dxStrCat(tmp,format);
	}
	/*time stamp*/
	dxStrCat(tmp,"t=0 0 \r\n");
	if(ipflag){
		free(ipflag);
		ipflag=NULL;
	}
	/* set session attribute */
	
	tmpAttr=MediaAttributetoStr(attr);
	if(tmpAttr){
			sprintf(format,"a=%s\r\n",tmpAttr);
			dxStrCat(tmp,format);
	}
	/*add media */
	if(_media->codeclist){
		rCode=MediaCodectoStr(_media->codeclist,codectype,32);
		sprintf(format,"m=%s %d RTP/AVP %s\r\n",MediaTypetoStr(_media->type),_media->port,codectype);
		dxStrCat(tmp,format);
		
		if(_media->address){
			/*add media address if it exists */
			if(strchr(_media->address,':')) ipflag=strDup("IP6");
			else ipflag=strDup("IP4");

			if(_media->numberofaddr>1)
				sprintf(format,"c=IN %s %s/%d\r\n",ipflag,_media->address,_media->numberofaddr);
			else
				sprintf(format,"c=IN %s %s\r\n",ipflag,_media->address);
			dxStrCat(tmp,format);
		}
		/*add bandwidth if it exists */
		if(_media->bandwidth>0){
			sprintf(format,"b=%d\r\n",_media->bandwidth);
			dxStrCat(tmp,format);
		}
		/*add codec parameter,a=rtpmap */
		codecnum=dxLstGetSize(_media->codeclist);
		if(codecnum >0){
			for(pos=0;pos<codecnum;pos++){
				pcodec=(UaCodec)dxLstPeek(_media->codeclist,pos);
				if(NULL == pcodec)
					break;
				if(MediaCodectoPayload(pcodec->codec)!=NULL)
					j=sprintf(format,"a=rtpmap:%s %s/%d",MediaCodectoPayload(pcodec->codec),uaCodecGetCodecname(pcodec),pcodec->samplerate);
				else
					j=sprintf(format,"a=rtpmap:%d %s/%d",pcodec->codec,uaCodecGetCodecname(pcodec),pcodec->samplerate);
				
				if(_media->type==UA_MEDIA_AUDIO){
					if(pcodec->channel>0)
						sprintf(format+j,"/%d\r\n",pcodec->channel);
				}else
					sprintf(format+j,"\r\n");
				dxStrCat(tmp,format);
				
			}
		}
		/*add fmtp */
		for(pos=0;pos<uaSDPMediaGetNumOfFmtp(_media);pos++){
			sprintf(format,"a=fmtp:%s\r\n",uaSDPMediaGetFmtp(_media,pos));
			dxStrCat(tmp,format);
		}
		/*add other media attribute */
		if(_media->type==UA_MEDIA_AUDIO){
			if(_media->pTime>0){
				sprintf(format,"a=ptime:%d\r\n",_media->pTime);
				dxStrCat(tmp,format);
			}
		}
		/*media status attribute*/
		if(_media->MediaAttr != UAMEDIA_SENDRECV){
			tmpAttr=MediaAttributetoStr(_media->MediaAttr);
			if(tmpAttr!=NULL){
					sprintf(format,"a=%s\r\n",tmpAttr);
					dxStrCat(tmp,format);
			}
		}
	}

	if(rCode == RC_OK ){
		sdp=sdpSessNewFromText(dxStrAsCStr(tmp));
	}
	dxStrFree(tmp);
	if(ipflag)
		free(ipflag);
	return sdp;
}

CCLAPI 
SdpSess uaSDPAddMedia(SdpSess _sdp,UaSDPMedia _media)	
{
	DxStr tmp;
	char sdpbuf[1025]={'\0'};
	unsigned int bodylen=1024;
	UaCodec pcodec=NULL;
	int codecnum,pos,j;
	char format[240]={'\0'},*ipflag;
	char codectype[32]={'\0'};
	RCODE	rcode;
	
	if(_sdp==NULL)
		return NULL;
	if(_media==NULL)
		return _sdp;

	/*serialize SDP to a string */
	rcode=sdpSess2Str(_sdp,sdpbuf,&bodylen);
	if(rcode!=RC_OK){
		UaCoreERR("[uaSDPAddMedia] sdp is too large or parse error!!!\n");
		return NULL;
	}
	
	/*copy old sdp to buf */
	tmp=dxStrNew();
	dxStrCat(tmp,sdpbuf);
	/*add media */
	if(_media->codeclist){
		rcode=MediaCodectoStr(_media->codeclist,codectype,32);
		sprintf(format,"m=%s %d RTP/AVP %s\r\n",MediaTypetoStr(_media->type),_media->port,codectype);
		dxStrCat(tmp,format);
		
		if(_media->address){
			/*add media address if it exists */
			if(strchr(_media->address,':')) ipflag=strDup("IP6");
			else ipflag=strDup("IP4");
			
			if(_media->numberofaddr>1)
				sprintf(format,"c=IN %s %s/%d\r\n",ipflag,_media->address,_media->numberofaddr);
			else
				sprintf(format,"c=IN %s %s\r\n",ipflag,_media->address);
			dxStrCat(tmp,format);
		}
		/*add bandwidth if it exists */
		if(_media->bandwidth>0){
			sprintf(format,"b=%d\r\n",_media->bandwidth);
			dxStrCat(tmp,format);
		}
		/*add codec parameter,a=rtpmap */
		codecnum=dxLstGetSize(_media->codeclist);
		if(codecnum >0){
			for(pos=0;pos<codecnum;pos++){
				pcodec=(UaCodec)dxLstPeek(_media->codeclist,pos);
				if(NULL == pcodec)
					break;
				if(MediaCodectoPayload(pcodec->codec)!=NULL)
					j=sprintf(format,"a=rtpmap:%s %s/%d",MediaCodectoPayload(pcodec->codec),uaCodecGetCodecname(pcodec),pcodec->samplerate);
				else
					j=sprintf(format,"a=rtpmap:%d %s/%d",pcodec->codec,uaCodecGetCodecname(pcodec),pcodec->samplerate);
				if(pcodec->channel>0)
					sprintf(format+j,"/%d\r\n",pcodec->channel);
				else
					sprintf(format+j,"\r\n");
				dxStrCat(tmp,format);
			}
		}
		/*add fmtp */
		for(pos=0;pos<uaSDPMediaGetNumOfFmtp(_media);pos++){
			sprintf(format,"a=fmtp:%s\r\n",uaSDPMediaGetFmtp(_media,pos));
			dxStrCat(tmp,format);
		}
		/*add other media attribute */
		if(_media->pTime>0){
			sprintf(format,"a=ptime:%d\r\n",_media->pTime);
			dxStrCat(tmp,format);
		}
		/*media status attribute*/
		if(_media->MediaAttr != UAMEDIA_SENDRECV){
			char* tmpAttr;
			tmpAttr=MediaAttributetoStr(_media->MediaAttr);
			if(tmpAttr!=NULL){
					sprintf(format,"a=%s\r\n",tmpAttr);
					dxStrCat(tmp,format);
			}
		}
	}
	/*convert to SdpSess */
	if(rcode==RC_OK){
		/*free old sdp */
		sdpSessFree(_sdp);
		_sdp=sdpSessNewFromText(dxStrAsCStr(tmp));
	}
	/*free alloc memory */
	dxStrFree(tmp);
	if(ipflag)
		free(ipflag);
	return _sdp;
}

CCLAPI
int uaSDPGetMediaSize(IN SdpSess _sdp)
{
	return sdpSessGetMsessSize(_sdp);
}

/* this function will alloc memory */
CCLAPI
UaSDPMedia uaSDPGetMedia(IN SdpSess _sdp,IN int pos)
{
	SdpMsess Media;
	SdpTa SDPMediaAttr;
	SdpTb	MediaBandwidth;
	SdpTm  MediaAnn;
	SdpTc pConnData;
	UaSDPMedia	 _uamedia=NULL;
	UAMediaType	 type=UA_MEDIA_AUDIO;
	UAMediaAttr	 MediaAttr;	
	int			 iAttr,j;
	char		 *ipaddr;

	if(pos>sdpSessGetMsessSize(_sdp))
		return _uamedia;
	else
		_uamedia=calloc(1,sizeof(struct uaMediaObj));

	if (!_uamedia) {
		UaCoreERR("[uaSDPGetMedia] memory alloc error!\n");
		return NULL;
	}

	/* set default value by session parameter if it present */
	if(sdpSessGetTc(_sdp,&pConnData) == RC_OK){
		ipaddr=strDup(pConnData.pConnAddr_);
		/* Do DNS query */
		if ( !ResolveName( &ipaddr ) ){
			UaCoreERR("[uaSDPGetDestAddress] Address resolution fail !\n");
			if(ipaddr) free(ipaddr);
			return _uamedia;
		}
		_uamedia->address=ipaddr;	
	}
	/* get parameter from sdp */
	/* media type */
	Media=sdpSessGetMsessAt(_sdp,pos);
	sdpMsessGetTm(Media,&MediaAnn);
	if(MediaAnn.Tm_type_ == SDP_TM_MEDIA_TYPE_VIDEO)
		_uamedia->type=UA_MEDIA_VIDEO;
	else if(MediaAnn.Tm_type_ == SDP_TM_MEDIA_TYPE_AUDIO)
		_uamedia->type=UA_MEDIA_AUDIO;
	else
		_uamedia->type=UA_MEDIA_UNKNOWN;	
	/* port*/
	_uamedia->port=MediaAnn.port_;
	/* address */
	_uamedia->numberofaddr=sdpMsessGetTcSize(Media);
	if(_uamedia->numberofaddr>0){
		sdpMsessGetTcAt(Media,0,&pConnData);
		ipaddr=strDup(pConnData.pConnAddr_);
		/* Do DNS query */
		if ( !ResolveName( &ipaddr ) ){
			UaCoreERR("[uaSDPGetDestAddress] Address resolution fail !\n");
			if(ipaddr) free(ipaddr);
			return _uamedia;
		}
		if (_uamedia->address) 
			free(_uamedia->address);
		_uamedia->address=ipaddr;
	}else
		_uamedia->address=NULL;
	/* bandwidth */
	sdpMsessGetTb(Media,&MediaBandwidth);
	_uamedia->bandwidth=MediaBandwidth.bandwidth_;

	/* pTime */
	/* status attribute ;default value is session attribute*/
	_uamedia->MediaAttr=uaSDPGetSessionAttr(_sdp);
	/* codec parameters */
	iAttr=sdpMsessGetTaSize(Media);	
	/* if there are parameter for codec */
	for(j=0;j<iAttr;j++){
		/* get Ta */
		sdpMsessGetTaAt(Media,j,&SDPMediaAttr);
		/*parse codec parameters*/
		if((strICmp("rtpmap",SDPMediaAttr.pAttrib_))==0){
			char *buf,*tmp,freeptr[256];
			UaCodec newCodec;
			
			sprintf(freeptr,"%s",SDPMediaAttr.pValue_);
			buf=strchr(freeptr,':');
			/*check if description exist*/
			if(buf != NULL)
				buf++;
			else{
				UaCoreERR("[uaSDPGetDestAddress]Error rtpmap format !\n");
				continue;
			}
			/* get codec type */
			tmp=strchr(buf,' ');
			if(tmp)
				*tmp='\0';
			else{
				UaCoreERR("[uaSDPGetDestAddress]Error rtpmap format !\n");
				continue;
			}
			newCodec=(UaCodec)calloc(1,sizeof(struct uaCodecObj));
			newCodec->codec=MediaCodectoNum(buf);
			buf=(char *)trimWS(tmp+1);
			tmp=strchr(buf,'/');
			/*find sample rate*/
			if(tmp!=NULL)
				*tmp='\0';

			newCodec->codename=strDup(buf);

			if(tmp!=NULL)
				buf=tmp+1;
			/*check if channel exist*/
			tmp=strchr(buf,'/');
			if(tmp!=NULL){
				*tmp++='\0';
				newCodec->channel=atoi(tmp);
			}
			newCodec->samplerate=atoi(buf);
			uaSDPMediaAddCodec(_uamedia,newCodec);	
			continue;
		}else if((strICmp("fmtp",SDPMediaAttr.pAttrib_))==0){
			uaSDPMediaAddFmtp(_uamedia,SDPMediaAttr.pValue_);
			continue;
		}else if((strICmp("ptime",SDPMediaAttr.pAttrib_))==0){
			_uamedia->pTime=atoi(SDPMediaAttr.pValue_+1);
			continue;
		/*get send/receive */
		}else if((MediaAttr=uaSdpGetMediaAttr(SDPMediaAttr.pAttrib_))!=UAMEDIA_UNKNOWN){
			_uamedia->MediaAttr=MediaAttr;
			continue;
		}
	}
	return _uamedia;
}

CCLAPI 
char* uaSDPGetSessionConnection(IN SdpSess _this)
{
	SdpTc pConnData;
	char *ipaddr=NULL; 

	if(!_this)
		return NULL;

	if(sdpSessGetTc(_this,&pConnData) == RC_OK){
		ipaddr=strDup(pConnData.pConnAddr_);
		/* Do DNS query */
		if ( !ResolveName( &ipaddr ) ){
			UaCoreERR("[uaSDPGetDestAddress] Address resolution fail !\n");
			if(ipaddr) free(ipaddr);
			return NULL;
		}
		ipaddr=(char*)pConnData.pConnAddr_;
		return ipaddr;
	}else
		return NULL;
}

CCLAPI 
UAMediaAttr uaSDPGetSessionAttr(IN SdpSess _this)
{

	SdpTa	SdpAttr;
	int		iAttr,j;
	UAMediaAttr	 SessAttr=UAMEDIA_SENDRECV;

	if (!_this) {
		/* error log */
		UaCoreWARN("[uaSDPGetSessionAttr] NULL SDP!\n");
		return UAMEDIA_UNKNOWN;
	}
	iAttr=sdpSessGetTaSize(_this);
	for(j=0;j<iAttr;j++){
		if(RC_OK==sdpSessGetTaAt(_this,j,&SdpAttr)){
			SessAttr=uaSdpGetMediaAttr(SdpAttr.pAttrib_);
			if (SessAttr!=UAMEDIA_UNKNOWN) 
				return SessAttr;
		}
	}
	
	return UAMEDIA_SENDRECV;
}