echo off
set CC=D:\Borland\BCC55\Bin\bcc32
set CXX=D:\Borland\BCC55\Bin\bcc32

set INCLUDE=D:\Borland\BCC55\Include
set LIB=D:\Borland\BCC55\Lib

set path=%path%;D:\Borland\BCC55;D:\Borland\BCC55\Bin;D:\Borland\BCC55\Lib;D:\Borland\BCC55\Include

set PATHEXT=%PATHEXT%;.OBJ

echo on

del CMakeCache.txt

cmake -G"Borland Makefiles" -Wno-dev