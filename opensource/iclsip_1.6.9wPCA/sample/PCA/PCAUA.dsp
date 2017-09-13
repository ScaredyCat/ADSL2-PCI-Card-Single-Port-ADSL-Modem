# Microsoft Developer Studio Project File - Name="PCAUA" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=PCAUA - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "PCAUA.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "PCAUA.mak" CFG="PCAUA - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "PCAUA - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "PCAUA - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Source/PCAUA", DAAAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "PCAUA - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I ".\include" /I "PCAUARes_V3oIP" /I "..\..\..\sip\src" /D "_PCAUA_Res_" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_SIMPLE" /D "_XCAPSERVER" /YX"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /win32
# ADD BASE RSC /l 0x404 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x404 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 winmm.lib wsock32.lib ws2_32.lib rpcrt4.lib lib\xmlrpclib.lib PCAUARes.lib lib\xmlparse.lib DualNetLibd.lib /nologo /subsystem:windows /machine:I386 /nodefaultlib:"LIBCMTD.lib" /out:"bin/PCAUA.exe" /libpath:"./lib" /libpath:"PCAUARes_V3oIP\Release"

!ELSEIF  "$(CFG)" == "PCAUA - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I ".\include" /I "PCAUARes_V3oIP" /I "..\..\src" /D "_PCAUA_Res_" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_SIMPLE" /D "_XCAPSERVER" /FR /YX"stdafx.h" /FD /GZ /Gs /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /win32
# ADD BASE RSC /l 0x404 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x404 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 winmm.lib wsock32.lib ws2_32.lib rpcrt4.lib lib\xmlrpclib.lib PCAUAResd.lib /nologo /subsystem:windows /debug /machine:I386 /out:"bin/PCAUAd.exe" /pdbtype:sept /libpath:"./lib" /libpath:"PCAUARes\Debug"

!ENDIF 

# Begin Target

# Name "PCAUA - Win32 Release"
# Name "PCAUA - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\AddPersonalDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\AddPersonDisplayNameDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\SkinSys\AnimatedLabel.cpp
# End Source File
# Begin Source File

SOURCE=.\SkinSys\BitmapBtn.cpp
# End Source File
# Begin Source File

SOURCE=.\SkinSys\BitmapProgress.cpp
# End Source File
# Begin Source File

SOURCE=.\SkinSys\BitmapSlider.cpp
# End Source File
# Begin Source File

SOURCE=.\CallDlgList.cpp
# End Source File
# Begin Source File

SOURCE=.\CallLog.cpp
# End Source File
# Begin Source File

SOURCE=.\CallLogDelDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\CallLogDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\dbaccess.cpp
# End Source File
# Begin Source File

SOURCE=.\DlgBrowser.cpp
# End Source File
# Begin Source File

SOURCE=.\DlgJoinMessage.cpp
# End Source File
# Begin Source File

SOURCE=.\DlgMessage.cpp
# End Source File
# Begin Source File

SOURCE=.\DlgRemoteVideo.cpp
# End Source File
# Begin Source File

SOURCE=.\EditItriDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\EditPersonalDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\EMailDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\EnterURIDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\EscapeCoding.cpp
# End Source File
# Begin Source File

SOURCE=.\GroupDeleteDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\GroupEditDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\GroupManageDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\GroupMsgDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\HelpDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\SkinSys\Inifile.Cpp
# End Source File
# Begin Source File

SOURCE=.\IPPrefDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\netadapter.cpp
# End Source File
# Begin Source File

SOURCE=.\Pack.cpp
# End Source File
# Begin Source File

SOURCE=.\PanelDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\PCASplashDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\PCAUA.cpp
# End Source File
# Begin Source File

SOURCE=.\PCAUA.idl
# ADD MTL /h "PCAUA_i.h" /iid "PCAUA_i.c" /Oicf
# End Source File
# Begin Source File

SOURCE=.\PCAUA.rc
# End Source File
# Begin Source File

SOURCE=.\PCAUADlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Personal.cpp
# End Source File
# Begin Source File

SOURCE=.\Picture.cpp
# End Source File
# Begin Source File

