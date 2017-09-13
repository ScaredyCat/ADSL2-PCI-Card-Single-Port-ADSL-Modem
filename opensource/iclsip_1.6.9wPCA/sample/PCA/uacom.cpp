// Machine generated IDispatch wrapper class(es) created with ClassWizard

#include "stdafx.h"
#include "uacom.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



/////////////////////////////////////////////////////////////////////////////
// _IUAControlEvents properties

/////////////////////////////////////////////////////////////////////////////
// _IUAControlEvents operations


/////////////////////////////////////////////////////////////////////////////
// IUAControl properties

/////////////////////////////////////////////////////////////////////////////
// IUAControl operations

void IUAControl::Initialize(LPCTSTR realm, LPCTSTR userid, LPCTSTR password)
{
	static BYTE parms[] =
		VTS_BSTR VTS_BSTR VTS_BSTR;
	InvokeHelper(0x1, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 realm, userid, password);
}

void IUAControl::CancelCall()
{
	InvokeHelper(0x2, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

void IUAControl::DropCall(long dlgHandle)
{
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0x3, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 dlgHandle);
}

void IUAControl::AnswerCall(long dlgHandle)
{
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0x4, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 dlgHandle);
}

void IUAControl::RejectCall(long dlgHandle)
{
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0x5, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 dlgHandle);
}

long IUAControl::MakeCall(LPCTSTR dialURL)
{
	long result;
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x6, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		dialURL);
	return result;
}

long IUAControl::ShowPreferences()
{
	//bool result;
	long result;
	InvokeHelper(0x7, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
	
}

void IUAControl::Test(LPCTSTR tmpstr)
{
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x8, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 tmpstr);
}

void IUAControl::HoldCall()
{
	InvokeHelper(0x9, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

void IUAControl::UnHoldCall(long dlgHandle)
{
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0xa, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 dlgHandle);
}

void IUAControl::UnAttendXfer(long dlgHandle, LPCTSTR xferURL)
{
	static BYTE parms[] =
		VTS_I4 VTS_BSTR;
	InvokeHelper(0xb, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 dlgHandle, xferURL);
}

void IUAControl::AttendXfer(long dlgHandle1, long dlgHandle2)
{
	static BYTE parms[] =
		VTS_I4 VTS_I4;
	InvokeHelper(0xc, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 dlgHandle1, dlgHandle2);
}

CString IUAControl::GetRemoteParty(long dlgHandle)
{
	CString result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0xd, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		dlgHandle);
	return result;
}

