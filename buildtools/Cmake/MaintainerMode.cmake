if(enable_maintainer_mode AND NOT WIN32)
find_program(FLEX_EXE NAMES flex)
find_program(FLEXML_EXE NAMES flexml)
find_program(SED_EXE NAMES sed)
find_program(PERL_EXE NAMES perl)

IF(FLEX_EXE)
	set(HAVE_FLEX 1)
ENDIF(FLEX_EXE)

IF(FLEXML_EXE)
	set(HAVE_FLEXML 1)
ENDIF(FLEXML_EXE)

if(HAVE_FLEXML AND HAVE_FLEX AND SED_EXE)
set(chaine1  "'s/extern  *\\([^ ]*[ \\*]*\\)/XBT_PUBLIC_DATA(\\1) /'")
set(chaine2  "'s/XBT_PUBLIC_DATA(\\([^)]*\\)) *\\([^(]*\\)(/XBT_PUBLIC(\\1) \\2(/'")
set(chaine3  "'s/extern  *\\([^ ]*[ \\*]*\\)/XBT_PUBLIC_DATA(\\1) /'")
set(chaine4  "'s/XBT_PUBLIC_DATA(\\([^)]*\\)) *\\([^(]*\\)(/XBT_PUBLIC(\\1) \\2(/'")
set(chaine5  "'s/SET(DOCTYPE)/SET(ROOT_dax__adag)/'")
set(chaine6  "'s/extern  *\\([^ ]*[ \\*]*\\)/XBT_PUBLIC_DATA(\\1) /'")
set(chaine7  "'s/XBT_PUBLIC_DATA(\\([^)]*\\)) *\\([^(]*\\)(/XBT_PUBLIC(\\1) \\2(/'")
set(chaine8  "'s/#if defined(_WIN32)/#if defined(_XBT_WIN32)/g'")
set(chaine9  "'s/#include <unistd.h>/#if defined(_XBT_WIN32) || defined(__WIN32__) || defined(WIN32) || defined(__TOS_WIN__)\\n#  ifndef __STRICT_ANSI__\\n#    include <io.h>\\n#    include <process.h>\\n#  endif\\n#else\\n#  include <unistd.h>\\n#endif/g'")
set(chaine10 "'s/#if defined(_WIN32)/#if defined(_XBT_WIN32)/g'")
set(chaine11 "'s/#include <unistd.h>/#if defined(_XBT_WIN32) || defined(__WIN32__) || defined(WIN32) || defined(__TOS_WIN__)\\n#  ifndef __STRICT_ANSI__\\n#    include <io.h>\\n#    include <process.h>\\n#  endif\\n#else\\n#  include <unistd.h>\\n#endif/g'")
set(chaine12 "'s/#if defined(_WIN32)/#if defined(_XBT_WIN32)/g'")
set(chaine13 "'s/#include <unistd.h>/#if defined(_XBT_WIN32) || defined(__WIN32__) || defined(WIN32) || defined(__TOS_WIN__)\\n#  ifndef __STRICT_ANSI__\\n#    include <io.h>\\n#    include <process.h>\\n#  endif\\n#else\\n#  include <unistd.h>\\n#endif/g'")