SOURCE=.\PrefDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\QueryDB.cpp
# End Source File
# Begin Source File

SOURCE=.\RepeatOptDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\SearchDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\SignalListCtrlEx.cpp
# End Source File
# Begin Source File

SOURCE=.\SignalListDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\SkinSys\SkinButton.cpp
# End Source File
# Begin Source File

SOURCE=.\SkinSys\SkinDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\SkinSys\SkinLabel.cpp
# End Source File
# Begin Source File

SOURCE=.\SkinSys\SkinProgress.cpp
# End Source File
# Begin Source File

SOURCE=.\SkinSys\SkinSlider.cpp
# End Source File
# Begin Source File

SOURCE=.\SortHeaderCtrl.cpp
# End Source File
# Begin Source File

SOURCE=.\SortListCtrl.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\StringTokenizer.cpp
# End Source File
# Begin Source File

SOURCE=.\SystemTray.cpp
# End Source File
# Begin Source File

SOURCE=.\UACDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\uacom.cpp
# End Source File
# Begin Source File

SOURCE=.\VoiceMailLogin.cpp
# End Source File
# Begin Source File

SOURCE=.\xcap_low.cpp
# End Source File
# Begin Source File

SOURCE=.\xcapapi.cpp
# End Source File
# Begin Source File

SOURCE=.\xcapxml_parser.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\AddPersonalDlg.h
# End Source File
# Begin Source File

SOURCE=.\AddPersonDisplayNameDlg.h
# End Source File
# Begin Source File

SOURCE=.\SkinSys\AnimatedLabel.h
# End Source File
# Begin Source File

SOURCE=.\SkinSys\BitmapBtn.h
# End Source File
# Begin Source File

SOURCE=.\SkinSys\BitmapProgress.h
# End Source File
# Begin Source File

SOURCE=.\SkinSys\BitmapSlider.h
# End Source File
# Begin Source File

SOURCE=.\BSOperation.h
# End Source File
# Begin Source File

SOURCE=.\CallDlgList.h
# End Source File
# Begin Source File

SOURCE=.\CallLog.h
# End Source File
# Begin Source File

SOURCE=.\CallLogDelDlg.h
# End Source File
# Begin Source File

SOURCE=.\CallLogDlg.h
# End Source File
# Begin Source File

SOURCE=.\dbaccess.h
# End Source File
# Begin Source File

SOURCE=.\DlgBrowser.h
# End Source File
# Begin Source File

SOURCE=.\DlgJoinMessage.h
# End Source File
# Begin Source File

SOURCE=.\DlgMessage.h
# End Source File
# Begin Source File

SOURCE=.\DlgRemoteVideo.h
# End Source File
# Begin Source File

SOURCE=.\EditItriDlg.h
# End Source File
# Begin Source File

SOURCE=.\EditPersonalDlg.h
# End Source File
# Begin Source File

SOURCE=.\EMailDialog.h
# End Source File
# Begin Source File

SOURCE=.\EnterURIDlg.h
# End Source File
# Begin Source File

SOURCE=.\EscapeCoding.h
# End Source File
# Begin Source File

SOURCE=.\GroupDeleteDlg.h
# End Source File
# Begin Source File

SOURCE=.\GroupEditDlg.h
# End Source File
# Begin Source File

SOURCE=.\GroupManageDlg.h
# End Source File
# Begin Source File

SOURCE=.\GroupMsgDlg.h
# End Source File
# Begin Source File

SOURCE=.\HelpDlg.h
# End Source File
# Begin Source File

SOURCE=.\SkinSys\IniFile.H
# End Source File
# Begin Source File

SOURCE=.\IPPrefDlg.h
# End Source File
# Begin Source File

SOURCE=.\netadapter.h
# End Source File
# Begin Source File

SOURCE=.\Pack.h
# End Source File
# Begin Source File

SOURCE=.\PanelDlg.h
# End Source File
# Begin Source File

SOURCE=.\PCASplashDlg.h
# End Source File
# Begin Source File

SOURCE=.\PCAUA.h
# End Source File
# Begin Source File