void IUAControl::StartLocalVideo()
{
	InvokeHelper(0xe, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

void IUAControl::StopLocalVideo()
{
	InvokeHelper(0xf, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

void IUAControl::Register()
{
	InvokeHelper(0x10, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

void IUAControl::UnRegister()
{
	InvokeHelper(0x11, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

void IUAControl::RegQuery()
{
	InvokeHelper(0x12, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

void IUAControl::SwitchLocalVideo()
{
	InvokeHelper(0x13, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

void IUAControl::Stop()
{
	InvokeHelper(0x14, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

CString IUAControl::GetLocalIP()
{
	CString result;
	InvokeHelper(0x15, DISPATCH_PROPERTYGET, VT_BSTR, (void*)&result, NULL);
	return result;
}

void IUAControl::SetLocalIP(LPCTSTR lpszNewValue)
{
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x15, DISPATCH_PROPERTYPUT, VT_EMPTY, NULL, parms,
		 lpszNewValue);
}

void IUAControl::SendDTMF0()
{
	InvokeHelper(0x16, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

void IUAControl::SendDTMF1()
{
	InvokeHelper(0x17, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

void IUAControl::SendDTMF2()
{
	InvokeHelper(0x18, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

void IUAControl::SendDTMF3()
{
	InvokeHelper(0x19, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

void IUAControl::SendDTMF4()
{
	InvokeHelper(0x1a, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

void IUAControl::SendDTMF5()
{
	InvokeHelper(0x1b, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

void IUAControl::SendDTMF6()
{
	InvokeHelper(0x1c, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

void IUAControl::SendDTMF7()
{
	InvokeHelper(0x1d, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

void IUAControl::SendDTMF8()
{
	InvokeHelper(0x1e, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

void IUAControl::SendDTMF9()
{
	InvokeHelper(0x1f, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

void IUAControl::SendDTMFP()
{
	InvokeHelper(0x20, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

void IUAControl::SendDTMFS()
{
	InvokeHelper(0x21, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

long IUAControl::GetBSingleCall()
{
	long result;
	InvokeHelper(0x22, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, NULL);
	return result;
}

void IUAControl::SetBSingleCall(long nNewValue)
{
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0x22, DISPATCH_PROPERTYPUT, VT_EMPTY, NULL, parms,
		 nNewValue);
}

void IUAControl::Reset()
{
	InvokeHelper(0x23, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

long IUAControl::GetAudioType()
{
	long result;
	InvokeHelper(0x24, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

long IUAControl::GetVideoType()
{
	long result;
	InvokeHelper(0x25, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

long IUAControl::GetVideoSize()
{
	long result;
	InvokeHelper(0x26, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

CString IUAControl::GetRemotePartyDisplayName(long dlgHandle)
{
	CString result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0x27, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		dlgHandle);
	return result;
}

long IUAControl::GetMediaReceiveQuality()
{
	long result;
	InvokeHelper(0x28, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

void IUAControl::StartRemoteVideo(long x, long y, long scale)
{
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4;
	InvokeHelper(0x29, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 x, y, scale);
}

void IUAControl::StopRemoteVideo()
{
	InvokeHelper(0x2a, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

void IUAControl::StartLocalVideoEx(long nhwnd, long x, long y, long scale)
{
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4;
	InvokeHelper(0x2b, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 nhwnd, x, y, scale);
}

void IUAControl::StartRemoteVideoEx(long nhwnd, long x, long y, long scale)
{
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4;
	InvokeHelper(0x2c, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 nhwnd, x, y, scale);
}

void IUAControl::SendText(LPCTSTR dialURL, LPCTSTR tmpstr)
{
	static BYTE parms[] =
		VTS_BSTR VTS_BSTR;
	InvokeHelper(0x2d, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 dialURL, tmpstr);
}

void IUAControl::AddBuddy(LPCTSTR buddyURI)
{
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x2e, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 buddyURI);
}

void IUAControl::AuthorizeBuddy(LPCTSTR buddyURI)
{
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x2f, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 buddyURI);
}

void IUAControl::UnauthorizeBuddy(LPCTSTR buddyURI)
{
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x30, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 buddyURI);
}

void IUAControl::DelBuddy(LPCTSTR buddyURI)
{
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x31, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 buddyURI);
}

void IUAControl::BlockBuddy(LPCTSTR buddyURI)
{
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x32, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 buddyURI);
}

void IUAControl::UnBlockBuddy(LPCTSTR buddyURI)
{
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x33, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 buddyURI);
}

long IUAControl::GetBuddyCount()
{
	long result;
	InvokeHelper(0x34, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

CString IUAControl::GetBuddyURI(long number)
{
	CString result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0x35, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		number);
	return result;
}

void IUAControl::UpdateBasicStatus(long bOpen)
{
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0x36, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 bOpen);
}

long IUAControl::GetBuddyINSubState(LPCTSTR buddyURI)
{
	long result;
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x37, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		buddyURI);
	return result;
}

long IUAControl::GetBuddyOUTSubState(LPCTSTR buddyURI)
{
	long result;
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x38, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		buddyURI);
	return result;
}

long IUAControl::GetBuddyBasicStatus(LPCTSTR buddyURI)
{
	long result;
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x39, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		buddyURI);
	return result;
}

void IUAControl::SubscribeBuddy(LPCTSTR buddyURI)
{
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x3a, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 buddyURI);
}

void IUAControl::UnsubscribeBuddy(LPCTSTR buddyURI)
{
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x3b, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 buddyURI);
}

long IUAControl::IsBuddyBlock(LPCTSTR buddyURI)
{
	long result;
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x3c, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		buddyURI);
	return result;
}

long IUAControl::IsBuddyAuthorized(LPCTSTR buddyURI)
{
	long result;
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x3d, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		buddyURI);
	return result;
}

void IUAControl::SendInfo(LPCTSTR bstrURL, LPCTSTR bstrContentType, LPCTSTR bstrBody)
{
	static BYTE parms[] =
		VTS_BSTR VTS_BSTR VTS_BSTR;
	InvokeHelper(0x3e, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 bstrURL, bstrContentType, bstrBody);
}

void IUAControl::SubscribeWinfo(LPCTSTR buddyURI)
{
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x3f, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 buddyURI);
}

void IUAControl::SubscribePres(LPCTSTR buddyURI)
{
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x40, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 buddyURI);
}

void IUAControl::SubscribeResLst(LPCTSTR ResourceURI)
{
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x41, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 ResourceURI);
}

void IUAControl::UnsubscribeWinfo(LPCTSTR buddyURI)
{
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x42, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 buddyURI);
}

void IUAControl::UnsubscribePres(LPCTSTR buddyURI)
{
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x43, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 buddyURI);
}

void IUAControl::UnsubscribeResLst(LPCTSTR ResourceURI)
{
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x44, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 ResourceURI);
}

void IUAControl::PublishStatus(LPCTSTR szURI, long bStatus)
{
	static BYTE parms[] =
		VTS_BSTR VTS_I4;
	InvokeHelper(0x45, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 szURI, bStatus);
}

void IUAControl::TestSIMPLE(LPCTSTR p_Test)
{
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x46, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 p_Test);
}
