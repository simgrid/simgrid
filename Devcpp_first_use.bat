echo off
set CC=C:\Dev-Cpp\bin\gcc
set CXX=C:\Dev-Cpp\bin\g++

set INCLUDE=C:\Dev-Cpp\include
set LIB=C:\Dev-Cpp\lib

set Path=%Path%;C:\Dev-Cpp\bin

echo on

del CMakeCache.txt

cmake -G"Unix Makefiles" -Wno-dev -Denable_tracing=on -Denable_print_message=on .

make clean