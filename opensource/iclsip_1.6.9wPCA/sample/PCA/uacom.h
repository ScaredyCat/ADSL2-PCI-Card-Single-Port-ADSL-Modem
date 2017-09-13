// Machine generated IDispatch wrapper class(es) created with ClassWizard
/////////////////////////////////////////////////////////////////////////////
// _IUAControlEvents wrapper class

class _IUAControlEvents : public COleDispatchDriver
{
public:
	_IUAControlEvents() {}		// Calls COleDispatchDriver default constructor
	_IUAControlEvents(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	_IUAControlEvents(const _IUAControlEvents& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// Attributes
public:

// Operations
public:
	// method 'ShowText' not emitted because of invalid return type or parameter type
	// method 'Ringback' not emitted because of invalid return type or parameter type
	// method 'Alerting' not emitted because of invalid return type or parameter type
	// method 'Proceeding' not emitted because of invalid return type or parameter type
	// method 'Connected' not emitted because of invalid return type or parameter type
	// method 'Disconnected' not emitted because of invalid return type or parameter type
	// method 'TimeOut' not emitted because of invalid return type or parameter type
	// method 'Dialing' not emitted because of invalid return type or parameter type
	// method 'Busy' not emitted because of invalid return type or parameter type
	// method 'Reject' not emitted because of invalid return type or parameter type
	// method 'Transferred' not emitted because of invalid return type or parameter type
	// method 'Held' not emitted because of invalid return type or parameter type
	// method 'RegistrationDone' not emitted because of invalid return type or parameter type
	// method 'RegistrationFail' not emitted because of invalid return type or parameter type
	// method 'NeedAuthentication' not emitted because of invalid return type or parameter type
	// method 'Waiting' not emitted because of invalid return type or parameter type
	// method 'RegistrationTimeout' not emitted because of invalid return type or parameter type
	// method 'Cancelling' not emitted because of invalid return type or parameter type
	// method 'TransportErr' not emitted because of invalid return type or parameter type
	// method 'RegistrationTxpErr' not emitted because of invalid return type or parameter type
	// method 'BuddyAllowSubscription' not emitted because of invalid return type or parameter type
	// method 'BuddyDenySubscription' not emitted because of invalid return type or parameter type
	// method 'BuddyPendingSubscription' not emitted because of invalid return type or parameter type
	// method 'BuddyRemoveSubscription' not emitted because of invalid return type or parameter type
	// method 'BuddyUpdateBasicStatus' not emitted because of invalid return type or parameter type
	// method 'BuddyRequestForContact' not emitted because of invalid return type or parameter type
	// method 'BuddyAddContact' not emitted because of invalid return type or parameter type
	// method 'BuddyRemoveContact' not emitted because of invalid return type or parameter type
	// method 'ReceivedText' not emitted because of invalid return type or parameter type
	// method 'InfoResponse' not emitted because of invalid return type or parameter type
	// method 'InfoRequest' not emitted because of invalid return type or parameter type
	// method 'NeedAuthorize' not emitted because of invalid return type or parameter type
};
/////////////////////////////////////////////////////////////////////////////
// IUAControl wrapper class

class IUAControl : public COleDispatchDriver
{
public:
	IUAControl() {}		// Calls COleDispatchDriver default constructor
	IUAControl(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	IUAControl(const IUAControl& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// Attributes
public:

// Operations
public:
	void Initialize(LPCTSTR realm, LPCTSTR userid, LPCTSTR password);
	void CancelCall();
	void DropCall(long dlgHandle);
	void AnswerCall(long dlgHandle);
	void RejectCall(long dlgHandle);
	long MakeCall(LPCTSTR dialURL);
	long ShowPreferences();
	void Test(LPCTSTR tmpstr);
	void HoldCall();
	void UnHoldCall(long dlgHandle);
	void UnAttendXfer(long dlgHandle, LPCTSTR xferURL);
	void AttendXfer(long dlgHandle1, long dlgHandle2);
	CString GetRemoteParty(long dlgHandle);
	void StartLocalVideo();
	void StopLocalVideo();
	void Register();
	void UnRegister();
	void RegQuery();
	void SwitchLocalVideo();
	void Stop();
	CString GetLocalIP();
	void SetLocalIP(LPCTSTR lpszNewValue);
	void SendDTMF0();
	void SendDTMF1();
	void SendDTMF2();
	void SendDTMF3();
	void SendDTMF4();
	void SendDTMF5();
	void SendDTMF6();
	void SendDTMF7();
	void SendDTMF8();
	void SendDTMF9();
	void SendDTMFP();
	void SendDTMFS();
	long GetBSingleCall();
	void SetBSingleCall(long nNewValue);
	void Reset();
	long GetAudioType();
	long GetVideoType();
	long GetVideoSize();
	CString GetRemotePartyDisplayName(long dlgHandle);
	long GetMediaReceiveQuality();
	void StartRemoteVideo(long x, long y, long scale);
	void StopRemoteVideo();
	void StartLocalVideoEx(long nhwnd, long x, long y, long scale);
	void StartRemoteVideoEx(long nhwnd, long x, long y, long scale);
	void SendText(LPCTSTR dialURL, LPCTSTR tmpstr);
	void AddBuddy(LPCTSTR buddyURI);
	void AuthorizeBuddy(LPCTSTR buddyURI);
	void UnauthorizeBuddy(LPCTSTR buddyURI);
	void DelBuddy(LPCTSTR buddyURI);
	void BlockBuddy(LPCTSTR buddyURI);
	void UnBlockBuddy(LPCTSTR buddyURI);
	long GetBuddyCount();
	CString GetBuddyURI(long number);
	void UpdateBasicStatus(long bOpen);
	long GetBuddyINSubState(LPCTSTR buddyURI);
	long GetBuddyOUTSubState(LPCTSTR buddyURI);
	long GetBuddyBasicStatus(LPCTSTR buddyURI);
	void SubscribeBuddy(LPCTSTR buddyURI);
	void UnsubscribeBuddy(LPCTSTR buddyURI);
	long IsBuddyBlock(LPCTSTR buddyURI);
	long IsBuddyAuthorized(LPCTSTR buddyURI);
	void SendInfo(LPCTSTR bstrURL, LPCTSTR bstrContentType, LPCTSTR bstrBody);
	void SubscribeWinfo(LPCTSTR buddyURI);
	void SubscribePres(LPCTSTR buddyURI);
	void SubscribeResLst(LPCTSTR ResourceURI);
	void UnsubscribeWinfo(LPCTSTR buddyURI);
	void UnsubscribePres(LPCTSTR buddyURI);
	void UnsubscribeResLst(LPCTSTR ResourceURI);
	void PublishStatus(LPCTSTR szURI, long bStatus);
	void TestSIMPLE(LPCTSTR p_Test);
};
