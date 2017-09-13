# Microsoft Developer Studio Project File - Name="PCAUARes" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=PCAUARes - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "PCAUARes.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "PCAUARes.mak" CFG="PCAUARes - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "PCAUARes - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "PCAUARes - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "PCAUARes - Win32 Release"

# PROP BASE Use_MFC 5
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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x404 /d "NDEBUG"
# ADD RSC /l 0x404 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 /nologo /subsystem:windows /dll /machine:I386 /out:"../bin/PCAUARes.dll"

!ELSEIF  "$(CFG)" == "PCAUARes - Win32 Debug"

# PROP BASE Use_MFC 5
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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_MBCS" /D "_USRDLL" /FR /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x404 /d "_DEBUG"
# ADD RSC /l 0x404 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /out:"../bin/PCAUAResd.dll" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "PCAUARes - Win32 Release"
# Name "PCAUARes - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\PCAUARes.cpp
# End Source File
# Begin Source File

SOURCE=.\PCAUARes.def
# End Source File
# Begin Source File

SOURCE=.\PCAUARes.rc
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\PCAUARes.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
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

SOURCE=.\res\calllog_in.bmp
# End Source File
# Begin Source File

SOURCE=.\res\calllog_in_miss.bmp
# End Source File
# Begin Source File

SOURCE=.\res\calllog_in_reject.bmp
# End Source File
# Begin Source File

SOURCE=.\res\calllog_out.bmp
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

SOURCE=.\SkinImg\Panel\over.bmp
# End Source File
# Begin Source File

SOURCE=.\SkinImg\Phone\over.bmp
# End Source File
# Begin Source File

SOURCE=.\res\PCAUA.ico
# End Source File
# Begin Source File

SOURCE=.\res\PCAUARes.rc2
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

SOURCE=.\res\voicemail.bmp
# End Source File
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