SOURCE=.\PCAUADlg.h
# End Source File
# Begin Source File

SOURCE=.\Personal.h
# End Source File
# Begin Source File

SOURCE=.\Picture.h
# End Source File
# Begin Source File

SOURCE=.\PrefDlg.h
# End Source File
# Begin Source File

SOURCE=.\QueryDB.h
# End Source File
# Begin Source File

SOURCE=.\RepeatOptDlg.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\SearchDlg.h
# End Source File
# Begin Source File

SOURCE=.\SignalListCtrlEx.h
# End Source File
# Begin Source File

SOURCE=.\SignalListDlg.h
# End Source File
# Begin Source File

SOURCE=.\SkinSys\SkinButton.h
# End Source File
# Begin Source File

SOURCE=.\SkinSys\SkinDialog.h
# End Source File
# Begin Source File

SOURCE=.\SkinSys\SkinLabel.h
# End Source File
# Begin Source File

SOURCE=.\SkinSys\SkinProgress.h
# End Source File
# Begin Source File

SOURCE=.\SkinSys\SkinSlider.h
# End Source File
# Begin Source File

SOURCE=.\SortHeaderCtrl.h
# End Source File
# Begin Source File

SOURCE=.\SortListCtrl.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\StringTokenizer.h
# End Source File
# Begin Source File

SOURCE=.\SystemTray.h
# End Source File
# Begin Source File

SOURCE=.\UACDlg.h
# End Source File
# Begin Source File

SOURCE=.\uacom.h
# End Source File
# Begin Source File

SOURCE=.\VoiceMailLogin.h
# End Source File
# Begin Source File

SOURCE=.\winerr.h
# End Source File
# Begin Source File

SOURCE=.\xcap_datatype.h
# End Source File
# Begin Source File

SOURCE=.\xcap_low.h
# End Source File
# Begin Source File

SOURCE=.\xcapapi.h
# End Source File
# Begin Source File

SOURCE=.\xcapxml_parser.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\BeepTone.wav
# End Source File
# Begin Source File

SOURCE=.\res\btn_offl.bmp
# End Source File
# Begin Source File

SOURCE=.\res\btn_onli.bmp
# End Source File
# Begin Source File

SOURCE=.\res\btnAdd.bmp
# End Source File
# Begin Source File

SOURCE=.\res\btnClear.bmp
# End Source File
# Begin Source File

SOURCE=.\res\btnDel.bmp
# End Source File
# Begin Source File

SOURCE=.\res\btnDetail.bmp
# End Source File
# Begin Source File

SOURCE=.\res\btnDial.bmp
# End Source File
# Begin Source File

SOURCE=.\res\btnEdit.bmp
# End Source File
# Begin Source File

SOURCE=.\res\btnListen.bmp
# End Source File
# Begin Source File

SOURCE=.\res\btnProfile.bmp
# End Source File
# Begin Source File

SOURCE=.\res\btnSearch.bmp
# End Source File
# Begin Source File

SOURCE=.\res\btnSetting.bmp
# End Source File
# Begin Source File

SOURCE=.\res\BusyTone.wav
# End Source File
# Begin Source File

SOURCE=.\res\calllog_in_miss.bmp
# End Source File
# Begin Source File

SOURCE=.\res\calllog_in_reject.bmp
# End Source File
# Begin Source File

SOURCE=.\res\calllog_out_fail.bmp
# End Source File
# Begin Source File

SOURCE=.\res\callog_in.bmp.bmp
# End Source File
# Begin Source File

SOURCE=.\res\callog_out.bmp.bmp
# End Source File
# Begin Source File

SOURCE=.\res\cursor1.cur
# End Source File
# Begin Source File

SOURCE=.\SkinImg\Panel\disable.bmp
# End Source File
# Begin Source File

SOURCE=.\SkinImg\Phone\disable.bmp
# End Source File
# Begin Source File

SOURCE=.\SkinImg\Phone\disable2.bmp
# End Source File
# Begin Source File

SOURCE=.\SkinImg\Panel\down.bmp
# End Source File
# Begin Source File

