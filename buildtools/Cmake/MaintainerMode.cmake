if(enable_maintainer_mode AND NOT WIN32)
message("")
message("________________________________________________________________________________")
message("________________________________________________________________________________ FLEXMl")
find_program(FLEX_EXE NAMES flex)
find_program(FLEXML_EXE NAMES flexml)
find_program(SED_EXE NAMES sed)
find_program(PERL_EXE NAMES perl)

message("  FLEX : 	${FLEX_EXE}")
message("FLEXML : 	${FLEXML_EXE}")
message("   SED : 	${SED_EXE}\n")

set(top_srcdir "${PROJECT_DIRECTORY}")
set(srcdir "${PROJECT_DIRECTORY}/src")

IF(FLEX_EXE)
	set(HAVE_FLEX 1)
ENDIF(FLEX_EXE)

IF(FLEXML_EXE)
	set(HAVE_FLEXML 1)
ENDIF(FLEXML_EXE)

if(HAVE_FLEXML AND HAVE_FLEX AND SED_EXE)

#${PROJECT_DIRECTORY}/src/surf/simgrid_dtd.l: ${PROJECT_DIRECTORY}/src/surf/simgrid.dtd
exec_program("${CMAKE_COMMAND} -E make_directory ${PROJECT_DIRECTORY}/src/surf")
exec_program("${FLEXML_EXE} --root-tags platform -b 1000000 -P surfxml --sysid=simgrid.dtd -S ${PROJECT_DIRECTORY}/src/surf/simgrid_dtd.l -L ${PROJECT_DIRECTORY}/src/surf/simgrid.dtd"  "${PROJECT_DIRECTORY}/src/")

#$(PROJECT_DIRECTORY)/include/surf/simgrid_dtd.h: ${PROJECT_DIRECTORY}/src/surf/simgrid.dtd
file(REMOVE "${PROJECT_DIRECTORY}/include/surf/simgrid.h") 
exec_program("${FLEXML_EXE} --root-tags platform -P surfxml --sysid=simgrid.dtd -H ${PROJECT_DIRECTORY}/include/surf/simgrid_dtd.h -L ${PROJECT_DIRECTORY}/src/surf/simgrid.dtd" "${PROJECT_DIRECTORY}/src/")
set(CHAINE "'s/extern  *\\([^ ]*[ \\*]*\\)/XBT_PUBLIC_DATA(\\1) /'")
exec_program("${SED_EXE} ${CHAINE} -i ${PROJECT_DIRECTORY}/include/surf/simgrid_dtd.h" "${PROJECT_DIRECTORY}/src/")
set(CHAINE "'s/XBT_PUBLIC_DATA(\\([^)]*\\)) *\\([^(]*\\)(/XBT_PUBLIC(\\1) \\2(/'")	
exec_program("${SED_EXE} ${CHAINE} -i ${PROJECT_DIRECTORY}/include/surf/simgrid_dtd.h" "${PROJECT_DIRECTORY}/src/")

#${PROJECT_DIRECTORY}/src/xbt/graphxml.l: ${PROJECT_DIRECTORY}/src/xbt/graphxml.dtd
exec_program("${FLEXML_EXE} -b 1000000 -P graphxml --sysid=graphxml.dtd -S ${PROJECT_DIRECTORY}/src/xbt/graphxml.l -L ${PROJECT_DIRECTORY}/src/xbt/graphxml.dtd" "${PROJECT_DIRECTORY}/src/")

#$(PROJECT_DIRECTORY)/include/xbt/graphxml.h: ${PROJECT_DIRECTORY}/src/xbt/graphxml.dtd
exec_program("${FLEXML_EXE} -P graphxml --sysid=graphxml.dtd -H ${PROJECT_DIRECTORY}/include/xbt/graphxml.h -L ${PROJECT_DIRECTORY}/src/xbt/graphxml.dtd" "${PROJECT_DIRECTORY}/src/")
set(CHAINE "'s/extern  *\\([^ ]*[ \\*]*\\)/XBT_PUBLIC_DATA(\\1) /'")
exec_program("${SED_EXE} ${CHAINE} -i ${PROJECT_DIRECTORY}/include/xbt/graphxml.h" "${PROJECT_DIRECTORY}/src/")
set(CHAINE "'s/XBT_PUBLIC_DATA(\\([^)]*\\)) *\\([^(]*\\)(/XBT_PUBLIC(\\1) \\2(/'")
exec_program("${SED_EXE} ${CHAINE} -i ${PROJECT_DIRECTORY}/include/xbt/graphxml.h" "${PROJECT_DIRECTORY}/src/")