ADD_CUSTOM_COMMAND(
  	OUTPUT 	${PROJECT_DIRECTORY}/include/surf/simgrid_dtd.h
  			${PROJECT_DIRECTORY}/include/xbt/graphxml.h
  			${PROJECT_DIRECTORY}/src/simdag/dax_dtd.h
  			${PROJECT_DIRECTORY}/src/surf/simgrid_dtd.c
  			${PROJECT_DIRECTORY}/src/xbt/graphxml.c
  			${PROJECT_DIRECTORY}/src/simdag/dax_dtd.c
  			
  	DEPENDS	${PROJECT_DIRECTORY}/src/surf/simgrid.dtd
  			${PROJECT_DIRECTORY}/src/xbt/graphxml.dtd
  			${PROJECT_DIRECTORY}/src/simdag/dax.dtd
		  			
	#${PROJECT_DIRECTORY}/src/surf/simgrid_dtd.l: ${PROJECT_DIRECTORY}/src/surf/simgrid.dtd
	COMMAND ${FLEXML_EXE} --root-tags platform -b 1000000 -P surfxml --sysid=simgrid.dtd -S src/surf/simgrid_dtd.l -L src/surf/simgrid.dtd
	COMMAND ${CMAKE_COMMAND} -E echo "src/surf/simgrid_dtd.l"
	#${PROJECT_DIRECTORY}/src/xbt/graphxml.l: ${PROJECT_DIRECTORY}/src/xbt/graphxml.dtd
	COMMAND ${FLEXML_EXE} -b 1000000 -P graphxml --sysid=graphxml.dtd -S src/xbt/graphxml.l -L src/xbt/graphxml.dtd
	COMMAND ${CMAKE_COMMAND} -E echo "src/xbt/graphxml.l"
	#${PROJECT_DIRECTORY}/src/simdag/dax_dtd.l: ${PROJECT_DIRECTORY}/src/simdag/dax.dtd
	COMMAND ${FLEXML_EXE} -b 1000000 --root-tags adag -P dax_ --sysid=dax.dtd -S ${PROJECT_DIRECTORY}/src/simdag/dax_dtd.l -L ${PROJECT_DIRECTORY}/src/simdag/dax.dtd
	COMMAND ${SED_EXE} ${chaine5} -i src/simdag/dax_dtd.l
	COMMAND ${CMAKE_COMMAND} -E echo "src/simdag/dax_dtd.l"
	
	#${PROJECT_DIRECTORY}/include/surf/simgrid_dtd.h: ${PROJECT_DIRECTORY}/src/surf/simgrid.dtd
	COMMAND ${CMAKE_COMMAND} -E remove -f ${PROJECT_DIRECTORY}/include/surf/simgrid.h
	COMMAND ${FLEXML_EXE} --root-tags platform -P surfxml --sysid=simgrid.dtd -H include/surf/simgrid_dtd.h -L src/surf/simgrid.dtd
	COMMAND ${SED_EXE} ${chaine1} -i include/surf/simgrid_dtd.h
	COMMAND ${SED_EXE} ${chaine2} -i include/surf/simgrid_dtd.h	
	COMMAND ${CMAKE_COMMAND} -E echo "include/surf/simgrid_dtd.h"
	#${PROJECT_DIRECTORY}/include/xbt/graphxml.h: ${PROJECT_DIRECTORY}/src/xbt/graphxml.dtd
	COMMAND ${CMAKE_COMMAND} -E remove -f ${PROJECT_DIRECTORY}/include/xbt/graphxml.h
	COMMAND ${FLEXML_EXE} -P graphxml --sysid=graphxml.dtd -H include/xbt/graphxml.h -L src/xbt/graphxml.dtd
	COMMAND ${SED_EXE} ${chaine3} -i include/xbt/graphxml.h	
	COMMAND ${SED_EXE} ${chaine4} -i include/xbt/graphxml.h
	COMMAND ${CMAKE_COMMAND} -E echo "include/xbt/graphxml.h"
	#${PROJECT_DIRECTORY}/src/simdag/dax_dtd.h: ${PROJECT_DIRECTORY}/src/simdag/dax.dtd
	COMMAND ${CMAKE_COMMAND} -E remove -f ${PROJECT_DIRECTORY}/src/simdag/dax_dtd.h
	COMMAND ${FLEXML_EXE} --root-tags adag -P dax_ --sysid=dax.dtd -H src/simdag/dax_dtd.h -L src/simdag/dax.dtd
	COMMAND ${SED_EXE} ${chaine6} -i src/simdag/dax_dtd.h	
	COMMAND ${SED_EXE} ${chaine7} -i src/simdag/dax_dtd.h
	COMMAND ${FLEX_EXE} -o src/gras/DataDesc/ddt_parse.yy.c -Pgras_ddt_parse_ --noline src/gras/DataDesc/ddt_parse.yy.l
	COMMAND ${CMAKE_COMMAND} -E echo "src/simdag/dax_dtd.h"
	
	#surf/simgrid_dtd.c: surf/simgrid_dtd.l
	COMMAND ${CMAKE_COMMAND} -E remove -f ${PROJECT_DIRECTORY}/src/surf/simgrid_dtd.c
	COMMAND ${SED_EXE} ${chaine8} -i src/surf/simgrid_dtd.l
	COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_DIRECTORY}/src/surf
	COMMAND ${FLEX_EXE} -o src/surf/simgrid_dtd.c -Psurf_parse_ --noline src/surf/simgrid_dtd.l
	COMMAND ${SED_EXE} ${chaine9} -i src/surf/simgrid_dtd.c
	COMMAND ${CMAKE_COMMAND} -E echo "surf/simgrid_dtd.c"
	#xbt/graphxml.c: xbt/graphxml.l
	COMMAND ${CMAKE_COMMAND} -E remove -f ${PROJECT_DIRECTORY}/src/xbt/graphxml.c
	COMMAND ${SED_EXE} ${chaine10} -i src/xbt/graphxml.l	
	COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_DIRECTORY}/src/xbt
	COMMAND ${FLEX_EXE} -o src/xbt/graphxml.c -Pxbt_graph_parse_ --noline src/xbt/graphxml.l
	COMMAND ${SED_EXE} ${chaine11} -i src/xbt/graphxml.c
	COMMAND ${CMAKE_COMMAND} -E echo "xbt/graphxml.c"
	#simdag/dax_dtd.c: simdag/dax_dtd.l
	COMMAND ${CMAKE_COMMAND} -E remove -f ${PROJECT_DIRECTORY}/src/simdag/dax_dtd.c
	COMMAND ${SED_EXE} ${chaine12} -i src/simdag/dax_dtd.l
	COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_DIRECTORY}/src/simdag
	COMMAND ${FLEX_EXE} -o src/simdag/dax_dtd.c -Pdax_ --noline src/simdag/dax_dtd.l
	COMMAND ${SED_EXE} ${chaine13} -i src/simdag/dax_dtd.c
	COMMAND ${CMAKE_COMMAND} -E echo "simdag/dax_dtd.c"
	
	WORKING_DIRECTORY ${PROJECT_DIRECTORY}
	COMMENT "Generating files in maintainer mode..."
)

