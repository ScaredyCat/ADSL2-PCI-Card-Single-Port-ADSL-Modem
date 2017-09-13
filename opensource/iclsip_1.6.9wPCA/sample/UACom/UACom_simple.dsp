# Microsoft Developer Studio Project File - Name="UACom" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=UACom - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "UACom_simple.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "UACom_simple.mak" CFG="UACom - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "UACom - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "UACom - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "UACom - Win32 Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\Debug"
# PROP Intermediate_Dir "..\temp\Debug\UACom_simple"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I ".\\" /I ".\include" /I "..\..\utillib\stun" /I "..\..\utillib\CPIM" /I "..\..\utillib\dnssrv" /I "..\..\src\common" /I "..\..\src\adt" /I "..\..\src\low" /I "..\..\src\rtp" /I "..\..\src\sip" /I "..\..\src\sdp" /I "..\..\src\sipTx" /I "..\..\src\uacore" /I "..\..\src\simpleapi\\" /D "USESTUN" /D "_DEBUG" /D "_simple" /D "_ATL_STATIC_REGISTRY" /D "_WRITE_PROFILE" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_WINDLL" /FR /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x404 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x404 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 AudioCodec.lib stund.lib cpimd.lib simpleapid.lib g723d.lib g729d.lib UACoresd.lib cclsipsd.lib rtpd.lib xmlparse.lib xmltok.lib dnsapi.lib winmm.lib Rpcrt4.lib vfw32.lib ws2_32.lib wsock32.lib Iphlpapi.lib /nologo /subsystem:windows /dll /debug /machine:I386 /nodefaultlib:"libcd.lib" /out:"..\Debug/UACom.dll" /pdbtype:sept /libpath:"..\..\src\debug" /libpath:"..\..\src\lib\debug" /libpath:"..\..\codec\lib\debug" /libpath:"..\..\utillib\lib\debug" /libpath:"..\lib\debug" /libpath:".\lib"
# Begin Custom Build - Performing registration
OutDir=.\..\Debug
TargetPath=\CVSRoot\sip\sample\Debug\UACom.dll
InputPath=\CVSRoot\sip\sample\Debug\UACom.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "UACom - Win32 Release"

# PROP BASE Use_MFC 1
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "UACom___Win32_Release"
# PROP BASE Intermediate_Dir "UACom___Win32_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\Release"
# PROP Intermediate_Dir "..\temp\Release\UACom_simple"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O1 /I ".\\" /I "..\..\utillib\VideoLib" /I "..\..\utillib\dnssrv" /I "..\..\src\sipTx" /I "..\..\src\uacore" /I "..\..\src\common" /I "..\..\src\sip" /I "..\..\src\sdp" /I "..\..\src\adt" /I "..\..\src\parser" /I "..\..\src\utils" /I "..\..\src\low" /I "..\..\src\rtp" /D "_FOR_VIDEO" /D "_WRITE_PROFILE" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /D "_WINDLL" /FR /FD /c
# SUBTRACT BASE CPP /YX /Yc /Yu
# ADD CPP /nologo /MT /W3 /GX /O1 /I ".\\" /I ".\include" /I "..\..\utillib\stun" /I "..\..\utillib\CPIM" /I "..\..\utillib\dnssrv" /I "..\..\src\common" /I "..\..\src\adt" /I "..\..\src\low" /I "..\..\src\rtp" /I "..\..\src\sip" /I "..\..\src\sdp" /I "..\..\src\sipTx" /I "..\..\src\uacore" /I "..\..\src\simpleapi\\" /D "USESTUN" /D "NDEBUG" /D "_simple" /D "_ATL_STATIC_REGISTRY" /D "_WRITE_PROFILE" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_WINDLL" /FD /c
# SUBTRACT CPP /Fr /YX /Yc /Yu
# ADD BASE RSC /l 0x404 /d "NDEBUG"
# ADD RSC /l 0x404 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 VideoLib.lib g723.lib g729.lib H263.lib UACores.lib cclsips.lib rtp.lib dnsapi.lib winmm.lib Rpcrt4.lib vfw32.lib ws2_32.lib wsock32.lib /nologo /subsystem:windows /dll /machine:I386 /libpath:"..\..\src\release" /libpath:"..\..\src\lib\release" /libpath:"..\..\codec\lib\release" /libpath:"..\..\utillib\lib\release" /libpath:"..\lib\release"
# ADD LINK32 AudioCodec.lib stun.lib cpim.lib simpleapi.lib g723.lib g729.lib UACores.lib cclsips.lib rtp.lib xmlparse.lib xmltok.lib dnsapi.lib winmm.lib Rpcrt4.lib vfw32.lib ws2_32.lib wsock32.lib Iphlpapi.lib /nologo /subsystem:windows /dll /machine:I386 /nodefaultlib:"libc.lib" /out:"..\Release/UACom.dll" /libpath:"..\..\src\release" /libpath:"..\..\src\lib\release" /libpath:"..\..\codec\lib\release" /libpath:"..\..\utillib\lib\release" /libpath:"..\lib\release" /libpath:".\lib"
# Begin Custom Build - Performing registration
OutDir=.\..\Release
TargetPath=\CVSRoot\sip\sample\Release\UACom.dll
InputPath=\CVSRoot\sip\sample\Release\UACom.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "UACom - Win32 Debug"
# Name "UACom - Win32 Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\UACom.cpp
# End Source File
# Begin Source File

