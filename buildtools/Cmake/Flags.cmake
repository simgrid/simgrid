set(warnCFLAGS "")
set(optCFLAGS "")

if(NOT __VISUALC__ AND NOT __BORLANDC__)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}-g3")
else(NOT __VISUALC__ AND NOT __BORLANDC__)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}/Zi")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}/Zi")
endif(NOT __VISUALC__ AND NOT __BORLANDC__)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${custom_flags}")

if(enable_supernovae)
	set(warnCFLAGS "-Wall -Wunused -Wmissing-prototypes -Wmissing-declarations -Wpointer-arith -Wchar-subscripts -Wcomment -Wformat -Wwrite-strings -Wno-unused-function -Wno-unused-parameter -Wno-strict-aliasing -Wno-format-nonliteral -Werror ")
	set(optCFLAGS "-O3 -finline-functions -funroll-loops -fno-strict-aliasing ")
endif(enable_supernovae)

if(enable_compile_warnings)
	set(warnCFLAGS "-Wall -Wunused -Wmissing-prototypes -Wmissing-declarations -Wpointer-arith -Wchar-subscripts -Wcomment -Wformat -Wwrite-strings -Wno-unused-function -Wno-unused-parameter -Wno-strict-aliasing -Wno-format-nonliteral -Werror ")
endif(enable_compile_warnings)

if(enable_compile_optimizations)
	set(optCFLAGS "-O3 -finline-functions -funroll-loops -fno-strict-aliasing ")
endif(enable_compile_optimizations)

set(CMAKE_C_FLAGS "${optCFLAGS}${warnCFLAGS}${CMAKE_C_FLAGS}")

if(enable_coverage)
	find_program(GCOV_PATH gcov)
	if(GCOV_PATH)
		SET(COVERAGE_COMMAND "${GCOV_PATH}" CACHE TYPE FILEPATH FORCE)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
		add_definitions(-fprofile-arcs -ftest-coverage)
		endif(GCOV_PATH)
endif(enable_coverage)

