set(warnCFLAGS "")
set(optCFLAGS "")

if(NOT WIN32)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}-g3 ${custom_flags}")
endif(NOT WIN32)

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
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage")
endif(enable_coverage)