SOURCE=.\UACom.def
# End Source File
# Begin Source File

SOURCE=.\UACom.idl
# ADD MTL /tlb ".\UACom.tlb" /h "UACom.h" /iid "UACom_i.c" /Oicf
# End Source File
# Begin Source File

SOURCE=.\UACom.rc
# End Source File
# Begin Source File

SOURCE=.\UAControl.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\UAComCP.h
# End Source File
# Begin Source File

SOURCE=.\UAControl.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\uacontro.bmp
# End Source File
# Begin Source File

SOURCE=.\UAControl.rgs
# End Source File
# End Group
# Begin Group "UA"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\UA\CallManager.cpp
# End Source File
# Begin Source File

SOURCE=.\UA\CallManager.h
# End Source File
# Begin Source File

SOURCE=.\UA\cclRtp.c
# End Source File
# Begin Source File

SOURCE=.\UA\cclRtp.h
# End Source File
# Begin Source File

SOURCE=.\UA\MediaManager.cpp
# End Source File
# Begin Source File

SOURCE=.\UA\MediaManager.h
# End Source File
# Begin Source File

SOURCE=.\UA\SDPManager.cpp
# End Source File
# Begin Source File

SOURCE=.\UA\SDPManager.h
# End Source File
# Begin Source File

SOURCE=.\UA\UACommon.cpp
# End Source File
# Begin Source File

SOURCE=.\UA\UACommon.h
# End Source File
# Begin Source File

SOURCE=.\UA\UAProfile.cpp
# End Source File
# Begin Source File

SOURCE=.\UA\UAProfile.h
# End Source File
# Begin Source File

SOURCE=.\UA\WavInOut.cpp
# End Source File
# Begin Source File

SOURCE=.\UA\WavInOut.h
# End Source File
# End Group
# Begin Group "UI"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\UI\AudioPage.cpp
# End Source File
# Begin Source File

SOURCE=.\UI\AudioPage.h
# End Source File
# Begin Source File

SOURCE=.\UI\ConfirmDeleteDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\UI\ConfirmDeleteDlg.h
# End Source File
# Begin Source File

SOURCE=.\UI\CredentialEditDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\UI\CredentialEditDlg.h
# End Source File
# Begin Source File

SOURCE=.\UI\CredentialPage.cpp
# End Source File
# Begin Source File

SOURCE=.\UI\CredentialPage.h
# End Source File
# Begin Source File

SOURCE=.\UI\PreferSheet.cpp
# End Source File
# Begin Source File

SOURCE=.\UI\PreferSheet.h
# End Source File
# Begin Source File

SOURCE=.\UI\RTPPage.cpp
# End Source File
# Begin Source File

SOURCE=.\UI\RTPPage.h
# End Source File
# Begin Source File

SOURCE=.\UI\ServerPage.cpp
# End Source File
# Begin Source File

SOURCE=.\UI\ServerPage.h
# End Source File
# Begin Source File

SOURCE=.\UI\SimplePage.cpp
# End Source File
# Begin Source File

SOURCE=.\UI\SimplePage.h
# End Source File
# Begin Source File

SOURCE=.\UI\sipextpage.cpp
# End Source File
# Begin Source File

SOURCE=.\UI\sipextpage.h
# End Source File
# Begin Source File

SOURCE=.\UI\UserPage.cpp
# End Source File
# Begin Source File

SOURCE=.\UI\UserPage.h
# End Source File
# End Group
# Begin Group "SIMPLE"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\SIMPLE\PresManager.cpp
# End Source File
# Begin Source File

SOURCE=.\SIMPLE\PresManager.h
# End Source File
# Begin Source File

SOURCE=.\SIMPLE\UABuddy.cpp
# End Source File
# Begin Source File

SOURCE=.\SIMPLE\UABuddy.h
# End Source File
# End Group
# Begin Group "Include"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Include\g723.h
# End Source File
# Begin Source File

SOURCE=.\Include\g729.h
# End Source File
# Begin Source File

SOURCE=.\Include\g729a.h
# End Source File
# End Group
# Begin Source File

SOURCE=".\Release-Note.txt"
# End Source File
# End Target
# End Project
