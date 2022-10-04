###
### Generate all parts of the documentation on non-Windows systems
###
###   - Javadoc (reference)
###   - manpages (reference of tools)
###
###  This file is not loaded on windows

#### Generate the html documentation
find_path(FIG2DEV_PATH  NAMES fig2dev  PATHS NO_DEFAULT_PATHS)

if(enable_documentation)
  if (Java_FOUND)
    find_path(JAVADOC_PATH  NAMES javadoc   PATHS NO_DEFAULT_PATHS)
    mark_as_advanced(JAVADOC_PATH)

   ADD_CUSTOM_TARGET(documentation
      COMMENT "Generating the SimGrid documentation..."
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/doc/html
    )
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
