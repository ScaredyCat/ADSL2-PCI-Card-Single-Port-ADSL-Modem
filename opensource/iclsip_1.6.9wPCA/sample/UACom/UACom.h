/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Mon Jan 22 11:26:14 2007
 */
/* Compiler settings for D:\ITRI\ReleasePack\sip\sample\UACom\UACom.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __UACom_h__
#define __UACom_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IUAControl_FWD_DEFINED__
#define __IUAControl_FWD_DEFINED__
typedef interface IUAControl IUAControl;
#endif 	/* __IUAControl_FWD_DEFINED__ */


#ifndef ___IUAControlEvents_FWD_DEFINED__
#define ___IUAControlEvents_FWD_DEFINED__
typedef interface _IUAControlEvents _IUAControlEvents;
#endif 	/* ___IUAControlEvents_FWD_DEFINED__ */


#ifndef __UAControl_FWD_DEFINED__
#define __UAControl_FWD_DEFINED__

#ifdef __cplusplus
typedef class UAControl UAControl;
#else
typedef struct UAControl UAControl;
#endif /* __cplusplus */

#endif 	/* __UAControl_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IUAControl_INTERFACE_DEFINED__
#define __IUAControl_INTERFACE_DEFINED__

/* interface IUAControl */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IUAControl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4A91A134-1B60-497E-87EE-23C97B2A6D84")
    IUAControl : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ BSTR realm,
            /* [in] */ BSTR userid,
            /* [in] */ BSTR password) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CancelCall( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DropCall( 
            /* [in] */ int dlgHandle) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AnswerCall( 
            /* [in] */ int dlgHandle) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RejectCall( 
            /* [in] */ int dlgHandle) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE MakeCall( 
            /* [in] */ BSTR dialURL,
            /* [retval][out] */ int __RPC_FAR *retval) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ShowPreferences( 
            /* [retval][out] */ BOOL __RPC_FAR *pChanged) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Test( 
            /* [in] */ BSTR tmpStr) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE HoldCall( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE UnHoldCall( 
            /* [in] */ int dlgHandle) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE UnAttendXfer( 
            /* [in] */ int dlgHandle,
            /* [in] */ BSTR xferURL) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AttendXfer( 
            /* [in] */ int dlgHandle1,
            /* [in] */ int dlgHandle2) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetRemoteParty( 
            /* [in] */ int dlgHandle,
            /* [retval][out] */ BSTR __RPC_FAR *retval) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE StartLocalVideo( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE StopLocalVideo( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Register( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE UnRegister( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RegQuery( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SwitchLocalVideo( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Stop( void) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_LocalIP( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_LocalIP( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SendDTMF0( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SendDTMF1( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SendDTMF2( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SendDTMF3( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SendDTMF4( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SendDTMF5( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SendDTMF6( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SendDTMF7( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SendDTMF8( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SendDTMF9( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SendDTMFP( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SendDTMFS( void) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_bSingleCall( 
            /* [retval][out] */ BOOL __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_bSingleCall( 
            /* [in] */ BOOL newVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetAudioType( 
            /* [retval][out] */ int __RPC_FAR *retval) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetVideoType( 
            /* [retval][out] */ int __RPC_FAR *retval) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetVideoSize( 
            /* [retval][out] */ int __RPC_FAR *retval) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetRemotePartyDisplayName( 
            /* [in] */ int dlgHandle,
            /* [retval][out] */ BSTR __RPC_FAR *retval) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetMediaReceiveQuality( 
            /* [retval][out] */ int __RPC_FAR *retval) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE StartRemoteVideo( 
            /* [in] */ int x,
            /* [in] */ int y,
            /* [in] */ int scale) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE StopRemoteVideo( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE StartLocalVideoEx( 
            /* [in] */ int nhwnd,
            /* [in] */ int x,
            /* [in] */ int y,
            /* [in] */ int scale) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE StartRemoteVideoEx( 
            /* [in] */ int nhwnd,
            /* [in] */ int x,
            /* [in] */ int y,
            /* [in] */ int scale) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SendText( 
            /* [in] */ BSTR dialURL,
            /* [in] */ BSTR tmpstr) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AddBuddy( 
            /* [in] */ BSTR buddyURI) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AuthorizeBuddy( 
            /* [in] */ BSTR buddyURI) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE UnauthorizeBuddy( 
            /* [in] */ BSTR buddyURI) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DelBuddy( 
            /* [in] */ BSTR buddyURI) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE BlockBuddy( 
            /* [in] */ BSTR buddyURI) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE UnBlockBuddy( 
            /* [in] */ BSTR buddyURI) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetBuddyCount( 
            /* [retval][out] */ int __RPC_FAR *retval) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetBuddyURI( 
            /* [in] */ int number,
            /* [retval][out] */ BSTR __RPC_FAR *retval) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE UpdateBasicStatus( 
            /* [in] */ BOOL bOpen) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetBuddyINSubState( 
            /* [in] */ BSTR buddyURI,
            /* [retval][out] */ int __RPC_FAR *SubState) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetBuddyOUTSubState( 
            /* [in] */ BSTR buddyURI,
            /* [retval][out] */ int __RPC_FAR *SubState) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetBuddyBasicStatus( 
            /* [in] */ BSTR buddyURI,
            /* [retval][out] */ int __RPC_FAR *BasicStatus) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SubscribeBuddy( 
            /* [in] */ BSTR buddyURI) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE UnsubscribeBuddy( 
            /* [in] */ BSTR buddyURI) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IsBuddyBlock( 
            /* [in] */ BSTR buddyURI,
            /* [retval][out] */ BOOL __RPC_FAR *retval) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IsBuddyAuthorized( 
            /* [in] */ BSTR buddyURI,
            /* [retval][out] */ BOOL __RPC_FAR *retval) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SendInfo( 
            /* [in] */ BSTR bstrURL,
            /* [in] */ BSTR bstrContentType,
            /* [in] */ BSTR bstrBody) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SubscribeWinfo( 
            /* [in] */ BSTR buddyURI) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SubscribePres( 
            /* [in] */ BSTR buddyURI) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SubscribeResLst( 
            /* [in] */ BSTR ResourceURI) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE UnsubscribeWinfo( 
            /* [in] */ BSTR buddyURI) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE UnsubscribePres( 
            /* [in] */ BSTR buddyURI) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE UnsubscribeResLst( 
            /* [in] */ BSTR ResourceURI) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE PublishStatus( 
            /* [in] */ BSTR szURI,
            /* [in] */ BOOL bStatus) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IUAControlVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IUAControl __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IUAControl __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IUAControl __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IUAControl __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IUAControl __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IUAControl __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IUAControl __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Initialize )( 
            IUAControl __RPC_FAR * This,
            /* [in] */ BSTR realm,
            /* [in] */ BSTR userid,
            /* [in] */ BSTR password);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CancelCall )( 
            IUAControl __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DropCall )( 
            IUAControl __RPC_FAR * This,
            /* [in] */ int dlgHandle);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AnswerCall )( 
            IUAControl __RPC_FAR * This,
            /* [in] */ int dlgHandle);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RejectCall )( 
            IUAControl __RPC_FAR * This,
            /* [in] */ int dlgHandle);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MakeCall )( 
            IUAControl __RPC_FAR * This,
            /* [in] */ BSTR dialURL,
            /* [retval][out] */ int __RPC_FAR *retval);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ShowPreferences )( 
            IUAControl __RPC_FAR * This,
            /* [retval][out] */ BOOL __RPC_FAR *pChanged);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Test )( 
            IUAControl __RPC_FAR * This,
            /* [in] */ BSTR tmpStr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *HoldCall )( 
            IUAControl __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UnHoldCall )( 
            IUAControl __RPC_FAR * This,
            /* [in] */ int dlgHandle);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UnAttendXfer )( 
            IUAControl __RPC_FAR * This,
            /* [in] */ int dlgHandle,
            /* [in] */ BSTR xferURL);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AttendXfer )( 
            IUAControl __RPC_FAR * This,
            /* [in] */ int dlgHandle1,
            /* [in] */ int dlgHandle2);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRemoteParty )( 
            IUAControl __RPC_FAR * This,
            /* [in] */ int dlgHandle,
            /* [retval][out] */ BSTR __RPC_FAR *retval);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *StartLocalVideo )( 
            IUAControl __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *StopLocalVideo )( 
            IUAControl __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Register )( 
            IUAControl __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UnRegister )( 
            IUAControl __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RegQuery )( 
            IUAControl __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SwitchLocalVideo )( 
            IUAControl __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Stop )( 
            IUAControl __RPC_FAR * This);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_LocalIP )( 
            IUAControl __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_LocalIP )( 
            IUAControl __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SendDTMF0 )( 
            IUAControl __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SendDTMF1 )( 
            IUAControl __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SendDTMF2 )( 
            IUAControl __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SendDTMF3 )( 
            IUAControl __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SendDTMF4 )( 
            IUAControl __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SendDTMF5 )( 
            IUAControl __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SendDTMF6 )( 
            IUAControl __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SendDTMF7 )( 
            IUAControl __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SendDTMF8 )( 
            IUAControl __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SendDTMF9 )( 
            IUAControl __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SendDTMFP )( 
            IUAControl __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SendDTMFS )( 
            IUAControl __RPC_FAR * This);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_bSingleCall )( 
            IUAControl __RPC_FAR * This,
            /* [retval][out] */ BOOL __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_bSingleCall )( 
            IUAControl __RPC_FAR * This,
            /* [in] */ BOOL newVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IUAControl __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetAudioType )( 
            IUAControl __RPC_FAR * This,
            /* [retval][out] */ int __RPC_FAR *retval);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetVideoType )( 
            IUAControl __RPC_FAR * This,
            /* [retval][out] */ int __RPC_FAR *retval);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetVideoSize )( 
            IUAControl __RPC_FAR * This,
            /* [retval][out] */ int __RPC_FAR *retval);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRemotePartyDisplayName )( 
            IUAControl __RPC_FAR * This,
            /* [in] */ int dlgHandle,
            /* [retval][out] */ BSTR __RPC_FAR *retval);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMediaReceiveQuality )( 
            IUAControl __RPC_FAR * This,
            /* [retval][out] */ int __RPC_FAR *retval);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *StartRemoteVideo )( 
            IUAControl __RPC_FAR * This,
            /* [in] */ int x,
            /* [in] */ int y,
            /* [in] */ int scale);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *StopRemoteVideo )( 
            IUAControl __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *StartLocalVideoEx )( 
            IUAControl __RPC_FAR * This,
            /* [in] */ int nhwnd,
            /* [in] */ int x,
            /* [in] */ int y,
            /* [in] */ int scale);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *StartRemoteVideoEx )( 
            IUAControl __RPC_FAR * This,
            /* [in] */ int nhwnd,
            /* [in] */ int x,
            /* [in] */ int y,
            /* [in] */ int scale);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SendText )( 
            IUAControl __RPC_FAR * This,
            /* [in] */ BSTR dialURL,
            /* [in] */ BSTR tmpstr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddBuddy )( 
            IUAControl __RPC_FAR * This,
            /* [in] */ BSTR buddyURI);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AuthorizeBuddy )( 
            IUAControl __RPC_FAR * This,
            /* [in] */ BSTR buddyURI);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UnauthorizeBuddy )( 
            IUAControl __RPC_FAR * This,
            /* [in] */ BSTR buddyURI);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DelBuddy )( 
            IUAControl __RPC_FAR * This,
            /* [in] */ BSTR buddyURI);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *BlockBuddy )( 
            IUAControl __RPC_FAR * This,
            /* [in] */ BSTR buddyURI);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UnBlockBuddy )( 
            IUAControl __RPC_FAR * This,
            /* [in] */ BSTR buddyURI);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetBuddyCount )( 
            IUAControl __RPC_FAR * This,
            /* [retval][out] */ int __RPC_FAR *retval);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetBuddyURI )( 
            IUAControl __RPC_FAR * This,
            /* [in] */ int number,
            /* [retval][out] */ BSTR __RPC_FAR *retval);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UpdateBasicStatus )( 
            IUAControl __RPC_FAR * This,
            /* [in] */ BOOL bOpen);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetBuddyINSubState )( 
            IUAControl __RPC_FAR * This,
            /* [in] */ BSTR buddyURI,
            /* [retval][out] */ int __RPC_FAR *SubState);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetBuddyOUTSubState )( 
            IUAControl __RPC_FAR * This,
            /* [in] */ BSTR buddyURI,
            /* [retval][out] */ int __RPC_FAR *SubState);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetBuddyBasicStatus )( 
            IUAControl __RPC_FAR * This,
            /* [in] */ BSTR buddyURI,
            /* [retval][out] */ int __RPC_FAR *BasicStatus);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SubscribeBuddy )( 
            IUAControl __RPC_FAR * This,
            /* [in] */ BSTR buddyURI);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UnsubscribeBuddy )( 
            IUAControl __RPC_FAR * This,
            /* [in] */ BSTR buddyURI);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsBuddyBlock )( 
            IUAControl __RPC_FAR * This,
            /* [in] */ BSTR buddyURI,
            /* [retval][out] */ BOOL __RPC_FAR *retval);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsBuddyAuthorized )( 
            IUAControl __RPC_FAR * This,
            /* [in] */ BSTR buddyURI,
            /* [retval][out] */ BOOL __RPC_FAR *retval);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SendInfo )( 
            IUAControl __RPC_FAR * This,
            /* [in] */ BSTR bstrURL,
            /* [in] */ BSTR bstrContentType,
            /* [in] */ BSTR bstrBody);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SubscribeWinfo )( 
            IUAControl __RPC_FAR * This,
            /* [in] */ BSTR buddyURI);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SubscribePres )( 
            IUAControl __RPC_FAR * This,
            /* [in] */ BSTR buddyURI);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SubscribeResLst )( 
            IUAControl __RPC_FAR * This,
            /* [in] */ BSTR ResourceURI);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UnsubscribeWinfo )( 
            IUAControl __RPC_FAR * This,
            /* [in] */ BSTR buddyURI);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UnsubscribePres )( 
            IUAControl __RPC_FAR * This,
            /* [in] */ BSTR buddyURI);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UnsubscribeResLst )( 
            IUAControl __RPC_FAR * This,
            /* [in] */ BSTR ResourceURI);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *PublishStatus )( 
            IUAControl __RPC_FAR * This,
            /* [in] */ BSTR szURI,
            /* [in] */ BOOL bStatus);
        
        END_INTERFACE
    } IUAControlVtbl;

    interface IUAControl
    {
        CONST_VTBL struct IUAControlVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IUAControl_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IUAControl_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IUAControl_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IUAControl_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IUAControl_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IUAControl_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IUAControl_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IUAControl_Initialize(This,realm,userid,password)	\
    (This)->lpVtbl -> Initialize(This,realm,userid,password)

#define IUAControl_CancelCall(This)	\
    (This)->lpVtbl -> CancelCall(This)

#define IUAControl_DropCall(This,dlgHandle)	\
    (This)->lpVtbl -> DropCall(This,dlgHandle)

#define IUAControl_AnswerCall(This,dlgHandle)	\
    (This)->lpVtbl -> AnswerCall(This,dlgHandle)

#define IUAControl_RejectCall(This,dlgHandle)	\
    (This)->lpVtbl -> RejectCall(This,dlgHandle)

#define IUAControl_MakeCall(This,dialURL,retval)	\
    (This)->lpVtbl -> MakeCall(This,dialURL,retval)

#define IUAControl_ShowPreferences(This,pChanged)	\
    (This)->lpVtbl -> ShowPreferences(This,pChanged)

#define IUAControl_Test(This,tmpStr)	\
    (This)->lpVtbl -> Test(This,tmpStr)

#define IUAControl_HoldCall(This)	\
    (This)->lpVtbl -> HoldCall(This)

#define IUAControl_UnHoldCall(This,dlgHandle)	\
    (This)->lpVtbl -> UnHoldCall(This,dlgHandle)

#define IUAControl_UnAttendXfer(This,dlgHandle,xferURL)	\
    (This)->lpVtbl -> UnAttendXfer(This,dlgHandle,xferURL)

#define IUAControl_AttendXfer(This,dlgHandle1,dlgHandle2)	\
    (This)->lpVtbl -> AttendXfer(This,dlgHandle1,dlgHandle2)

#define IUAControl_GetRemoteParty(This,dlgHandle,retval)	\
    (This)->lpVtbl -> GetRemoteParty(This,dlgHandle,retval)

#define IUAControl_StartLocalVideo(This)	\
    (This)->lpVtbl -> StartLocalVideo(This)

#define IUAControl_StopLocalVideo(This)	\
    (This)->lpVtbl -> StopLocalVideo(This)

#define IUAControl_Register(This)	\
    (This)->lpVtbl -> Register(This)

#define IUAControl_UnRegister(This)	\
    (This)->lpVtbl -> UnRegister(This)

#define IUAControl_RegQuery(This)	\
    (This)->lpVtbl -> RegQuery(This)

#define IUAControl_SwitchLocalVideo(This)	\
    (This)->lpVtbl -> SwitchLocalVideo(This)

#define IUAControl_Stop(This)	\
    (This)->lpVtbl -> Stop(This)

#define IUAControl_get_LocalIP(This,pVal)	\
    (This)->lpVtbl -> get_LocalIP(This,pVal)

#define IUAControl_put_LocalIP(This,newVal)	\
    (This)->lpVtbl -> put_LocalIP(This,newVal)

#define IUAControl_SendDTMF0(This)	\
    (This)->lpVtbl -> SendDTMF0(This)

#define IUAControl_SendDTMF1(This)	\
    (This)->lpVtbl -> SendDTMF1(This)

#define IUAControl_SendDTMF2(This)	\
    (This)->lpVtbl -> SendDTMF2(This)

#define IUAControl_SendDTMF3(This)	\
    (This)->lpVtbl -> SendDTMF3(This)

#define IUAControl_SendDTMF4(This)	\
    (This)->lpVtbl -> SendDTMF4(This)

#define IUAControl_SendDTMF5(This)	\
    (This)->lpVtbl -> SendDTMF5(This)

#define IUAControl_SendDTMF6(This)	\
    (This)->lpVtbl -> SendDTMF6(This)

#define IUAControl_SendDTMF7(This)	\
    (This)->lpVtbl -> SendDTMF7(This)

#define IUAControl_SendDTMF8(This)	\
    (This)->lpVtbl -> SendDTMF8(This)

#define IUAControl_SendDTMF9(This)	\
    (This)->lpVtbl -> SendDTMF9(This)

#define IUAControl_SendDTMFP(This)	\
    (This)->lpVtbl -> SendDTMFP(This)

#define IUAControl_SendDTMFS(This)	\
    (This)->lpVtbl -> SendDTMFS(This)

#define IUAControl_get_bSingleCall(This,pVal)	\
    (This)->lpVtbl -> get_bSingleCall(This,pVal)

#define IUAControl_put_bSingleCall(This,newVal)	\
    (This)->lpVtbl -> put_bSingleCall(This,newVal)

#define IUAControl_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IUAControl_GetAudioType(This,retval)	\
    (This)->lpVtbl -> GetAudioType(This,retval)

#define IUAControl_GetVideoType(This,retval)	\
    (This)->lpVtbl -> GetVideoType(This,retval)

#define IUAControl_GetVideoSize(This,retval)	\
    (This)->lpVtbl -> GetVideoSize(This,retval)

#define IUAControl_GetRemotePartyDisplayName(This,dlgHandle,retval)	\
    (This)->lpVtbl -> GetRemotePartyDisplayName(This,dlgHandle,retval)

#define IUAControl_GetMediaReceiveQuality(This,retval)	\
    (This)->lpVtbl -> GetMediaReceiveQuality(This,retval)

#define IUAControl_StartRemoteVideo(This,x,y,scale)	\
    (This)->lpVtbl -> StartRemoteVideo(This,x,y,scale)

#define IUAControl_StopRemoteVideo(This)	\
    (This)->lpVtbl -> StopRemoteVideo(This)

#define IUAControl_StartLocalVideoEx(This,nhwnd,x,y,scale)	\
    (This)->lpVtbl -> StartLocalVideoEx(This,nhwnd,x,y,scale)

#define IUAControl_StartRemoteVideoEx(This,nhwnd,x,y,scale)	\
    (This)->lpVtbl -> StartRemoteVideoEx(This,nhwnd,x,y,scale)

#define IUAControl_SendText(This,dialURL,tmpstr)	\
    (This)->lpVtbl -> SendText(This,dialURL,tmpstr)

#define IUAControl_AddBuddy(This,buddyURI)	\
    (This)->lpVtbl -> AddBuddy(This,buddyURI)

#define IUAControl_AuthorizeBuddy(This,buddyURI)	\
    (This)->lpVtbl -> AuthorizeBuddy(This,buddyURI)

#define IUAControl_UnauthorizeBuddy(This,buddyURI)	\
    (This)->lpVtbl -> UnauthorizeBuddy(This,buddyURI)

#define IUAControl_DelBuddy(This,buddyURI)	\
    (This)->lpVtbl -> DelBuddy(This,buddyURI)

#define IUAControl_BlockBuddy(This,buddyURI)	\
    (This)->lpVtbl -> BlockBuddy(This,buddyURI)

#define IUAControl_UnBlockBuddy(This,buddyURI)	\
    (This)->lpVtbl -> UnBlockBuddy(This,buddyURI)

#define IUAControl_GetBuddyCount(This,retval)	\
    (This)->lpVtbl -> GetBuddyCount(This,retval)

#define IUAControl_GetBuddyURI(This,number,retval)	\
    (This)->lpVtbl -> GetBuddyURI(This,number,retval)

#define IUAControl_UpdateBasicStatus(This,bOpen)	\
    (This)->lpVtbl -> UpdateBasicStatus(This,bOpen)

#define IUAControl_GetBuddyINSubState(This,buddyURI,SubState)	\
    (This)->lpVtbl -> GetBuddyINSubState(This,buddyURI,SubState)

#define IUAControl_GetBuddyOUTSubState(This,buddyURI,SubState)	\
    (This)->lpVtbl -> GetBuddyOUTSubState(This,buddyURI,SubState)

#define IUAControl_GetBuddyBasicStatus(This,buddyURI,BasicStatus)	\
    (This)->lpVtbl -> GetBuddyBasicStatus(This,buddyURI,BasicStatus)

#define IUAControl_SubscribeBuddy(This,buddyURI)	\
    (This)->lpVtbl -> SubscribeBuddy(This,buddyURI)

#define IUAControl_UnsubscribeBuddy(This,buddyURI)	\
    (This)->lpVtbl -> UnsubscribeBuddy(This,buddyURI)

#define IUAControl_IsBuddyBlock(This,buddyURI,retval)	\
    (This)->lpVtbl -> IsBuddyBlock(This,buddyURI,retval)

#define IUAControl_IsBuddyAuthorized(This,buddyURI,retval)	\
    (This)->lpVtbl -> IsBuddyAuthorized(This,buddyURI,retval)

#define IUAControl_SendInfo(This,bstrURL,bstrContentType,bstrBody)	\
    (This)->lpVtbl -> SendInfo(This,bstrURL,bstrContentType,bstrBody)

#define IUAControl_SubscribeWinfo(This,buddyURI)	\
    (This)->lpVtbl -> SubscribeWinfo(This,buddyURI)

#define IUAControl_SubscribePres(This,buddyURI)	\
    (This)->lpVtbl -> SubscribePres(This,buddyURI)

#define IUAControl_SubscribeResLst(This,ResourceURI)	\
    (This)->lpVtbl -> SubscribeResLst(This,ResourceURI)

#define IUAControl_UnsubscribeWinfo(This,buddyURI)	\
    (This)->lpVtbl -> UnsubscribeWinfo(This,buddyURI)

#define IUAControl_UnsubscribePres(This,buddyURI)	\
    (This)->lpVtbl -> UnsubscribePres(This,buddyURI)

#define IUAControl_UnsubscribeResLst(This,ResourceURI)	\
    (This)->lpVtbl -> UnsubscribeResLst(This,ResourceURI)

#define IUAControl_PublishStatus(This,szURI,bStatus)	\
    (This)->lpVtbl -> PublishStatus(This,szURI,bStatus)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_Initialize_Proxy( 
    IUAControl __RPC_FAR * This,
    /* [in] */ BSTR realm,
    /* [in] */ BSTR userid,
    /* [in] */ BSTR password);


void __RPC_STUB IUAControl_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_CancelCall_Proxy( 
    IUAControl __RPC_FAR * This);


void __RPC_STUB IUAControl_CancelCall_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_DropCall_Proxy( 
    IUAControl __RPC_FAR * This,
    /* [in] */ int dlgHandle);


void __RPC_STUB IUAControl_DropCall_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_AnswerCall_Proxy( 
    IUAControl __RPC_FAR * This,
    /* [in] */ int dlgHandle);


void __RPC_STUB IUAControl_AnswerCall_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_RejectCall_Proxy( 
    IUAControl __RPC_FAR * This,
    /* [in] */ int dlgHandle);


void __RPC_STUB IUAControl_RejectCall_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_MakeCall_Proxy( 
    IUAControl __RPC_FAR * This,
    /* [in] */ BSTR dialURL,
    /* [retval][out] */ int __RPC_FAR *retval);


void __RPC_STUB IUAControl_MakeCall_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_ShowPreferences_Proxy( 
    IUAControl __RPC_FAR * This,
    /* [retval][out] */ BOOL __RPC_FAR *pChanged);


void __RPC_STUB IUAControl_ShowPreferences_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_Test_Proxy( 
    IUAControl __RPC_FAR * This,
    /* [in] */ BSTR tmpStr);


void __RPC_STUB IUAControl_Test_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_HoldCall_Proxy( 
    IUAControl __RPC_FAR * This);


void __RPC_STUB IUAControl_HoldCall_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_UnHoldCall_Proxy( 
    IUAControl __RPC_FAR * This,
    /* [in] */ int dlgHandle);


void __RPC_STUB IUAControl_UnHoldCall_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_UnAttendXfer_Proxy( 
    IUAControl __RPC_FAR * This,
    /* [in] */ int dlgHandle,
    /* [in] */ BSTR xferURL);


void __RPC_STUB IUAControl_UnAttendXfer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_AttendXfer_Proxy( 
    IUAControl __RPC_FAR * This,
    /* [in] */ int dlgHandle1,
    /* [in] */ int dlgHandle2);


void __RPC_STUB IUAControl_AttendXfer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_GetRemoteParty_Proxy( 
    IUAControl __RPC_FAR * This,
    /* [in] */ int dlgHandle,
    /* [retval][out] */ BSTR __RPC_FAR *retval);


void __RPC_STUB IUAControl_GetRemoteParty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_StartLocalVideo_Proxy( 
    IUAControl __RPC_FAR * This);


void __RPC_STUB IUAControl_StartLocalVideo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_StopLocalVideo_Proxy( 
    IUAControl __RPC_FAR * This);


void __RPC_STUB IUAControl_StopLocalVideo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_Register_Proxy( 
    IUAControl __RPC_FAR * This);


void __RPC_STUB IUAControl_Register_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_UnRegister_Proxy( 
    IUAControl __RPC_FAR * This);


void __RPC_STUB IUAControl_UnRegister_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_RegQuery_Proxy( 
    IUAControl __RPC_FAR * This);


void __RPC_STUB IUAControl_RegQuery_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_SwitchLocalVideo_Proxy( 
    IUAControl __RPC_FAR * This);


void __RPC_STUB IUAControl_SwitchLocalVideo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_Stop_Proxy( 
    IUAControl __RPC_FAR * This);


void __RPC_STUB IUAControl_Stop_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IUAControl_get_LocalIP_Proxy( 
    IUAControl __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IUAControl_get_LocalIP_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IUAControl_put_LocalIP_Proxy( 
    IUAControl __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IUAControl_put_LocalIP_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_SendDTMF0_Proxy( 
    IUAControl __RPC_FAR * This);


void __RPC_STUB IUAControl_SendDTMF0_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_SendDTMF1_Proxy( 
    IUAControl __RPC_FAR * This);


void __RPC_STUB IUAControl_SendDTMF1_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_SendDTMF2_Proxy( 
    IUAControl __RPC_FAR * This);


void __RPC_STUB IUAControl_SendDTMF2_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_SendDTMF3_Proxy( 
    IUAControl __RPC_FAR * This);


void __RPC_STUB IUAControl_SendDTMF3_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_SendDTMF4_Proxy( 
    IUAControl __RPC_FAR * This);


void __RPC_STUB IUAControl_SendDTMF4_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_SendDTMF5_Proxy( 
    IUAControl __RPC_FAR * This);


void __RPC_STUB IUAControl_SendDTMF5_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_SendDTMF6_Proxy( 
    IUAControl __RPC_FAR * This);


void __RPC_STUB IUAControl_SendDTMF6_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_SendDTMF7_Proxy( 
    IUAControl __RPC_FAR * This);


void __RPC_STUB IUAControl_SendDTMF7_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_SendDTMF8_Proxy( 
    IUAControl __RPC_FAR * This);


void __RPC_STUB IUAControl_SendDTMF8_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_SendDTMF9_Proxy( 
    IUAControl __RPC_FAR * This);


void __RPC_STUB IUAControl_SendDTMF9_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_SendDTMFP_Proxy( 
    IUAControl __RPC_FAR * This);


void __RPC_STUB IUAControl_SendDTMFP_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_SendDTMFS_Proxy( 
    IUAControl __RPC_FAR * This);


void __RPC_STUB IUAControl_SendDTMFS_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IUAControl_get_bSingleCall_Proxy( 
    IUAControl __RPC_FAR * This,
    /* [retval][out] */ BOOL __RPC_FAR *pVal);


void __RPC_STUB IUAControl_get_bSingleCall_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IUAControl_put_bSingleCall_Proxy( 
    IUAControl __RPC_FAR * This,
    /* [in] */ BOOL newVal);


void __RPC_STUB IUAControl_put_bSingleCall_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_Reset_Proxy( 
    IUAControl __RPC_FAR * This);


void __RPC_STUB IUAControl_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_GetAudioType_Proxy( 
    IUAControl __RPC_FAR * This,
    /* [retval][out] */ int __RPC_FAR *retval);


void __RPC_STUB IUAControl_GetAudioType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_GetVideoType_Proxy( 
    IUAControl __RPC_FAR * This,
    /* [retval][out] */ int __RPC_FAR *retval);


void __RPC_STUB IUAControl_GetVideoType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_GetVideoSize_Proxy( 
    IUAControl __RPC_FAR * This,
    /* [retval][out] */ int __RPC_FAR *retval);


void __RPC_STUB IUAControl_GetVideoSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_GetRemotePartyDisplayName_Proxy( 
    IUAControl __RPC_FAR * This,
    /* [in] */ int dlgHandle,
    /* [retval][out] */ BSTR __RPC_FAR *retval);


void __RPC_STUB IUAControl_GetRemotePartyDisplayName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_GetMediaReceiveQuality_Proxy( 
    IUAControl __RPC_FAR * This,
    /* [retval][out] */ int __RPC_FAR *retval);


void __RPC_STUB IUAControl_GetMediaReceiveQuality_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_StartRemoteVideo_Proxy( 
    IUAControl __RPC_FAR * This,
    /* [in] */ int x,
    /* [in] */ int y,
    /* [in] */ int scale);


void __RPC_STUB IUAControl_StartRemoteVideo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_StopRemoteVideo_Proxy( 
    IUAControl __RPC_FAR * This);


void __RPC_STUB IUAControl_StopRemoteVideo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_StartLocalVideoEx_Proxy( 
    IUAControl __RPC_FAR * This,
    /* [in] */ int nhwnd,
    /* [in] */ int x,
    /* [in] */ int y,
    /* [in] */ int scale);


void __RPC_STUB IUAControl_StartLocalVideoEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_StartRemoteVideoEx_Proxy( 
    IUAControl __RPC_FAR * This,
    /* [in] */ int nhwnd,
    /* [in] */ int x,
    /* [in] */ int y,
    /* [in] */ int scale);


void __RPC_STUB IUAControl_StartRemoteVideoEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_SendText_Proxy( 
    IUAControl __RPC_FAR * This,
    /* [in] */ BSTR dialURL,
    /* [in] */ BSTR tmpstr);


void __RPC_STUB IUAControl_SendText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_AddBuddy_Proxy( 
    IUAControl __RPC_FAR * This,
    /* [in] */ BSTR buddyURI);


void __RPC_STUB IUAControl_AddBuddy_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_AuthorizeBuddy_Proxy( 
    IUAControl __RPC_FAR * This,
    /* [in] */ BSTR buddyURI);


void __RPC_STUB IUAControl_AuthorizeBuddy_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_UnauthorizeBuddy_Proxy( 
    IUAControl __RPC_FAR * This,
    /* [in] */ BSTR buddyURI);


void __RPC_STUB IUAControl_UnauthorizeBuddy_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_DelBuddy_Proxy( 
    IUAControl __RPC_FAR * This,
    /* [in] */ BSTR buddyURI);


void __RPC_STUB IUAControl_DelBuddy_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_BlockBuddy_Proxy( 
    IUAControl __RPC_FAR * This,
    /* [in] */ BSTR buddyURI);


void __RPC_STUB IUAControl_BlockBuddy_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_UnBlockBuddy_Proxy( 
    IUAControl __RPC_FAR * This,
    /* [in] */ BSTR buddyURI);


void __RPC_STUB IUAControl_UnBlockBuddy_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_GetBuddyCount_Proxy( 
    IUAControl __RPC_FAR * This,
    /* [retval][out] */ int __RPC_FAR *retval);


void __RPC_STUB IUAControl_GetBuddyCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_GetBuddyURI_Proxy( 
    IUAControl __RPC_FAR * This,
    /* [in] */ int number,
    /* [retval][out] */ BSTR __RPC_FAR *retval);


void __RPC_STUB IUAControl_GetBuddyURI_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_UpdateBasicStatus_Proxy( 
    IUAControl __RPC_FAR * This,
    /* [in] */ BOOL bOpen);


void __RPC_STUB IUAControl_UpdateBasicStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_GetBuddyINSubState_Proxy( 
    IUAControl __RPC_FAR * This,
    /* [in] */ BSTR buddyURI,
    /* [retval][out] */ int __RPC_FAR *SubState);


void __RPC_STUB IUAControl_GetBuddyINSubState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_GetBuddyOUTSubState_Proxy( 
    IUAControl __RPC_FAR * This,
    /* [in] */ BSTR buddyURI,
    /* [retval][out] */ int __RPC_FAR *SubState);


void __RPC_STUB IUAControl_GetBuddyOUTSubState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_GetBuddyBasicStatus_Proxy( 
    IUAControl __RPC_FAR * This,
    /* [in] */ BSTR buddyURI,
    /* [retval][out] */ int __RPC_FAR *BasicStatus);


void __RPC_STUB IUAControl_GetBuddyBasicStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_SubscribeBuddy_Proxy( 
    IUAControl __RPC_FAR * This,
    /* [in] */ BSTR buddyURI);


void __RPC_STUB IUAControl_SubscribeBuddy_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_UnsubscribeBuddy_Proxy( 
    IUAControl __RPC_FAR * This,
    /* [in] */ BSTR buddyURI);


void __RPC_STUB IUAControl_UnsubscribeBuddy_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_IsBuddyBlock_Proxy( 
    IUAControl __RPC_FAR * This,
    /* [in] */ BSTR buddyURI,
    /* [retval][out] */ BOOL __RPC_FAR *retval);


void __RPC_STUB IUAControl_IsBuddyBlock_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_IsBuddyAuthorized_Proxy( 
    IUAControl __RPC_FAR * This,
    /* [in] */ BSTR buddyURI,
    /* [retval][out] */ BOOL __RPC_FAR *retval);


void __RPC_STUB IUAControl_IsBuddyAuthorized_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_SendInfo_Proxy( 
    IUAControl __RPC_FAR * This,
    /* [in] */ BSTR bstrURL,
    /* [in] */ BSTR bstrContentType,
    /* [in] */ BSTR bstrBody);


void __RPC_STUB IUAControl_SendInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_SubscribeWinfo_Proxy( 
    IUAControl __RPC_FAR * This,
    /* [in] */ BSTR buddyURI);


void __RPC_STUB IUAControl_SubscribeWinfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_SubscribePres_Proxy( 
    IUAControl __RPC_FAR * This,
    /* [in] */ BSTR buddyURI);


void __RPC_STUB IUAControl_SubscribePres_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_SubscribeResLst_Proxy( 
    IUAControl __RPC_FAR * This,
    /* [in] */ BSTR ResourceURI);


void __RPC_STUB IUAControl_SubscribeResLst_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_UnsubscribeWinfo_Proxy( 
    IUAControl __RPC_FAR * This,
    /* [in] */ BSTR buddyURI);


void __RPC_STUB IUAControl_UnsubscribeWinfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_UnsubscribePres_Proxy( 
    IUAControl __RPC_FAR * This,
    /* [in] */ BSTR buddyURI);


void __RPC_STUB IUAControl_UnsubscribePres_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_UnsubscribeResLst_Proxy( 
    IUAControl __RPC_FAR * This,
    /* [in] */ BSTR ResourceURI);


void __RPC_STUB IUAControl_UnsubscribeResLst_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUAControl_PublishStatus_Proxy( 
    IUAControl __RPC_FAR * This,
    /* [in] */ BSTR szURI,
    /* [in] */ BOOL bStatus);


void __RPC_STUB IUAControl_PublishStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IUAControl_INTERFACE_DEFINED__ */



#ifndef __UACOMLib_LIBRARY_DEFINED__
#define __UACOMLib_LIBRARY_DEFINED__

/* library UACOMLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_UACOMLib;

#ifndef ___IUAControlEvents_DISPINTERFACE_DEFINED__
#define ___IUAControlEvents_DISPINTERFACE_DEFINED__

/* dispinterface _IUAControlEvents */
/* [helpstring][uuid] */ 


EXTERN_C const IID DIID__IUAControlEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("3DFB7930-376E-44FD-822A-617CC4764FFA")
    _IUAControlEvents : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct _IUAControlEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            _IUAControlEvents __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            _IUAControlEvents __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            _IUAControlEvents __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            _IUAControlEvents __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            _IUAControlEvents __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            _IUAControlEvents __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            _IUAControlEvents __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        END_INTERFACE
    } _IUAControlEventsVtbl;

    interface _IUAControlEvents
    {
        CONST_VTBL struct _IUAControlEventsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define _IUAControlEvents_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define _IUAControlEvents_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define _IUAControlEvents_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define _IUAControlEvents_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define _IUAControlEvents_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define _IUAControlEvents_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define _IUAControlEvents_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* ___IUAControlEvents_DISPINTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_UAControl;

#ifdef __cplusplus

class DECLSPEC_UUID("D851737B-22E8-4947-B812-5FF1F9212732")
UAControl;
#endif
#endif /* __UACOMLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long __RPC_FAR *, unsigned long            , BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long __RPC_FAR *, BSTR __RPC_FAR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
