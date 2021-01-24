###
### Generate all parts of the documentation on non-Windows systems
###
###   - HTML with doxygen (reference and manual)
###   - Javadoc (reference)
###   - manpages (reference of tools)
###
###  This file is not loaded on windows

#### Generate the html documentation
if (enable_documentation)
  find_package(Doxygen REQUIRED)
else()
  find_package(Doxygen)
endif()

find_path(FIG2DEV_PATH  NAMES fig2dev  PATHS NO_DEFAULT_PATHS)

if(enable_documentation)
  ADD_CUSTOM_TARGET(documentation
    COMMENT "Generating the SimGrid documentation..."
    DEPENDS ${DOC_SOURCES} ${source_doxygen}
    COMMAND ${CMAKE_COMMAND} -E make_directory   ${CMAKE_BINARY_DIR}/doc/doxygen
    COMMAND ${CMAKE_COMMAND} -E make_directory   ${CMAKE_BINARY_DIR}/doc/example_lists
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/doc/html
    COMMAND ${CMAKE_COMMAND} -E make_directory   ${CMAKE_BINARY_DIR}/doc/html
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/doc/xml
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/docs/source/api
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/doc
  )

  message(STATUS "Doxygen version: ${DOXYGEN_VERSION}")

  # This is a workaround for older cmake versions (such as 2.8.7 on Ubuntu 12.04). These cmake versions do not provide
  # the DOXYGEN_VERSION variable and hence, building the documentation will always  fail. This code is the same as used
  # in the cmake library, version 3.
  if(DOXYGEN_EXECUTABLE)
    execute_process(COMMAND ${DOXYGEN_EXECUTABLE} "--version" OUTPUT_VARIABLE DOXYGEN_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
  endif()

  if(DOXYGEN_VERSION VERSION_LESS "1.8")
    ADD_CUSTOM_TARGET(error_doxygen
        COMMAND ${CMAKE_COMMAND} -E echo "Doxygen must be at least version 1.8 to generate documentation. Version found: ${DOXYGEN_VERSION}"
      COMMAND false
    )
    add_dependencies(documentation error_doxygen)
  endif()

  configure_file(${CMAKE_HOME_DIRECTORY}/doc/Doxyfile.in ${CMAKE_BINARY_DIR}/doc/Doxyfile @ONLY)

  foreach(file ${DOC_IMG})
    ADD_CUSTOM_COMMAND(TARGET documentation COMMAND ${CMAKE_COMMAND} -E copy ${file} ${CMAKE_BINARY_DIR}/doc/html/)
  endforeach()

  ADD_CUSTOM_COMMAND(TARGET documentation
    COMMAND ${FIG2DEV_PATH}/fig2dev -Lmap ${CMAKE_HOME_DIRECTORY}/doc/shared/fig/simgrid_modules.fig | perl -pe 's/imagemap/simgrid_modules/g'| perl -pe 's/<IMG/<IMG style=border:0px/g' | ${CMAKE_HOME_DIRECTORY}/tools/doxygen/fig2dev_postprocessor.pl > ${CMAKE_BINARY_DIR}/doc/simgrid_modules.map
    COMMAND pwd
    COMMAND ${CMAKE_COMMAND} -E echo "XX Generate list of files in examples/ for routing models"
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/doc/example_lists/
    COMMAND ${CMAKE_HOME_DIRECTORY}/tools/doxygen/list_routing_models_examples.sh Floyd > ${CMAKE_BINARY_DIR}/doc/example_lists/example_filelist_routing_floyd
    COMMAND ${CMAKE_HOME_DIRECTORY}/tools/doxygen/list_routing_models_examples.sh Dijkstra > ${CMAKE_BINARY_DIR}/doc/example_lists/example_filelist_routing_dijkstra
    COMMAND ${CMAKE_HOME_DIRECTORY}/tools/doxygen/list_routing_models_examples.sh DijkstraCache > ${CMAKE_BINARY_DIR}/doc/example_lists/example_filelist_routing_dijkstra_cache
    COMMAND ${CMAKE_HOME_DIRECTORY}/tools/doxygen/list_routing_models_examples.sh 'routing="None"' > ${CMAKE_BINARY_DIR}/doc/example_lists/example_filelist_routing_none
    COMMAND ${CMAKE_HOME_DIRECTORY}/tools/doxygen/list_routing_models_examples.sh 'routing="Cluster"' > ${CMAKE_BINARY_DIR}/doc/example_lists/example_filelist_routing_cluster
    COMMAND ${CMAKE_HOME_DIRECTORY}/tools/doxygen/list_routing_models_examples.sh 'routing="Vivaldi"' > ${CMAKE_BINARY_DIR}/doc/example_lists/example_filelist_routing_vivaldi
    COMMAND ${CMAKE_HOME_DIRECTORY}/tools/doxygen/list_routing_models_examples.sh 'routing="Full"' > ${CMAKE_BINARY_DIR}/doc/example_lists/example_filelist_routing_full
    COMMAND ${CMAKE_COMMAND} -E echo "XX Generate list of files in examples/ for XML tags"
    COMMAND ${CMAKE_HOME_DIRECTORY}/tools/doxygen/list_routing_models_examples.sh '<mount ' > ${CMAKE_BINARY_DIR}/doc/example_lists/example_filelist_xmltag_mount
    COMMAND ${CMAKE_HOME_DIRECTORY}/tools/doxygen/list_routing_models_examples.sh '<link_ctn ' > ${CMAKE_BINARY_DIR}/doc/example_lists/example_filelist_xmltag_linkctn
    COMMAND ${CMAKE_COMMAND} -E echo "XX Run doxygen"
    COMMAND ${DOXYGEN_EXECUTABLE} Doxyfile
    COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_BINARY_DIR}/doc/simgrid_modules.map
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/doc
  )

  ### Fill in the "make gforge-sync" target ###

  set(RSYNC_CMD rsync --verbose --cvs-exclude --compress --delete --delete-excluded --rsh=ssh --ignore-times --recursive --links --times --omit-dir-times --perms --chmod=a+rX,ug+w,o-w,Dg+s)

  add_custom_target(gforge-sync
    COMMAND ssh scm.gforge.inria.fr mkdir -p -m 2775 /home/groups/simgrid/htdocs/simgrid/${release_version}/ || true

    COMMAND ${RSYNC_CMD} doc/html/ scm.gforge.inria.fr:/home/groups/simgrid/htdocs/simgrid/${release_version}/doc/ || true

    COMMAND ${RSYNC_CMD} doc/html/simgrid_modules2.png doc/html/simgrid_modules.png /${CMAKE_HOME_DIRECTORY}/doc/webcruft/simgrid_logo_2011.png
    /${CMAKE_HOME_DIRECTORY}/doc/webcruft/simgrid_logo_2011_small.png scm.gforge.inria.fr:/home/groups/simgrid/htdocs/simgrid/${release_version}/

    COMMAND ${RSYNC_CMD} ${CMAKE_HOME_DIRECTORY}/src/surf/xml/simgrid.dtd scm.gforge.inria.fr:/home/groups/simgrid/htdocs/simgrid/
    COMMAND ${RSYNC_CMD} ${CMAKE_HOME_DIRECTORY}/src/surf/xml/simgrid.dtd scm.gforge.inria.fr:/home/groups/simgrid/htdocs/simgrid/${release_version}/simgrid.dtd

    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  )
  add_dependencies(gforge-sync documentation)

  if (Java_FOUND)
    find_path(JAVADOC_PATH  NAMES javadoc   PATHS NO_DEFAULT_PATHS)
    mark_as_advanced(JAVADOC_PATH)

    ADD_CUSTOM_COMMAND(TARGET documentation
      COMMAND ${CMAKE_COMMAND} -E echo "XX Javadoc pass"
      COMMAND ${JAVADOC_PATH}/javadoc -quiet -d ${CMAKE_BINARY_DIR}/doc/html/javadoc/ ${CMAKE_HOME_DIRECTORY}/src/bindings/java/org/simgrid/*.java ${CMAKE_HOME_DIRECTORY}/src/bindings/java/org/simgrid/*/*.java
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/doc
    )
  endif()

  #### Generate the manpages
  if( NOT MANPAGE_DIR)
    set( MANPAGE_DIR ${CMAKE_BINARY_DIR}/manpages )
  endif()

  add_custom_target(manpages ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${MANPAGE_DIR}
    COMMAND pod2man ${CMAKE_HOME_DIRECTORY}/tools/simgrid_update_xml.pl > ${MANPAGE_DIR}/simgrid_update_xml.1
    COMMAND pod2man ${CMAKE_HOME_DIRECTORY}/docs/manpages/tesh.pod > ${MANPAGE_DIR}/tesh.1
    COMMENT "Generating manpages"
  )
  install(FILES
    ${MANPAGE_DIR}/simgrid_update_xml.1
    ${MANPAGE_DIR}/tesh.1
    ${CMAKE_HOME_DIRECTORY}/docs/manpages/smpicc.1
    ${CMAKE_HOME_DIRECTORY}/docs/manpages/smpicxx.1
    ${CMAKE_HOME_DIRECTORY}/docs/manpages/smpif90.1
    ${CMAKE_HOME_DIRECTORY}/docs/manpages/smpiff.1
    ${CMAKE_HOME_DIRECTORY}/docs/manpages/smpirun.1
    DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
  )

else(enable_documentation)
  ADD_CUSTOM_TARGET(documentation
    COMMENT "The generation of the SimGrid documentation was disabled in cmake"
  )
endif(enable_documentation)
