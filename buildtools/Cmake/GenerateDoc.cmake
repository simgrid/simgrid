#### Generate the whole html documentation

find_path(DOXYGEN_PATH  NAMES doxygen  PATHS NO_DEFAULT_PATHS)
find_path(FIG2DEV_PATH  NAMES fig2dev  PATHS NO_DEFAULT_PATHS)

if(DOXYGEN_PATH)

  ADD_CUSTOM_TARGET(simgrid_documentation
    COMMENT "Generating the SimGrid documentation..."
    DEPENDS ${DOC_SOURCES} ${DOC_FIGS} ${source_doxygen}
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_HOME_DIRECTORY}/doc/html
    COMMAND ${CMAKE_COMMAND} -E make_directory   ${CMAKE_HOME_DIRECTORY}/doc/html
    WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}/doc
    )

  execute_process(COMMAND ${DOXYGEN_PATH}/doxygen --version OUTPUT_VARIABLE DOXYGEN_VERSION )
  string(REGEX MATCH "^[0-9]" DOXYGEN_MAJOR_VERSION "${DOXYGEN_VERSION}")
  string(REGEX MATCH "^[0-9].[0-9]" DOXYGEN_MINOR_VERSION "${DOXYGEN_VERSION}")
  string(REGEX MATCH "^[0-9].[0-9].[0-9]" DOXYGEN_PATCH_VERSION "${DOXYGEN_VERSION}")
  string(REGEX REPLACE "^${DOXYGEN_MINOR_VERSION}." "" DOXYGEN_PATCH_VERSION "${DOXYGEN_PATCH_VERSION}")
  string(REGEX REPLACE "^${DOXYGEN_MAJOR_VERSION}." "" DOXYGEN_MINOR_VERSION "${DOXYGEN_MINOR_VERSION}")
  message(STATUS "Doxygen version : ${DOXYGEN_MAJOR_VERSION}.${DOXYGEN_MINOR_VERSION}.${DOXYGEN_PATCH_VERSION}")

  if(DOXYGEN_MAJOR_VERSION STRLESS "2" AND DOXYGEN_MINOR_VERSION STRLESS "8")
    ADD_CUSTOM_TARGET(error_doxygen
      COMMAND ${CMAKE_COMMAND} -E echo "Doxygen must be at least version 1.8 to generate documentation"
      COMMAND false
    )

    add_dependencies(simgrid_documentation error_doxygen)
  endif()

  configure_file(${CMAKE_HOME_DIRECTORY}/doc/Doxyfile.in ${CMAKE_HOME_DIRECTORY}/doc/Doxyfile @ONLY)

  foreach(file ${DOC_IMG})
    ADD_CUSTOM_COMMAND(
      TARGET simgrid_documentation
      COMMAND ${CMAKE_COMMAND} -E copy ${file} ${CMAKE_HOME_DIRECTORY}/doc/html/
    )
  endforeach()

  ADD_CUSTOM_COMMAND(TARGET simgrid_documentation
    COMMAND ${FIG2DEV_PATH}/fig2dev -Lmap ${CMAKE_HOME_DIRECTORY}/doc/shared/fig/simgrid_modules.fig | perl -pe 's/imagemap/simgrid_modules/g'| perl -pe 's/<IMG/<IMG style=border:0px/g' | ${CMAKE_HOME_DIRECTORY}/tools/doxygen/fig2dev_postprocessor.pl > ${CMAKE_HOME_DIRECTORY}/doc/simgrid_modules.map
    COMMAND ${CMAKE_COMMAND} -E echo "XX Run doxygen"
    COMMAND ${DOXYGEN_PATH}/doxygen Doxyfile
    COMMAND ${CMAKE_COMMAND} -E echo "XX Generate the index files"
    COMMAND ${CMAKE_HOME_DIRECTORY}/tools/doxygen/index_create.pl simgrid.tag index-API.doc     
    COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_HOME_DIRECTORY}/doc/doxygen/logcategories.doc
    COMMAND ${CMAKE_HOME_DIRECTORY}/tools/doxygen/xbt_log_extract_hierarchy.pl > ${CMAKE_HOME_DIRECTORY}/doc/doxygen/logcategories.doc
    COMMAND ${CMAKE_COMMAND} -E echo "XX Run doxygen again"
    COMMAND ${DOXYGEN_PATH}/doxygen Doxyfile
    COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_HOME_DIRECTORY}/doc/simgrid_modules.map
    WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}/doc
    )

  ADD_CUSTOM_TARGET(pdf
    COMMAND ${CMAKE_COMMAND} -E echo "XX First pass simgrid_documentation.pdf"
    COMMAND make clean
    COMMAND make pdf || true
    COMMAND ${CMAKE_COMMAND} -E echo "XX Second pass simgrid_documentation.pdf"
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_HOME_DIRECTORY}/doc/latex/refman.pdf
    COMMAND make pdf || true
    COMMAND ${CMAKE_COMMAND} -E echo "XX Write Simgrid_documentation.pdf"
    COMMAND ${CMAKE_COMMAND} -E rename ${CMAKE_HOME_DIRECTORY}/doc/latex/refman.pdf ${CMAKE_HOME_DIRECTORY}/doc/latex/simgrid_documentation.pdf
    WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}/doc/latex/
    )
  add_dependencies(pdf simgrid_documentation)


endif()

#############################################
### Fill in the "make sync-gforge" target ###
#############################################

add_custom_target(sync-gforge-doc
  COMMAND chmod g+rw -R doc/
  COMMAND chmod a+rX -R doc/
  COMMAND ssh scm.gforge.inria.fr mkdir -p /home/groups/simgrid/htdocs/simgrid/${release_version}/html/ || true

  COMMAND rsync --verbose --cvs-exclude --compress --delete --delete-excluded --rsh=ssh --ignore-times --recursive --links --perms --times --omit-dir-times
  doc/html/ scm.gforge.inria.fr:/home/groups/simgrid/htdocs/simgrid/${release_version}/doc/ || true

  COMMAND scp doc/html/simgrid_modules2.png doc/html/simgrid_modules.png doc/webcruft/simgrid_logo_2011.png
  doc/webcruft/simgrid_logo_2011_small.png scm.gforge.inria.fr:/home/groups/simgrid/htdocs/simgrid/${release_version}/
  WORKING_DIRECTORY "${CMAKE_HOME_DIRECTORY}"
  )
add_dependencies(sync-gforge-doc simgrid_documentation)

add_custom_target(sync-gforge-dtd
  COMMAND scp src/surf/simgrid.dtd scm.gforge.inria.fr:/home/groups/simgrid/htdocs/simgrid/${release_version}/simgrid.dtd
  COMMAND scp src/surf/simgrid.dtd scm.gforge.inria.fr:/home/groups/simgrid/htdocs/simgrid.dtd
  WORKING_DIRECTORY "${CMAKE_HOME_DIRECTORY}"
  )

