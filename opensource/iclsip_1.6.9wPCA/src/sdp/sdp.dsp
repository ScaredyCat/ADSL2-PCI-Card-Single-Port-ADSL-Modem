# Microsoft Developer Studio Project File - Name="sdp" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=sdp - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "sdp.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "sdp.mak" CFG="sdp - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "sdp - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "sdp - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "sdp - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\lib\release"
# PROP Intermediate_Dir "..\temp\release\sdp"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\\" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x404 /d "NDEBUG"
# ADD RSC /l 0x404 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "sdp - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\lib\debug"
# PROP Intermediate_Dir "..\temp\debug\sdp"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\\" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /Fr /YX /FD /GZ /c
# ADD BASE RSC /l 0x404 /d "_DEBUG"
# ADD RSC /l 0x404 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\lib\Debug\sdpd.lib"

!ENDIF 

# Begin Target

# Name "sdp - Win32 Release"
# Name "sdp - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\sdp_mses.c
# End Source File
# Begin Source File

SOURCE=.\sdp_sess.c
# End Source File
# Begin Source File

SOURCE=.\sdp_ta.c
# End Source File
# Begin Source File

SOURCE=.\sdp_tb.c
# End Source File
# Begin Source File

SOURCE=.\sdp_tc.c
# End Source File
# Begin Source File

SOURCE=.\sdp_tk.c
# End Source File
# Begin Source File

SOURCE=.\sdp_tm.c
# End Source File
# Begin Source File

SOURCE=.\sdp_to.c
# End Source File
# Begin Source File

SOURCE=.\sdp_tok.c
# End Source File
# Begin Source File

SOURCE=.\sdp_tr.c
# End Source File
# Begin Source File

SOURCE=.\sdp_tses.c
# End Source File
# Begin Source File

SOURCE=.\sdp_tt.c
# End Source File
# Begin Source File

SOURCE=.\sdp_tz.c
# End Source File
# Begin Source File

SOURCE=.\sdp_utl.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\sdp.h
# End Source File
# Begin Source File

SOURCE=.\sdp_def.h
# End Source File
# Begin Source File

SOURCE=.\sdp_mses.h
# End Source File
# Begin Source File

SOURCE=.\sdp_sess.h
# End Source File
# Begin Source File

SOURCE=.\sdp_ta.h
# End Source File
# Begin Source File

SOURCE=.\sdp_tb.h
# End Source File
# Begin Source File

SOURCE=.\sdp_tc.h
# End Source File
# Begin Source File

SOURCE=.\sdp_tk.h
# End Source File
# Begin Source File

SOURCE=.\sdp_tm.h
# End Source File
# Begin Source File

SOURCE=.\sdp_to.h
# End Source File
# Begin Source File

SOURCE=.\sdp_tok.h
# End Source File
# Begin Source File

SOURCE=.\sdp_tr.h
# End Source File
# Begin Source File

SOURCE=.\sdp_tses.h
# End Source File
# Begin Source File

SOURCE=.\sdp_tt.h
# End Source File
# Begin Source File

SOURCE=.\sdp_tz.h
# End Source File
# Begin Source File

SOURCE=.\sdp_utl.h
# End Source File
# End Group
# End Target
# End Project