else(HAVE_FLEXML AND HAVE_FLEX  AND SED_EXE)
	message("  FLEX : 	${FLEX_EXE}")
	message("FLEXML : 	${FLEXML_EXE}")
	message("   SED : 	${SED_EXE}")
	message(FATAL_ERROR "Install flex or flexml or sed before use maintainer mode")
endif(HAVE_FLEXML AND HAVE_FLEX  AND SED_EXE)

if(PERL_EXE)
	
	ADD_CUSTOM_COMMAND(
  	OUTPUT	${PROJECT_DIRECTORY}/src/cunit_unit.c
			${PROJECT_DIRECTORY}/src/ex_unit.c
			${PROJECT_DIRECTORY}/src/dynar_unit.c
			${PROJECT_DIRECTORY}/src/dict_unit.c
			${PROJECT_DIRECTORY}/src/set_unit.c
			${PROJECT_DIRECTORY}/src/swag_unit.c
			${PROJECT_DIRECTORY}/src/xbt_str_unit.c
			${PROJECT_DIRECTORY}/src/xbt_strbuff_unit.c
			${PROJECT_DIRECTORY}/src/xbt_sha_unit.c
			${PROJECT_DIRECTORY}/src/config_unit.c
			${PROJECT_DIRECTORY}/src/xbt_synchro_unit.c
			
  	DEPENDS	${PROJECT_DIRECTORY}/tools/sg_unit_extractor.pl
  			${PROJECT_DIRECTORY}/src/xbt/cunit.c
			${PROJECT_DIRECTORY}/src/xbt/ex.c
			${PROJECT_DIRECTORY}/src/xbt/dynar.c
			${PROJECT_DIRECTORY}/src/xbt/dict.c
			${PROJECT_DIRECTORY}/src/xbt/set.c
			${PROJECT_DIRECTORY}/src/xbt/swag.c
			${PROJECT_DIRECTORY}/src/xbt/xbt_str.c
			${PROJECT_DIRECTORY}/src/xbt/xbt_strbuff.c
			${PROJECT_DIRECTORY}/src/xbt/xbt_sha.c
			${PROJECT_DIRECTORY}/src/xbt/config.c
			${PROJECT_DIRECTORY}/src/xbt/xbt_synchro.c
  	
	COMMAND	${CMAKE_COMMAND} -E remove -f ${PROJECT_DIRECTORY}/src/simgrid_units_main.c
	COMMAND	${CMAKE_COMMAND} -E remove -f ${PROJECT_DIRECTORY}/src/cunit_unit.c
	COMMAND	${CMAKE_COMMAND} -E remove -f ${PROJECT_DIRECTORY}/src/ex_unit.c
	COMMAND	${CMAKE_COMMAND} -E remove -f ${PROJECT_DIRECTORY}/src/dynar_unit.c
	COMMAND	${CMAKE_COMMAND} -E remove -f ${PROJECT_DIRECTORY}/src/dict_unit.c
	COMMAND	${CMAKE_COMMAND} -E remove -f ${PROJECT_DIRECTORY}/src/set_unit.c
	COMMAND	${CMAKE_COMMAND} -E remove -f ${PROJECT_DIRECTORY}/src/swag_unit.c
	COMMAND	${CMAKE_COMMAND} -E remove -f ${PROJECT_DIRECTORY}/src/xbt_str_unit.c
	COMMAND	${CMAKE_COMMAND} -E remove -f ${PROJECT_DIRECTORY}/src/xbt_strbuff_unit.c
	COMMAND	${CMAKE_COMMAND} -E remove -f ${PROJECT_DIRECTORY}/src/xbt_sha_unit.c
	COMMAND	${CMAKE_COMMAND} -E remove -f ${PROJECT_DIRECTORY}/src/config_unit.c
	COMMAND	${CMAKE_COMMAND} -E remove -f ${PROJECT_DIRECTORY}/src/xbt_synchro_unit.c
	
	COMMAND chmod a=rwx ${PROJECT_DIRECTORY}/tools/sg_unit_extractor.pl
	
	#$(TEST_UNITS): $(TEST_CFILES)
	COMMAND ${PROJECT_DIRECTORY}/tools/sg_unit_extractor.pl xbt/cunit.c
	COMMAND ${PROJECT_DIRECTORY}/tools/sg_unit_extractor.pl xbt/ex.c
	COMMAND ${PROJECT_DIRECTORY}/tools/sg_unit_extractor.pl xbt/dynar.c
	COMMAND ${PROJECT_DIRECTORY}/tools/sg_unit_extractor.pl xbt/dict.c
	COMMAND ${PROJECT_DIRECTORY}/tools/sg_unit_extractor.pl xbt/set.c
	COMMAND ${PROJECT_DIRECTORY}/tools/sg_unit_extractor.pl xbt/swag.c
	COMMAND ${PROJECT_DIRECTORY}/tools/sg_unit_extractor.pl xbt/xbt_str.c
	COMMAND ${PROJECT_DIRECTORY}/tools/sg_unit_extractor.pl xbt/xbt_strbuff.c
	COMMAND ${PROJECT_DIRECTORY}/tools/sg_unit_extractor.pl xbt/xbt_sha.c
	COMMAND ${PROJECT_DIRECTORY}/tools/sg_unit_extractor.pl xbt/config.c
	COMMAND ${PROJECT_DIRECTORY}/tools/sg_unit_extractor.pl xbt/xbt_synchro.c
	
	#@builddir@/simgrid_units_main.c: $(TEST_UNITS)
	COMMAND ${PROJECT_DIRECTORY}/tools/sg_unit_extractor.pl xbt/cunit.c
	
	WORKING_DIRECTORY ${PROJECT_DIRECTORY}/src
	
	COMMENT "Generating *_units files for testall..."
	)
	
	add_custom_target(units_files
						DEPENDS ${PROJECT_DIRECTORY}/src/cunit_unit.c
								${PROJECT_DIRECTORY}/src/ex_unit.c
								${PROJECT_DIRECTORY}/src/dynar_unit.c
								${PROJECT_DIRECTORY}/src/dict_unit.c
								${PROJECT_DIRECTORY}/src/set_unit.c
								${PROJECT_DIRECTORY}/src/swag_unit.c
								${PROJECT_DIRECTORY}/src/xbt_str_unit.c
								${PROJECT_DIRECTORY}/src/xbt_strbuff_unit.c
								${PROJECT_DIRECTORY}/src/xbt_sha_unit.c
								${PROJECT_DIRECTORY}/src/config_unit.c
								${PROJECT_DIRECTORY}/src/xbt_synchro_unit.c
						)
	
else(PERL_EXE)
	message(FATAL_ERROR "Install perl before use maintainer mode")
endif(PERL_EXE)

endif(enable_maintainer_mode AND NOT WIN32)

