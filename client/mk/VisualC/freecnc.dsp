# Microsoft Developer Studio Project File - Name="freecnc" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=freecnc - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "freecnc.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "freecnc.mak" CFG="freecnc - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "freecnc - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "freecnc - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "freecnc - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "sdl\include" /I "..\..\src\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x41d /d "NDEBUG"
# ADD RSC /l 0x41d /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /out:"..\..\freecnc.exe" /machine:I386

!ELSEIF  "$(CFG)" == "freecnc - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MD /Gm /GX /ZI /Od /I "sdl\include" /I "..\..\src\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x41d /d "_DEBUG"
# ADD RSC /l 0x41d /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /out:"..\..\freecnc.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "freecnc - Win32 Release"
# Name "freecnc - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "audio"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\audio\sound.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\audio\soundengine.cpp
# End Source File
# End Group
# Begin Group "game"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\game\actioneventqueue.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\game\aiplugman.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\game\dispatcher.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\game\game.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\game\loadmap.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\game\map.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\game\netconnection.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\game\path.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\game\playerpool.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\game\projectileanim.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\game\structure.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\game\structureanims.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\game\unit.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\game\unitandstructurepool.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\game\unitanimations.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\game\unitorstructure.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\game\unitqueue.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\game\weaponspool.cpp
# End Source File
# End Group
# Begin Group "misc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\misc\args.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\misc\common.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\misc\compression.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\misc\dllibrary.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\misc\fibheap.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\misc\inifile.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\misc\snprintf.c
# End Source File
# Begin Source File

SOURCE=..\..\src\misc\strcase.cpp
# End Source File
# End Group
# Begin Group "ui"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\ui\cursor.cpp

!IF  "$(CFG)" == "freecnc - Win32 Release"

!ELSEIF  "$(CFG)" == "freecnc - Win32 Debug"

# ADD CPP /MD

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\src\ui\cursorpool.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\ui\font.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\ui\input.cpp

!IF  "$(CFG)" == "freecnc - Win32 Release"

!ELSEIF  "$(CFG)" == "freecnc - Win32 Debug"

# ADD CPP /MD

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\src\ui\logger.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\ui\selection.cpp

!IF  "$(CFG)" == "freecnc - Win32 Release"

!ELSEIF  "$(CFG)" == "freecnc - Win32 Debug"

# ADD CPP /MD

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\src\ui\sidebar.cpp

!IF  "$(CFG)" == "freecnc - Win32 Release"

!ELSEIF  "$(CFG)" == "freecnc - Win32 Debug"

# ADD CPP /MD

!ENDIF 

# End Source File
# End Group
# Begin Group "video"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\video\cpsimage.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\video\graphicsengine.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\video\imagecache.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\video\imageproc.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\video\loadingscreen.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\video\message.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\video\shpimage.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\video\vqa.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\video\wsa.cpp
# End Source File
# End Group
# Begin Group "vfs"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\vfs\archive.h
# End Source File
# Begin Source File

SOURCE=..\..\src\vfs\externalvfs.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\vfs\externalvfs.h
# End Source File
# Begin Source File

SOURCE=..\..\src\vfs\vfs.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\vfs\vfsplugman.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\vfs\vfsplugman.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\src\freecnc.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\src\include\actioneventqueue.h
# End Source File
# Begin Source File

SOURCE=..\..\src\include\ai.h
# End Source File
# Begin Source File

SOURCE=..\..\src\include\aistubinterface.h
# End Source File
# Begin Source File

SOURCE=..\..\src\include\args.h
# End Source File
# Begin Source File

SOURCE=..\..\src\include\common.h
# End Source File
# Begin Source File

SOURCE=..\..\src\include\compression.h
# End Source File
# Begin Source File

SOURCE=..\..\src\include\crc.h
# End Source File
# Begin Source File

SOURCE=..\..\src\include\cursor.h
# End Source File
# Begin Source File

SOURCE=..\..\src\include\cursorpool.h
# End Source File
# Begin Source File

SOURCE=..\..\src\include\dllibrary.h
# End Source File
# Begin Source File

SOURCE=..\..\src\include\endian.h
# End Source File
# Begin Source File

SOURCE=..\..\src\include\fibheap.h
# End Source File
# Begin Source File

SOURCE=..\..\src\include\font.h
# End Source File
# Begin Source File

SOURCE=..\..\src\include\freecnc.h
# End Source File
# Begin Source File

SOURCE=..\..\src\include\game.h
# End Source File
# Begin Source File

SOURCE=..\..\src\include\graphicsengine.h
# End Source File
# Begin Source File

SOURCE=..\..\src\include\imagecache.h
# End Source File
# Begin Source File

SOURCE=..\..\src\include\imageproc.h
# End Source File
# Begin Source File

SOURCE=..\..\src\include\inifile.h
# End Source File
# Begin Source File

SOURCE=..\..\src\include\input.h
# End Source File
# Begin Source File

SOURCE=..\..\src\include\loadingscreen.h
# End Source File
# Begin Source File

SOURCE=..\..\src\include\logger.h
# End Source File
# Begin Source File

SOURCE=..\..\src\include\map.h
# End Source File
# Begin Source File

SOURCE=..\..\src\include\memtrack.h
# End Source File
# Begin Source File

SOURCE=..\..\src\include\message.h
# End Source File
# Begin Source File

SOURCE=..\..\src\include\path.h
# End Source File
# Begin Source File

SOURCE=..\..\src\include\playerpool.h
# End Source File
# Begin Source File

SOURCE=..\..\src\include\projectileanim.h
# End Source File
# Begin Source File

SOURCE=..\..\src\include\selection.h
# End Source File
# Begin Source File

SOURCE=..\..\src\include\shpimage.h
# End Source File
# Begin Source File

SOURCE=..\..\src\include\sidebar.h
# End Source File
# Begin Source File

SOURCE=..\..\src\include\sound.h
# End Source File
# Begin Source File

SOURCE=..\..\src\include\soundengine.h
# End Source File
# Begin Source File

SOURCE=..\..\src\include\structure.h
# End Source File
# Begin Source File

SOURCE=..\..\src\include\structureanims.h
# End Source File
# Begin Source File

SOURCE=..\..\src\include\timeline.h
# End Source File
# Begin Source File

SOURCE=..\..\src\include\unit.h
# End Source File
# Begin Source File

SOURCE=..\..\src\include\unitandstructurepool.h
# End Source File
# Begin Source File

SOURCE=..\..\src\include\unitanimations.h
# End Source File
# Begin Source File

SOURCE=..\..\src\include\unitorstructure.h
# End Source File
# Begin Source File

SOURCE=..\..\src\include\unitqueue.h
# End Source File
# Begin Source File

SOURCE=..\..\src\include\vfs.h
# End Source File
# Begin Source File

SOURCE=..\..\src\include\vqa.h
# End Source File
# Begin Source File

SOURCE=..\..\src\include\weaponspool.h
# End Source File
# Begin Source File

SOURCE=..\..\src\include\wsa.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE="C:\Program Files\Microsoft Visual Studio\VC98\Lib\SDLmain.lib"
# End Source File
# Begin Source File

SOURCE="C:\Program Files\Microsoft Visual Studio\VC98\Lib\SDL.lib"
# End Source File
# Begin Source File

SOURCE="C:\Program Files\Microsoft Visual Studio\VC98\Lib\SDL_net.lib"
# End Source File
# End Target
# End Project
