# Change the following when we need a recent enough version of flexml to get the maintainer mode working
set(FLEXML_MIN_MAJOR 1)
set(FLEXML_MIN_MINOR 9)
set(FLEXML_MIN_PATCH 6)
# Change the following when we need a recent enough version of flex to get the maintainer mode working.
# Maintainer mode does NOT work with version 2.5.35!
set(FLEX_MIN_MAJOR 2)
set(FLEX_MIN_MINOR 5)
set(FLEX_MIN_PATCH 39)

# the rest should only be changed if you understand what you're doing
if(enable_maintainer_mode AND NOT WIN32)
  find_program(PYTHON_EXE NAMES python)
  mark_as_advanced(PYTHON_EXE)
  if (PYTHON_EXE)
    add_custom_command(
      OUTPUT
      ${CMAKE_HOME_DIRECTORY}/src/simix/popping_generated.cpp
      ${CMAKE_HOME_DIRECTORY}/src/simix/popping_bodies.cpp
      ${CMAKE_HOME_DIRECTORY}/src/simix/popping_enum.h
      ${CMAKE_HOME_DIRECTORY}/src/simix/popping_accessors.h

      DEPENDS
      ${CMAKE_HOME_DIRECTORY}/src/simix/simcalls.py
      ${CMAKE_HOME_DIRECTORY}/src/simix/simcalls.in

      COMMENT "Generating simcalls source files"
      COMMAND ${PYTHON_EXE} simcalls.py
      WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}/src/simix/
      )

    add_custom_target(simcalls_generated_src
      DEPENDS
      ${CMAKE_HOME_DIRECTORY}/src/simix/popping_generated.cpp
      ${CMAKE_HOME_DIRECTORY}/src/simix/popping_bodies.cpp
      ${CMAKE_HOME_DIRECTORY}/src/simix/popping_enum.h
      ${CMAKE_HOME_DIRECTORY}/src/simix/popping_accessors.h
      )

    SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES
      "${CMAKE_HOME_DIRECTORY}/src/simix/popping_enum.h;${CMAKE_HOME_DIRECTORY}/src/simix/popping_generated.cpp;${CMAKE_HOME_DIRECTORY}/src/simix/popping_bodies.cpp;${CMAKE_HOME_DIRECTORY}/src/simix/popping_accessors.h"
      )
  endif()
endif()

