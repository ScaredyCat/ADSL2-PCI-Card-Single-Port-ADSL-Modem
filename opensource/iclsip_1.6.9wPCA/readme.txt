
-PCAUA
pre-work unzip the package.note:do not put the
source file in the "�ୱ" ,it wiil cause uacomtest
crash due to the illeage path by ����

1.build uacom
	-open sample\PCA.dsw
	-build uacom
2.build PCAUARes
	-open sample\PCA.dsw
	-build PCAUARes
3.build PCAUA
	-open sample\PCA.dsw
	-build PCAUA

ps:make sure project uacore has define _notautosend

�Y�n�sĶuacom�A�h�t�λݥ��w��Windows Platform SDK��Internet Development SDK�MDX9 sdk�A

�w�˧�����I��u�}�l�v�\��� -> �uMicrosoft Platform SDK February 2003�v
->�uVisual Studio Registration�v->�uRegister PSDK Directories with Visual Studio�v
�H�K�NPlatform SDK���U��Visual C++�C

�w�˫���ˬd�Avc->tools->options->directories����
1.include�O�_��"Microsoft SDK�ؿ�\include"
2.library�O�_��"Microsoft SDK�ؿ�\Lib"
3.executable file�O�_�� "Microsoft SDK�ؿ�\bin" and "Microsoft SDK�ؿ�\bin\winnt\"


-src�ؿ��]�t��

adt
	abstract data type,
	include:dx_buf,dx_hash,dx_lst,dx_msgq,dx_str,dx_vec.
common
	common type define,log system and some utilities
	include:cm_def,cm_trace,cm_utl
low
	platform dependent APIs,
	include:cx_event,cx_misc,cx_mutex,cx_sock,cx_thrd,cx_time
make
	default make setting for all projects
rtp
	rtp library
sdp
	sdp library
SIMPLEAPI
	SIMPLE APIs,
	include:simple_api,simple_dlg,simple_msg
sip
	sip stack parse,and udp/tcp/tls transport,
	include:base64,cclsip,md5,md5_g,rfc822,sip,sip_body,sip_cfg
		,sip_cm,sip_hdr,sip_req,sip_rsp,sip_tx,sip_url
sipTx
	sip transaction layer,
	include:CTransactionDatabase,sipTx,StateMachine,Transport,TxStruct
	
UaCore
	CCL user agent core,
	include:ua_cfg,ua_class,ua_cm,ua_content,ua_core,ua_dlg,ua_evtpkg
		,ua_int,ua_mgr,ua_msg,ua_sdp,ua_sipmsg,ua_sub,ua_user
