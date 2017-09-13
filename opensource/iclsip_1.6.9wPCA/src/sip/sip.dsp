# Microsoft Developer Studio Project File - Name="sip" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=sip - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "sip.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "sip.mak" CFG="sip - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "sip - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "sip - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "sip - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\lib\Release"
# PROP Intermediate_Dir "..\temp\Release\sip"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /G6 /MT /W3 /GX /O2 /I "..\\" /I "..\..\utillib\openssl\include" /D "ICL_IMS" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x404 /d "NDEBUG"
# ADD RSC /l 0x404 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "sip - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\lib\Debug"
# PROP Intermediate_Dir "..\temp\Debug\sip"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\\" /I "..\..\utillib\openssl\include" /D "ICL_IMS" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /FR /FD /GZ /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x404 /d "_DEBUG"
# ADD RSC /l 0x404 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\lib\Debug\sipd.lib"

!ENDIF 

# Begin Target

# Name "sip - Win32 Release"
# Name "sip - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\base64.c
# End Source File
# Begin Source File

SOURCE=.\md5c.c
# End Source File
# Begin Source File

SOURCE=.\rfc822.c
# End Source File
# Begin Source File

SOURCE=.\sip_cfg.c
# End Source File
# Begin Source File

SOURCE=.\sip_hdr.c
# End Source File
# Begin Source File

SOURCE=.\sip_req.c
# End Source File
# Begin Source File

SOURCE=.\sip_rsp.c
# End Source File
# Begin Source File

SOURCE=.\sip_tx.c
# End Source File
# Begin Source File

SOURCE=.\sip_url.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\base64.h
# End Source File
# Begin Source File

SOURCE=.\cclsip.h
# End Source File
# Begin Source File

SOURCE=.\md5.h
# End Source File
# Begin Source File

SOURCE=.\md5_g.h
# End Source File
# Begin Source File

SOURCE=.\rfc822.h
# End Source File
# Begin Source File

SOURCE=.\sip.h
# End Source File
# Begin Source File

SOURCE=.\sip_bdy.h
# End Source File
# Begin Source File

SOURCE=.\sip_cfg.h
# End Source File
# Begin Source File

SOURCE=.\sip_cm.h
# End Source File
# Begin Source File

SOURCE=.\sip_hdr.h
# End Source File
# Begin Source File

SOURCE=.\sip_int.h
# End Source File
# Begin Source File

SOURCE=.\sip_req.h
# End Source File
# Begin Source File

SOURCE=.\sip_rsp.h
# End Source File
# Begin Source File

SOURCE=.\sip_tx.h
# End Source File
# Begin Source File

SOURCE=.\sip_url.h
# End Source File
# End Group
# End Target
# End Project