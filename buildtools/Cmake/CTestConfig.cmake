# Configure CTest. For details, see:
# http://www.cmake.org/Wiki/CMake_Testing_With_CTest#Customizing_CTest

SET (UPDATE_TYPE "svn")
SET (CTEST_CUSTOM_MAXIMUM_FAILED_TEST_OUTPUT_SIZE "3000000")
SET (CTEST_CUSTOM_MAXIMUM_PASSED_TEST_OUTPUT_SIZE "3000000")

#Get the hostname of current machine :
find_program(HOSTNAME_CMD NAMES hostname)
exec_program(${HOSTNAME_CMD} OUTPUT_VARIABLE HOSTNAME)
set(SITE "${HOSTNAME}")
MARK_AS_ADVANCED(HOSTNAME_CMD)

#Get the system information of current machine
find_program(UNAME NAMES uname)
macro(getuname name flag)
exec_program("${UNAME}" ARGS "${flag}" OUTPUT_VARIABLE "${name}")
endmacro(getuname)
MARK_AS_ADVANCED(UNAME)

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  SET(DISTRIB2 "OSX")
else(CMAKE_SYSTEM_NAME STREQUAL "Darwin")

  #Try to get the distrib
  find_program(CAT NAMES cat)
  exec_program("${CAT}" ARGS " /etc/issue" OUTPUT_VARIABLE DISTRIB)
  MARK_AS_ADVANCED(CAT)
  STRING(REPLACE "\\n \\l" "" DISTRIB2 ${DISTRIB})
endif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
         
getuname(osname -s)
getuname(node -n)
getuname(osrel  -r)
getuname(cpu    -m)
	
SET (BUILDNAME "${osname}-${DISTRIB2}-${cpu}")
SET (SITE "${node}")
SET (CTEST_PROJECT_NAME "Simgrid")
SET (CTEST_DROP_METHOD "http")
SET (CTEST_DROP_SITE "cdash.inria.fr/CDash")
SET (CTEST_DROP_LOCATION "/submit.php?project=Simgrid")
SET (CTEST_DROP_SITE_CDASH TRUE)
SET (CTEST_TRIGGER_SITE "http://cdash.inria.fr/CDash/cgi-bin/Submit-Random-TestingResults.cgi") 
       