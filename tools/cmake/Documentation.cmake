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

if(DOXYGEN_FOUND)
  ADD_CUSTOM_TARGET(documentation 
    COMMENT "Generating the SimGrid documentation..."
    DEPENDS ${DOC_SOURCES} ${DOC_FIGS} ${source_doxygen}
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_HOME_DIRECTORY}/doc/html
    COMMAND ${CMAKE_COMMAND} -E make_directory   ${CMAKE_HOME_DIRECTORY}/doc/html
    WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}/doc
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

  configure_file(${CMAKE_HOME_DIRECTORY}/doc/Doxyfile.in ${CMAKE_HOME_DIRECTORY}/doc/Doxyfile @ONLY)

  foreach(file ${DOC_FIGS})
    string(REPLACE ".fig" ".png" tmp_file ${file})
    string(REPLACE "${CMAKE_HOME_DIRECTORY}/doc/shared/fig/" "${CMAKE_HOME_DIRECTORY}/doc/html/" tmp_file ${tmp_file})
    ADD_CUSTOM_COMMAND(TARGET documentation  COMMAND ${FIG2DEV_PATH}/fig2dev -Lpng -S 4 ${file} ${tmp_file})
  endforeach()

  foreach(file ${DOC_IMG})
    ADD_CUSTOM_COMMAND(TARGET documentation COMMAND ${CMAKE_COMMAND} -E copy ${file} ${CMAKE_HOME_DIRECTORY}/doc/html/)
  endforeach()

  ADD_CUSTOM_COMMAND(TARGET documentation
    COMMAND ${FIG2DEV_PATH}/fig2dev -Lmap ${CMAKE_HOME_DIRECTORY}/doc/shared/fig/simgrid_modules.fig | perl -pe 's/imagemap/simgrid_modules/g'| perl -pe 's/<IMG/<IMG style=border:0px/g' | ${CMAKE_HOME_DIRECTORY}/tools/doxygen/fig2dev_postprocessor.pl > ${CMAKE_HOME_DIRECTORY}/doc/simgrid_modules.map
    COMMAND pwd
    COMMAND ${CMAKE_COMMAND} -E echo "XX Run doxygen"
    COMMAND ${DOXYGEN_EXECUTABLE} Doxyfile
    COMMAND ${CMAKE_COMMAND} -E echo "XX Generate the index files"
    COMMAND ${CMAKE_HOME_DIRECTORY}/tools/doxygen/index_create.pl simgrid.tag index-API.doc
    COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_HOME_DIRECTORY}/doc/doxygen/logcategories.doc
    COMMAND ${CMAKE_HOME_DIRECTORY}/tools/doxygen/xbt_log_extract_hierarchy.pl > ${CMAKE_HOME_DIRECTORY}/doc/doxygen/logcategories.doc
    COMMAND ${CMAKE_COMMAND} -E echo "XX Generate list of files in examples/ for routing models"
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_HOME_DIRECTORY}/doc/example_lists/
    COMMAND ${CMAKE_HOME_DIRECTORY}/tools/doxygen/list_routing_models_examples.sh Floyd > ${CMAKE_HOME_DIRECTORY}/doc/example_lists/example_filelist_routing_floyd
    COMMAND ${CMAKE_HOME_DIRECTORY}/tools/doxygen/list_routing_models_examples.sh Dijkstra > ${CMAKE_HOME_DIRECTORY}/doc/example_lists/example_filelist_routing_dijkstra
    COMMAND ${CMAKE_HOME_DIRECTORY}/tools/doxygen/list_routing_models_examples.sh DijkstraCache > ${CMAKE_HOME_DIRECTORY}/doc/example_lists/example_filelist_routing_dijkstra_cache
    COMMAND ${CMAKE_HOME_DIRECTORY}/tools/doxygen/list_routing_models_examples.sh 'routing="None"' > ${CMAKE_HOME_DIRECTORY}/doc/example_lists/example_filelist_routing_none
    COMMAND ${CMAKE_HOME_DIRECTORY}/tools/doxygen/list_routing_models_examples.sh 'routing="Cluster"' > ${CMAKE_HOME_DIRECTORY}/doc/example_lists/example_filelist_routing_cluster
    COMMAND ${CMAKE_HOME_DIRECTORY}/tools/doxygen/list_routing_models_examples.sh 'routing="Vivaldi"' > ${CMAKE_HOME_DIRECTORY}/doc/example_lists/example_filelist_routing_vivaldi
    COMMAND ${CMAKE_HOME_DIRECTORY}/tools/doxygen/list_routing_models_examples.sh 'routing="Full"' > ${CMAKE_HOME_DIRECTORY}/doc/example_lists/example_filelist_routing_full
    COMMAND ${CMAKE_COMMAND} -E echo "XX Generate list of files in examples/ for XML tags"
    COMMAND ${CMAKE_HOME_DIRECTORY}/tools/doxygen/list_routing_models_examples.sh '<mount ' > ${CMAKE_HOME_DIRECTORY}/doc/example_lists/example_filelist_xmltag_mount
    COMMAND ${CMAKE_HOME_DIRECTORY}/tools/doxygen/list_routing_models_examples.sh '<link_ctn ' > ${CMAKE_HOME_DIRECTORY}/doc/example_lists/example_filelist_xmltag_linkctn
    COMMAND ${CMAKE_COMMAND} -E echo "XX Run doxygen again"
    COMMAND ${DOXYGEN_EXECUTABLE} Doxyfile
    COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_HOME_DIRECTORY}/doc/simgrid_modules.map
    WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}/doc
    )