#${PROJECT_DIRECTORY}/src/simdag/dax_dtd.l: ${PROJECT_DIRECTORY}/src/simdag/dax.dtd
exec_program("${FLEXML_EXE} -b 1000000 --root-tags adag -P dax_ --sysid=dax.dtd -S ${PROJECT_DIRECTORY}/src/simdag/dax_dtd.l -L ${PROJECT_DIRECTORY}/src/simdag/dax.dtd" "${PROJECT_DIRECTORY}/src/")
set(CHAINE "'s/SET(DOCTYPE)/SET(ROOT_dax__adag)/'")
exec_program("${SED_EXE} -i ${CHAINE} ${PROJECT_DIRECTORY}/src/simdag/dax_dtd.l" "${PROJECT_DIRECTORY}/src/") # DOCTYPE not mandatory

#${PROJECT_DIRECTORY}/src/simdag/dax_dtd.h: ${PROJECT_DIRECTORY}/src/simdag/dax.dtd
exec_program("${FLEXML_EXE} --root-tags adag -P dax_ --sysid=dax.dtd -H ${PROJECT_DIRECTORY}/src/simdag/dax_dtd.h -L ${PROJECT_DIRECTORY}/src/simdag/dax.dtd" "${PROJECT_DIRECTORY}/src/")
set(CHAINE "'s/extern  *\\([^ ]*[ \\*]*\\)/XBT_PUBLIC_DATA(\\1) /'")
exec_program("${SED_EXE} ${CHAINE} -i ${PROJECT_DIRECTORY}/src/simdag/dax_dtd.h" "${PROJECT_DIRECTORY}/src/")
set(CHAINE "'s/XBT_PUBLIC_DATA(\\([^)]*\\)) *\\([^(]*\\)(/XBT_PUBLIC(\\1) \\2(/'")	
exec_program("${SED_EXE} ${CHAINE} -i ${PROJECT_DIRECTORY}/src/simdag/dax_dtd.h" "${PROJECT_DIRECTORY}/src/")

exec_program("${FLEX_EXE} -o ${PROJECT_DIRECTORY}/src/gras/DataDesc/ddt_parse.yy.c -Pgras_ddt_parse_ --noline ${PROJECT_DIRECTORY}/src/gras/DataDesc/ddt_parse.yy.l" "${PROJECT_DIRECTORY}/src/")

#surf/simgrid_dtd.c: surf/simgrid_dtd.l
exec_program("${CMAKE_COMMAND} -E make_directory ${PROJECT_DIRECTORY}/src/surf")
exec_program("${FLEX_EXE} -o ${PROJECT_DIRECTORY}/src/surf/simgrid_dtd.c -Psurf_parse_ --noline ${PROJECT_DIRECTORY}/src/surf/simgrid_dtd.l" "${PROJECT_DIRECTORY}/src/")
set(CHAINE "'s/#include <unistd.h>/#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32) || defined(__TOS_WIN__)\\n#  ifndef __STRICT_ANSI__\\n#    include <io.h>\\n#    include <process.h>\\n#  endif\\n#else\\n#  include <unistd.h>\\n#endif/g' -i ${PROJECT_DIRECTORY}/src/surf/simgrid_dtd.c")	
exec_program("${SED_EXE} ${CHAINE}" "${PROJECT_DIRECTORY}/src/")