if(enable_maintainer_mode AND NOT WIN32)
  find_program(FLEX_EXE NAMES flex)
  find_program(FLEXML_EXE NAMES flexml)
  find_program(SED_EXE NAMES sed)
  find_program(BISON_EXE NAMES bison)
  find_program(LEX_EXE NAMES lex)

  mark_as_advanced(BISON_EXE)
  mark_as_advanced(LEX_EXE)

  if(BISON_EXE AND LEX_EXE)
    add_custom_command(
      OUTPUT
      ${CMAKE_HOME_DIRECTORY}/src/xbt/automaton/automaton_lexer.yy.c
      ${CMAKE_HOME_DIRECTORY}/src/xbt/automaton/parserPromela.tab.cacc
      ${CMAKE_HOME_DIRECTORY}/src/xbt/automaton/parserPromela.tab.hacc

      DEPENDS
      ${CMAKE_HOME_DIRECTORY}/src/xbt/automaton/parserPromela.lex
      ${CMAKE_HOME_DIRECTORY}/src/xbt/automaton/parserPromela.yacc

      COMMENT "Generating automaton source files"
      COMMAND ${BISON_EXE} --name-prefix=xbt_automaton_parser_ -d parserPromela.yacc
      COMMAND ${LEX_EXE} --prefix=xbt_automaton_parser_ --outfile=automaton_lexer.yy.c parserPromela.lex
      WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}/src/xbt/automaton/
      )

    add_custom_target(automaton_generated_src
      DEPENDS
      ${CMAKE_HOME_DIRECTORY}/src/xbt/automaton/automaton_lexer.yy.c
      ${CMAKE_HOME_DIRECTORY}/src/xbt/automaton/parserPromela.tab.cacc
      ${CMAKE_HOME_DIRECTORY}/src/xbt/automaton/parserPromela.tab.hacc
      )

    SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES
      "${CMAKE_HOME_DIRECTORY}/src/xbt/automaton/parserPromela.tab.cacc;${CMAKE_HOME_DIRECTORY}/src/xbt/automaton/parserPromela.tab.hacc;${CMAKE_HOME_DIRECTORY}/src/xbt/automaton/automaton_parse.yy.c"
      )
  endif()

  message(STATUS "Found flex: ${FLEX_EXE}")
  IF(FLEX_EXE)
    set(HAVE_FLEX 1)
    execute_process(COMMAND ${FLEX_EXE} --version OUTPUT_VARIABLE FLEX_VERSION)
    string(REGEX MATCH "[0-9]+[.]+[0-9]+[.]+[0-9]+" FLEX_VERSION "${FLEX_VERSION}")
    string(REGEX MATCH "^[0-9]+" FLEX_MAJOR_VERSION "${FLEX_VERSION}")
    string(REGEX MATCH "[0-9]+[.]+[0-9]+$" FLEX_VERSION "${FLEX_VERSION}")
    string(REGEX MATCH "^[0-9]+" FLEX_MINOR_VERSION "${FLEX_VERSION}")
    string(REGEX MATCH "[0-9]+$" FLEX_PATCH_VERSION "${FLEX_VERSION}")
  ENDIF()

  message(STATUS "Found flexml: ${FLEXML_EXE}")
  IF(FLEXML_EXE)
    set(HAVE_FLEXML 1)
    execute_process(COMMAND ${FLEXML_EXE} --version OUTPUT_VARIABLE FLEXML_VERSION)
    if (FLEXML_VERSION MATCHES "version Id:")
      message(FATAL_ERROR "You have an ancient flexml version (${FLEXML_VERSION}). You need at least v${FLEXML_MIN_MAJOR}.${FLEXML_MIN_MINOR}.${FLEXML_MIN_PATCH} to compile in maintainer mode. Upgrade your flexml, or disable the Maintainer mode option in cmake.")
    endif()

    string(REGEX MATCH "[0-9]+[.]+[0-9]+[.]+[0-9]+" FLEXML_VERSION "${FLEXML_VERSION}")
    string(REGEX MATCH "^[0-9]*" FLEXML_MAJOR_VERSION "${FLEXML_VERSION}")
    string(REGEX MATCH "[0-9]+[.]+[0-9]+$" FLEXML_VERSION "${FLEXML_VERSION}")
    string(REGEX MATCH "^[0-9]+" FLEXML_MINOR_VERSION "${FLEXML_VERSION}")
    string(REGEX MATCH "[0-9]+$" FLEXML_PATCH_VERSION "${FLEXML_VERSION}")
  ENDIF()

  message(STATUS "Found sed: ${SED_EXE}")

  if(HAVE_FLEXML AND HAVE_FLEX AND SED_EXE)

    message(STATUS "Flex version: ${FLEX_MAJOR_VERSION}.${FLEX_MINOR_VERSION}.${FLEX_PATCH_VERSION} (need at least version ${FLEX_MIN_MAJOR}.${FLEX_MIN_MINOR}.${FLEX_MIN_PATCH})")
    message(STATUS "Flexml version: ${FLEXML_MAJOR_VERSION}.${FLEXML_MINOR_VERSION}.${FLEXML_PATCH_VERSION} (need at least version ${FLEXML_MIN_MAJOR}.${FLEXML_MIN_MINOR}.${FLEXML_MIN_PATCH})")

    IF(     (${FLEXML_MAJOR_VERSION} LESS ${FLEXML_MIN_MAJOR})
        OR ((${FLEXML_MAJOR_VERSION} EQUAL ${FLEXML_MIN_MAJOR}) AND (${FLEXML_MINOR_VERSION} LESS ${FLEXML_MIN_MINOR}) )
        OR (    (${FLEXML_MAJOR_VERSION} EQUAL ${FLEXML_MIN_MAJOR})
	    AND (${FLEXML_MINOR_VERSION} EQUAL ${FLEXML_MIN_MINOR})
	    AND (${FLEXML_PATCH_VERSION} LESS ${FLEXML_MIN_PATCH}) ))

      message(FATAL_ERROR "Your flexml version is too old to compile in maintainer mode (need at least v${FLEXML_MIN_MAJOR}.${FLEXML_MIN_MINOR}.${FLEXML_MIN_PATCH}). Upgrade your flexml, or disable the Maintainer mode option in cmake.")

    ENDIF()

    IF(     (${FLEX_MAJOR_VERSION} LESS ${FLEX_MIN_MAJOR})
        OR ((${FLEX_MAJOR_VERSION} EQUAL ${FLEX_MIN_MAJOR}) AND (${FLEX_MINOR_VERSION} LESS ${FLEX_MIN_MINOR}) )
        OR (    (${FLEX_MAJOR_VERSION} EQUAL ${FLEX_MIN_MAJOR})
	    AND (${FLEX_MINOR_VERSION} EQUAL ${FLEX_MIN_MINOR})
	    AND (${FLEX_PATCH_VERSION} LESS ${FLEX_MIN_PATCH}) ))

        message(FATAL_ERROR "Your flex version is too old to compile in maintainer mode (need at least v${FLEX_MIN_MAJOR}.${FLEX_MIN_MINOR}.${FLEX_MIN_PATCH}). Upgrade your flex, or disable the Maintainer mode option in cmake (run 'ccmake').")

    ENDIF()

    set(string1  "'s/extern *\\([^(]*\\)\\( \\|\\( \\*\\)\\)/XBT_PUBLIC_DATA(\\1\\3) /'")
    set(string2  "'s/XBT_PUBLIC_DATA(\\([^)]*\\)) *\\([^(]*\\)(/XBT_PUBLIC(\\1) \\2(/'")
    set(string5  "'s/SET(DOCTYPE)/SET(ROOT_dax__adag)/'")
    set(string9  "'s/#include <unistd.h>/#if defined(_WIN32)\\n#  ifndef __STRICT_ANSI__\\n#    include <io.h>\\n#    include <process.h>\\n#  endif\\n#else\\n#  include <unistd.h>\\n#endif/g'")
    set(string14 "'\\!^ \\* Generated [0-9/]\\{10\\} [0-9:]\\{8\\}\\.$$!d'")
    set(string15 "'s/FAIL(\"Premature EOF/if(!ETag_surfxml_include_state()) FAIL(\"Premature EOF/'")

    ADD_CUSTOM_COMMAND(
      OUTPUT 	${CMAKE_HOME_DIRECTORY}/src/surf/xml/simgrid_dtd.h
                ${CMAKE_HOME_DIRECTORY}/src/simdag/dax_dtd.h
                ${CMAKE_HOME_DIRECTORY}/src/surf/xml/simgrid_dtd.c
                ${CMAKE_HOME_DIRECTORY}/src/simdag/dax_dtd.c

      DEPENDS	${CMAKE_HOME_DIRECTORY}/src/surf/xml/simgrid.dtd
                ${CMAKE_HOME_DIRECTORY}/src/simdag/dax.dtd

      #${CMAKE_HOME_DIRECTORY}/src/surf/xml/simgrid_dtd.l: ${CMAKE_HOME_DIRECTORY}/src/surf/xml/simgrid.dtd
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_HOME_DIRECTORY}/src/surf/xml
      COMMAND ${FLEXML_EXE} --root-tags platform -b 1000000 -P surfxml --sysid=http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd -S src/surf/xml/simgrid_dtd.l -L src/surf/xml/simgrid.dtd
      COMMAND ${SED_EXE} -i ${string14} src/surf/xml/simgrid_dtd.l
      COMMAND ${SED_EXE} -i "'s/FAIL(\"Bad declaration %s.\",yytext)/FAIL(\"Bad declaration %s.\\\\nIf your are using a XML v3 file (check the version attribute in <platform>), please update it with tools\\/simgrid_update_xml.pl\",yytext)/'" src/surf/xml/simgrid_dtd.l
      COMMAND ${CMAKE_COMMAND} -E echo "       Generated src/surf/xml/simgrid_dtd.l"

      #${CMAKE_HOME_DIRECTORY}/src/simdag/dax_dtd.l: ${CMAKE_HOME_DIRECTORY}/src/simdag/dax.dtd
      COMMAND ${FLEXML_EXE} -b 1000000 --root-tags adag -P dax_ --sysid=dax.dtd -S src/simdag/dax_dtd.l -L src/simdag/dax.dtd
      COMMAND ${SED_EXE} -i ${string5} src/simdag/dax_dtd.l
      COMMAND ${SED_EXE} -i ${string14} src/simdag/dax_dtd.l
      COMMAND ${CMAKE_COMMAND} -E echo "       Generated src/simdag/dax_dtd.l"

      #${CMAKE_HOME_DIRECTORY}/src/surf/xml/simgrid_dtd.h: ${CMAKE_HOME_DIRECTORY}/src/surf/xml/simgrid.dtd
      COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_HOME_DIRECTORY}/include/surf/simgrid.h
      COMMAND ${FLEXML_EXE} --root-tags platform -P surfxml --sysid=http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd -H src/surf/xml/simgrid_dtd.h -L src/surf/xml/simgrid.dtd
      COMMAND ${SED_EXE} -i ${string1} src/surf/xml/simgrid_dtd.h
      COMMAND ${SED_EXE} -i ${string2} src/surf/xml/simgrid_dtd.h
      COMMAND ${SED_EXE} -i ${string14} src/surf/xml/simgrid_dtd.h
      COMMAND ${CMAKE_COMMAND} -E echo "       Generated src/surf/xml/simgrid_dtd.h"

      #${CMAKE_HOME_DIRECTORY}/src/simdag/dax_dtd.h: ${CMAKE_HOME_DIRECTORY}/src/simdag/dax.dtd
      COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_HOME_DIRECTORY}/src/simdag/dax_dtd.h
      COMMAND ${FLEXML_EXE} --root-tags adag -P dax_ --sysid=dax.dtd -H src/simdag/dax_dtd.h -L src/simdag/dax.dtd
      COMMAND ${SED_EXE} -i ${string1} src/simdag/dax_dtd.h
      COMMAND ${SED_EXE} -i ${string2} src/simdag/dax_dtd.h
      COMMAND ${SED_EXE} -i ${string14} src/simdag/dax_dtd.h
      COMMAND ${CMAKE_COMMAND} -E echo "       Generated src/simdag/dax_dtd.h"

      #surf/xml/simgrid_dtd.c: surf/xml/simgrid_dtd.l
      COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_HOME_DIRECTORY}/src/surf/xml/simgrid_dtd.c
      COMMAND ${FLEX_EXE} -o src/surf/xml/simgrid_dtd.c -Psurf_parse_ --noline src/surf/xml/simgrid_dtd.l
      COMMAND ${SED_EXE} -i ${string9} src/surf/xml/simgrid_dtd.c
      COMMAND ${SED_EXE} -i ${string15} src/surf/xml/simgrid_dtd.c
      COMMAND ${SED_EXE} -i 's/int yyl\;/unsigned int yyl\;/' src/surf/xml/simgrid_dtd.c
      COMMAND ${SED_EXE} -i "s/register //" src/surf/xml/simgrid_dtd.c
      COMMAND ${CMAKE_COMMAND} -E echo "       Generated surf/xml/simgrid_dtd.c"

      #simdag/dax_dtd.c: simdag/dax_dtd.l
      COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_HOME_DIRECTORY}/src/simdag/dax_dtd.c
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_HOME_DIRECTORY}/src/simdag
      COMMAND ${FLEX_EXE} -o src/simdag/dax_dtd.c -Pdax_ --noline src/simdag/dax_dtd.l
      COMMAND ${SED_EXE} -i ${string9}                        src/simdag/dax_dtd.c
      COMMAND ${SED_EXE} -i 's/int yyl\;/unsigned int yyl\;/' src/simdag/dax_dtd.c
      COMMAND ${CMAKE_COMMAND} -E echo "       Generated src/simdag/dax_dtd.c"

      WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}
      COMMENT "Generating files in maintainer mode..."
      )

  else()
    if(NOT HAVE_FLEXML)
      message(STATUS "Error : Install flexml to use the maintainer mode.")
    endif()
    if(NOT HAVE_FLEX)
      message(STATUS "Error : Install flex to use the maintainer mode.")
    endif()
    if(NOT SED_EXE)
      message(STATUS "Error : Install sed before use maintainer mode.")
    endif()

    message(FATAL_ERROR STATUS "Error : Need to install all tools for maintainer mode !!!")
  endif()

endif()

    add_custom_target(maintainer_files
      DEPENDS ${CMAKE_HOME_DIRECTORY}/src/surf/xml/simgrid_dtd.h
              ${CMAKE_HOME_DIRECTORY}/src/surf/xml/simgrid_dtd.c
              ${CMAKE_HOME_DIRECTORY}/src/simdag/dax_dtd.h
              ${CMAKE_HOME_DIRECTORY}/src/simdag/dax_dtd.c
      )
