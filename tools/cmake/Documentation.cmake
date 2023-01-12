###
### Generate the manpages documentation. The sphinx content is not handled in cmake but with the Build.sh script
###

#### Generate the html documentation
find_path(FIG2DEV_PATH  NAMES fig2dev  PATHS NO_DEFAULT_PATHS)

if(enable_documentation)
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
