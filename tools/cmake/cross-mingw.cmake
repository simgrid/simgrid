# cmake -DCMAKE_TOOLCHAIN_FILE=tools/cmake/cross-mingw.cmake ..

set (CMAKE_SYSTEM_NAME Windows)
set (CMAKE_SYSTEM_PROCESSOR x86_64)

set (CMAKE_C_COMPILER /usr/bin/x86_64-w64-mingw32-gcc-win32)
set (CMAKE_CXX_COMPILER /usr/bin/x86_64-w64-mingw32-g++-win32)
set (CMAKE_Fortran_COMPILER /usr/bin/x86_64-w64-mingw32-gfortran-win32)

set (CMAKE_FIND_ROOT_PATH /usr/lib/gcc/x86_64-w64-mingw32/ /usr/i686-w64-mingw32/)

set (CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set (CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set (CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
