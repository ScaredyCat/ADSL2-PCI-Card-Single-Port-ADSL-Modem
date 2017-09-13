# Microsoft Developer Studio Project File - Name="cclmime" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=cclmime - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "cclmime.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "cclmime.mak" CFG="cclmime - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "cclmime - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "cclmime - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "cclmime - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\lib\Release"
# PROP Intermediate_Dir "..\temp\Release\cclmime"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "..\\" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "cclmime - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\lib\Debug"
# PROP Intermediate_Dir "..\temp\Debug\cclmime"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\\" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\lib\Debug\cclmimed.lib"

!ENDIF 

# Begin Target

# Name "cclmime - Win32 Release"
# Name "cclmime - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\address.c
# End Source File
# Begin Source File

SOURCE=.\body.c
# End Source File
# Begin Source File

SOURCE=.\file_stream.c
# End Source File
# Begin Source File

SOURCE=.\header.c
# End Source File
# Begin Source File

SOURCE=.\mapfile_stream.c
# End Source File
# Begin Source File

SOURCE=.\memory_stream.c
# End Source File
# Begin Source File

SOURCE=.\message.c
# End Source File
# Begin Source File

SOURCE=.\mime.c
# End Source File
# Begin Source File

SOURCE=.\mime_body.c
# End Source File
# Begin Source File

SOURCE=.\mime_hdr.c
# End Source File
# Begin Source File

SOURCE=.\mime_mem_strm.c
# End Source File
# Begin Source File

SOURCE=.\mime_mime.c
# End Source File
# Begin Source File

SOURCE=.\mime_msg.c
# End Source File
# Begin Source File

SOURCE=.\mime_strm.c
# End Source File
# Begin Source File

SOURCE=.\monitor.c
# End Source File
# Begin Source File

SOURCE=.\muerror.c
# End Source File
# Begin Source File

SOURCE=.\mutil.c
# End Source File
# Begin Source File

SOURCE=.\parse822.c
# End Source File
# Begin Source File

SOURCE=.\property.c
# End Source File
# Begin Source File

SOURCE=.\stream.c
# End Source File
# Begin Source File

SOURCE=.\tcp.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\address.h
# End Source File
# Begin Source File

SOURCE=.\body.h
# End Source File
# Begin Source File

SOURCE=.\header.h
# End Source File
# Begin Source File

SOURCE=.\message.h
# End Source File
# Begin Source File

SOURCE=.\mime.h
# End Source File
# Begin Source File

SOURCE=.\mime_body.h
# End Source File
# Begin Source File

SOURCE=.\mime_hdr.h
# End Source File
# Begin Source File

SOURCE=.\mime_mem_strm.h
# End Source File
# Begin Source File

SOURCE=.\mime_mime.h
# End Source File
# Begin Source File

SOURCE=.\mime_msg.h
# End Source File
# Begin Source File

SOURCE=.\mime_strm.h
# End Source File
# Begin Source File

SOURCE=.\monitor.h
# End Source File
# Begin Source File

SOURCE=.\muerror.h
# End Source File
# Begin Source File

SOURCE=.\mutil.h
# End Source File
# Begin Source File

SOURCE=.\parse822.h
# End Source File
# Begin Source File

SOURCE=.\property.h
# End Source File
# Begin Source File

SOURCE=.\stream.h
# End Source File
# End Group
# Begin Group "Docs"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Docs\mime_api.ppt
# End Source File
# End Group
# End Target
# End Project