SOURCE=.\SkinImg\Phone\down.bmp
# End Source File
# Begin Source File

SOURCE=.\SkinImg\Phone\down2.bmp
# End Source File
# Begin Source File

SOURCE=.\res\HoldTone.wav
# End Source File
# Begin Source File

SOURCE=.\SkinImg\Panel\main.bmp
# End Source File
# Begin Source File

SOURCE=.\SkinImg\Phone\main.bmp
# End Source File
# Begin Source File

SOURCE=.\SkinImg\Phone\main2.bmp
# End Source File
# Begin Source File

SOURCE=.\SkinImg\Panel\mask.bmp
# End Source File
# Begin Source File

SOURCE=.\SkinImg\Phone\mask.bmp
# End Source File
# Begin Source File

SOURCE=.\res\mcicon_alert.bmp
# End Source File
# Begin Source File

SOURCE=.\res\mcicon_conn.bmp
# End Source File
# Begin Source File

SOURCE=.\res\mcicon_hold.bmp
# End Source File
# Begin Source File

SOURCE=.\res\mcicon_ringbk.bmp
# End Source File
# Begin Source File

SOURCE=.\res\mcPanel.bmp
# End Source File
# Begin Source File

SOURCE=.\res\message.ico
# End Source File
# Begin Source File

SOURCE=.\res\offline.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Online.bmp
# End Source File
# Begin Source File

SOURCE=.\SkinImg\Panel\over.bmp
# End Source File
# Begin Source File

SOURCE=.\SkinImg\Phone\over.bmp
# End Source File
# Begin Source File

SOURCE=.\res\PCAUA.ico
# End Source File
# Begin Source File

SOURCE=.\res\PCAUA.rc2
# End Source File
# Begin Source File

SOURCE=.\PCAUA.rgs
# End Source File
# Begin Source File

SOURCE=.\res\pre_offline.bmp
# End Source File
# Begin Source File

SOURCE=.\res\pre_online.bmp
# End Source File
# Begin Source File

SOURCE=.\res\pre_unknown.bmp
# End Source File
# Begin Source File

SOURCE=.\res\RingbackTone.wav
# End Source File
# Begin Source File

SOURCE=.\res\RingTone.wav
# End Source File
# Begin Source File

SOURCE=.\skinPanel.ini
# End Source File
# Begin Source File

SOURCE=.\skinPhone.ini
# End Source File
# Begin Source File

SOURCE=.\res\unknown.bmp
# End Source File
# Begin Source File

SOURCE=.\res\user_sta.bmp
# End Source File
# Begin Source File

SOURCE=.\res\v3oip.ico
# End Source File
# Begin Source File

SOURCE=.\res\videolabel.bmp
# End Source File
# Begin Source File

SOURCE=.\res\videolabel_w.bmp
# End Source File
# Begin Source File

SOURCE=.\res\videotitle.bmp
# End Source File
# Begin Source File

SOURCE=.\res\videotitle2.bmp
# End Source File
# Begin Source File

SOURCE=.\res\voicemai.bmp
# End Source File
# Begin Source File

SOURCE=.\res\WaitingTone.wav
# End Source File
# Begin Source File

SOURCE=.\res\wave10.bin
# End Source File
# End Group
# Begin Source File

SOURCE=.\ReleaseNote.txt
# End Source File
# Begin Source File

SOURCE=.\VersionNo.h
# End Source File
# End Target
# End Project
# Section PCAUA : {D851737B-22E8-4947-B812-5FF1F9212732}
# 	2:21:DefaultSinkHeaderFile:uacontrol.h
# 	2:16:DefaultSinkClass:CUAControl
# End Section
# Section PCAUA : {4A91A134-1B60-497E-87EE-23C97B2A6D84}
# 	2:5:Class:CUAControl
# 	2:10:HeaderFile:uacontrol.h
# 	2:8:ImplFile:uacontrol.cpp
# End Section
# Section PCAUA : {002F7302-7302-0038-0264-1300026F3000}
# 	1:9:IDR_PCAUA:106
# End Section