### Fill in the "make sync-gforge" target ###

set(RSYNC_CMD rsync --verbose --cvs-exclude --compress --delete --delete-excluded --rsh=ssh --ignore-times --recursive --links --times --omit-dir-times --perms --chmod=a+rX,ug+w,o-w,Dg+s)

add_custom_target(sync-gforge-doc
  COMMAND ssh scm.gforge.inria.fr mkdir -p -m 2775 /home/groups/simgrid/htdocs/simgrid/${release_version}/ || true

  COMMAND ${RSYNC_CMD} doc/html/ scm.gforge.inria.fr:/home/groups/simgrid/htdocs/simgrid/${release_version}/doc/ || true

  COMMAND ${RSYNC_CMD} doc/html/simgrid_modules2.png doc/html/simgrid_modules.png doc/webcruft/simgrid_logo_2011.png
  doc/webcruft/simgrid_logo_2011_small.png scm.gforge.inria.fr:/home/groups/simgrid/htdocs/simgrid/${release_version}/

  COMMAND ${RSYNC_CMD} src/surf/xml/simgrid.dtd scm.gforge.inria.fr:/home/groups/simgrid/htdocs/simgrid/

  WORKING_DIRECTORY "${CMAKE_HOME_DIRECTORY}"
  )
add_dependencies(sync-gforge-doc documentation)

add_custom_target(sync-gforge-dtd
  COMMAND ${RSYNC_CMD} src/surf/simgrid.dtd scm.gforge.inria.fr:/home/groups/simgrid/htdocs/simgrid/${release_version}/simgrid.dtd
  COMMAND ${RSYNC_CMD} src/surf/simgrid.dtd scm.gforge.inria.fr:/home/groups/simgrid/htdocs/simgrid.dtd
  WORKING_DIRECTORY "${CMAKE_HOME_DIRECTORY}"
  )
endif() # Doxygen found

if (Java_FOUND)
  find_path(JAVADOC_PATH  NAMES javadoc   PATHS NO_DEFAULT_PATHS)
  mark_as_advanced(JAVADOC_PATH)
  
  ADD_CUSTOM_COMMAND(TARGET documentation
    COMMAND ${CMAKE_COMMAND} -E echo "XX Javadoc pass"
    COMMAND ${JAVADOC_PATH}/javadoc -quiet -d ${CMAKE_HOME_DIRECTORY}/doc/html/javadoc/ ${CMAKE_HOME_DIRECTORY}/src/bindings/java/org/simgrid/*.java ${CMAKE_HOME_DIRECTORY}/src/bindings/java/org/simgrid/*/*.java
    WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}/doc
  )
endif()
       
#### Generate the manpages
if( NOT MANPAGE_DIR)
  set( MANPAGE_DIR ${CMAKE_BINARY_DIR}/manpages )
endif()

add_custom_target(manpages ALL
  COMMAND ${CMAKE_COMMAND} -E make_directory ${MANPAGE_DIR}
  COMMAND pod2man ${CMAKE_HOME_DIRECTORY}/tools/simgrid_update_xml.pl > ${MANPAGE_DIR}/simgrid_update_xml.1
  COMMAND pod2man ${CMAKE_HOME_DIRECTORY}/tools/tesh/tesh.pl > ${MANPAGE_DIR}/tesh.1
  COMMENT "Generating manpages"
  )
install(FILES
  ${MANPAGE_DIR}/simgrid_update_xml.1
  ${MANPAGE_DIR}/tesh.1
  ${CMAKE_HOME_DIRECTORY}/doc/manpage/smpicc.1
  ${CMAKE_HOME_DIRECTORY}/doc/manpage/smpicxx.1
  ${CMAKE_HOME_DIRECTORY}/doc/manpage/smpif90.1
  ${CMAKE_HOME_DIRECTORY}/doc/manpage/smpiff.1
  ${CMAKE_HOME_DIRECTORY}/doc/manpage/smpirun.1
  DESTINATION $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/man/man1)
