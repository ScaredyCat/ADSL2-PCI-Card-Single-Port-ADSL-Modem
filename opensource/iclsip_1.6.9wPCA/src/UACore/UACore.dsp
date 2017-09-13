# Microsoft Developer Studio Project File - Name="uacore" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=uacore - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "UACore.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "UACore.mak" CFG="uacore - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "uacore - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "uacore - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "uacore - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\lib\Release"
# PROP Intermediate_Dir "..\temp\Release\uacore"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\\" /D "_WIN32" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /Fr /FD /c
# SUBTRACT CPP /X /YX
# ADD BASE RSC /l 0x404 /d "NDEBUG"
# ADD RSC /l 0x404 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\lib\release\UACores.lib"

!ELSEIF  "$(CFG)" == "uacore - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\lib\Debug"
# PROP Intermediate_Dir "..\temp\Debug\uacore"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\\" /D "_WIN32" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /FR /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x404 /d "_DEBUG"
# ADD RSC /l 0x404 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\lib\Debug\UACoresd.lib"

!ENDIF 

# Begin Target

# Name "uacore - Win32 Release"
# Name "uacore - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\ua_cfg.c
# End Source File
# Begin Source File

SOURCE=.\ua_class.c
# End Source File
# Begin Source File

SOURCE=.\ua_cm.c
# End Source File
# Begin Source File

SOURCE=.\ua_content.c
# End Source File
# Begin Source File

SOURCE=.\ua_dlg.c
# End Source File
# Begin Source File

SOURCE=.\ua_evtpkg.c
# End Source File
# Begin Source File

SOURCE=.\ua_int.c
# End Source File
# Begin Source File

SOURCE=.\ua_mgr.c
# End Source File
# Begin Source File

SOURCE=.\ua_msg.c
# End Source File
# Begin Source File

SOURCE=.\ua_sdp.c
# End Source File
# Begin Source File

SOURCE=.\ua_sipmsg.c
# End Source File
# Begin Source File

SOURCE=.\ua_sub.c
# End Source File
# Begin Source File

SOURCE=.\ua_user.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\ua_cfg.h
# End Source File
# Begin Source File

SOURCE=.\ua_class.h
# End Source File
# Begin Source File

SOURCE=.\ua_cm.h
# End Source File
# Begin Source File

SOURCE=.\ua_content.h
# End Source File
# Begin Source File

SOURCE=.\ua_core.h
# End Source File
# Begin Source File

SOURCE=.\ua_dlg.h
# End Source File
# Begin Source File

SOURCE=.\ua_evtpkg.h
# End Source File
# Begin Source File

SOURCE=.\ua_int.h
# End Source File
# Begin Source File

SOURCE=.\ua_mgr.h
# End Source File
# Begin Source File

SOURCE=.\ua_msg.h
# End Source File
# Begin Source File

SOURCE=.\ua_sdp.h
# End Source File
# Begin Source File

SOURCE=.\ua_sipmsg.h
# End Source File
# Begin Source File

SOURCE=.\ua_sub.h
# End Source File
# Begin Source File

SOURCE=.\ua_user.h
# End Source File
# End Group
# End Target
# End Project
