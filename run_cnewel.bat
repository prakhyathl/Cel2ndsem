@echo off
SET PATH=C:\msys64\ucrt64\bin;C:\msys64\usr\bin;%PATH%
SET MSYSTEM=UCRT64
SET XDG_DATA_DIRS=C:\msys64\ucrt64\share
SET GDK_PIXBUF_MODULE_FILE=C:\msys64\ucrt64\lib\gdk-pixbuf-2.0\2.10.0\loaders.cache
SET GTK_EXE_PREFIX=C:\msys64\ucrt64
SET GTK_DATA_PREFIX=C:\msys64\ucrt64
SET GDK_BACKEND=win32

cd /d "%~dp0"
cnewel.exe
pause
