# Find the libunwind library
#
#  LIBUNWIND_FOUND       - True if libunwind was found.
#  LIBUNWIND_LIBRARIES   - The libraries needed to use libunwind
#  LIBUNWIND_INCLUDE_DIR - Location of unwind.h and libunwind.h

# This file was downloaded from https://github.com/cmccabe/lksmith/blob/master/cmake_modules/FindLibunwind.cmake
# Copyright (c) 2011-2012, the Locksmith authors.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#
# Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Modified by the SimGrid team for:
#  - Survive the fact that libunwind cannot be found (setting LIBUNWIND_FOUND correctly)
#  - Also search for the unwind-ptrace component (but survive when it's not here)

set(LIBUNWIND_FOUND "1")

FIND_PATH(LIBUNWIND_INCLUDE_DIR libunwind.h)
if(NOT LIBUNWIND_INCLUDE_DIR)
  message(WARNING "failed to find libunwind.h")
  set(LIBUNWIND_FOUND "")
elif(NOT EXISTS "${LIBUNWIND_INCLUDE_DIR}/unwind.h")
  message(WARNING "libunwind.h was found, but unwind.h was not found in that directory.")
  set(LIBUNWIND_FOUND "")
  SET(LIBUNWIND_INCLUDE_DIR "")
endif()

FIND_LIBRARY(LIBUNWIND_GENERIC_LIBRARY "unwind")
if (LIBUNWIND_GENERIC_LIBRARY)
  MESSAGE(STATUS "Found libunwind library: ${LIBUNWIND_GENERIC_LIBRARY}")
else()
  set(LIBUNWIND_FOUND "")
  MESSAGE(WARNING "failed to find unwind generic library")
endif ()
SET(LIBUNWIND_LIBRARIES ${LIBUNWIND_GENERIC_LIBRARY})
unset(LIBUNWIND_GENERIC_LIBRARY)

FIND_LIBRARY(LIBUNWIND_PTRACE_LIBRARY "unwind-ptrace")
if (LIBUNWIND_PTRACE_LIBRARY)
  MESSAGE(STATUS "Found libunwind-ptrace library: ${LIBUNWIND_PTRACE_LIBRARY}")
  SET(LIBUNWIND_LIBRARIES ${LIBUNWIND_LIBRARIES} ${LIBUNWIND_PTRACE_LIBRARY})
else()
  MESSAGE(WARNING "Failed to find unwind-ptrace library. Proceeding anyway.")
endif ()
unset(LIBUNWIND_PTRACE_LIBRARY)

# For some reason, we have to link to two libunwind shared object files:
# one arch-specific and one not.
if (CMAKE_SYSTEM_PROCESSOR MATCHES "^arm")
    SET(LIBUNWIND_ARCH "arm")
elseif (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "amd64")
    SET(LIBUNWIND_ARCH "x86_64")
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "^i.86$")
    SET(LIBUNWIND_ARCH "x86")
endif()

if (LIBUNWIND_FOUND AND LIBUNWIND_ARCH)
    FIND_LIBRARY(LIBUNWIND_SPECIFIC_LIBRARY "unwind-${LIBUNWIND_ARCH}")
    if (LIBUNWIND_SPECIFIC_LIBRARY)
        MESSAGE(STATUS "Found libunwind-${LIBUNWIND_ARCH} library: ${LIBUNWIND_SPECIFIC_LIBRARY}")
    else()
        MESSAGE(FATAL_ERROR "failed to find unwind-${LIBUNWIND_ARCH}")
	set(LIBUNWIND_FOUND "")
    endif ()
    SET(LIBUNWIND_LIBRARIES ${LIBUNWIND_LIBRARIES} ${LIBUNWIND_SPECIFIC_LIBRARY})
    unset(LIBUNWIND_SPECIFIC_LIBRARY)
endif()

if (LIBUNWIND_FOUND)
  MESSAGE(STATUS "Found all required libunwind components.")
endif()

MARK_AS_ADVANCED(LIBUNWIND_LIBRARIES LIBUNWIND_INCLUDE_DIR)