#xbt/graphxml.c: xbt/graphxml.l
exec_program("${CMAKE_COMMAND} -E make_directory ${PROJECT_DIRECTORY}/src/xbt")
exec_program("${FLEX_EXE} -o ${PROJECT_DIRECTORY}/src/xbt/graphxml.c -Pxbt_graph_parse_ --noline ${PROJECT_DIRECTORY}/src/xbt/graphxml.l" "${PROJECT_DIRECTORY}/src/")
set(CHAINE "'s/#include <unistd.h>/#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32) || defined(__TOS_WIN__)\\n#  ifndef __STRICT_ANSI__\\n#    include <io.h>\\n#    include <process.h>\\n#  endif\\n#else\\n#  include <unistd.h>\\n#endif/g' -i ${PROJECT_DIRECTORY}/src/xbt/graphxml.c")			
exec_program("${SED_EXE} ${CHAINE}" "${PROJECT_DIRECTORY}/src/")

#simdag/dax_dtd.c: simdag/dax_dtd.l
exec_program("${CMAKE_COMMAND} -E make_directory ${PROJECT_DIRECTORY}/src/simdag")
exec_program("${FLEX_EXE} -o ${PROJECT_DIRECTORY}/src/simdag/dax_dtd.c -Pdax_ --noline ${PROJECT_DIRECTORY}/src/simdag/dax_dtd.l" "${PROJECT_DIRECTORY}/src/")
set(CHAINE "'s/#include <unistd.h>/#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32) || defined(__TOS_WIN__)\\n#  ifndef __STRICT_ANSI__\\n#    include <io.h>\\n#    include <process.h>\\n#  endif\\n#else\\n#  include <unistd.h>\\n#endif/g' -i ${PROJECT_DIRECTORY}/src/simdag/dax_dtd.c")	
exec_program("${SED_EXE} ${CHAINE}" "${PROJECT_DIRECTORY}/src/")

elseif(HAVE_FLEXML AND HAVE_FLEX  AND SED_EXE)
	message("  FLEX : 	${FLEX_EXE}")
	message("FLEXML : 	${FLEXML_EXE}")
	message("   SED : 	${SED_EXE}")
	message(FATAL_ERROR "Install flex or flexml or sed before use maintainer mode")
endif(HAVE_FLEXML AND HAVE_FLEX  AND SED_EXE)

#include $(PROJECT_DIRECTORY)/acmacro/dist-files.mk

message("")
message("________________________________________________________________________________")
message("________________________________________________________________________________ FLEXML END")

message("")
message("________________________________________________________________________________")
message("________________________________________________________________________________ SG_UNIT_EXTRACTOR")

if(PERL_EXE)
	message("   PERL : 	${PERL_EXE}\n")
	exec_program("${CMAKE_COMMAND} -E  remove -f ${PROJECT_DIRECTORY}/src/simgrid_units_main.c")
		
	foreach(file ${TEST_UNITS})
		exec_program("${CMAKE_COMMAND} -E remove ${PROJECT_DIRECTORY}/src/${file}")
	endforeach(file ${TEST_UNITS})

	#$(TEST_UNITS): $(TEST_CFILES)
	string(REPLACE ";" " " USE_TEST_CFILES "${TEST_CFILES}")
	exec_program("chmod a=rwx ${PROJECT_DIRECTORY}/tools/sg_unit_extractor.pl")
	exec_program("${PROJECT_DIRECTORY}/tools/sg_unit_extractor.pl ${USE_TEST_CFILES}")

	#@builddir@/simgrid_units_main.c: $(TEST_UNITS)
	exec_program("${PROJECT_DIRECTORY}/tools/sg_unit_extractor.pl src/xbt/cunit.c")

else(PERL_EXE)
	message(FATAL_ERROR "Install perl before use maintainer mode")
endif(PERL_EXE)
message("")
message("________________________________________________________________________________")
message("________________________________________________________________________________ SG_UNIT_EXTRACTOR END")

#Those lines permit to remake a cmake configure if "sources to look" have been changed

foreach(file ${SRC_TO_LOOK})
	configure_file(${PROJECT_DIRECTORY}/${file} ${PROJECT_DIRECTORY}/${file} COPYONLY)
endforeach(file ${SRC_TO_LOOK})

endif(enable_maintainer_mode AND NOT WIN32)
