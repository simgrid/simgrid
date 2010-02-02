# Configure CTest. For details, see:
# http://www.cmake.org/Wiki/CMake_Testing_With_CTest#Customizing_CTest

#Get the hostname of current machine :
exec_program(${HOSTNAME_CMD} OUTPUT_VARIABLE HOSTNAME)
set(SITE "${HOSTNAME}")
MARK_AS_ADVANCED(HOSTNAME_CMD)

#Get the system information of current machine
macro(getuname name flag)
exec_program("${UNAME}" ARGS "${flag}" OUTPUT_VARIABLE "${name}")
endmacro(getuname)
MARK_AS_ADVANCED(UNAME)

exec_program("${CAT}" ARGS "version" OUTPUT_VARIABLE VERSION)

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  SET(DISTRIB2 "OSX")
else(CMAKE_SYSTEM_NAME STREQUAL "Darwin")

  #Try to get the distrib
  exec_program("${CAT}" ARGS " /etc/issue" OUTPUT_VARIABLE DISTRIB)
  MARK_AS_ADVANCED(CAT)
endif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")

STRING(REPLACE " \\n \\l" "" DISTRIB2 ${DISTRIB})
         
getuname(osname -s)
getuname(node -n)
getuname(osrel  -r)
getuname(cpu    -m)
	
SET(BUILDNAME "${osname}_${DISTRIB2}_${cpu}")
SET(CTEST_SITE "${node}")
SET(CTEST_PROJECT_NAME "${PROJECT_NAME}")
SET(CTEST_CUSTOM_MAXIMUM_FAILED_TEST_OUTPUT_SIZE "3000000")
SET(CTEST_CUSTOM_MAXIMUM_PASSED_TEST_OUTPUT_SIZE "3000000")
