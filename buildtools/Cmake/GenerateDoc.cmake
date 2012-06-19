#### Generate the whole html documentation

find_path(DOXYGEN_PATH  NAMES doxygen  PATHS NO_DEFAULT_PATHS)
find_path(FIG2DEV_PATH  NAMES fig2dev  PATHS NO_DEFAULT_PATHS)

if(DOXYGEN_PATH)

  execute_process(COMMAND ${DOXYGEN_PATH}/doxygen --version OUTPUT_VARIABLE DOXYGEN_VERSION )
  string(REGEX MATCH "^[0-9]" DOXYGEN_MAJOR_VERSION "${DOXYGEN_VERSION}")
  string(REGEX MATCH "^[0-9].[0-9]" DOXYGEN_MINOR_VERSION "${DOXYGEN_VERSION}")
  string(REGEX MATCH "^[0-9].[0-9].[0-9]" DOXYGEN_PATCH_VERSION "${DOXYGEN_VERSION}")
  string(REGEX REPLACE "^${DOXYGEN_MINOR_VERSION}." "" DOXYGEN_PATCH_VERSION "${DOXYGEN_PATCH_VERSION}")
  string(REGEX REPLACE "^${DOXYGEN_MAJOR_VERSION}." "" DOXYGEN_MINOR_VERSION "${DOXYGEN_MINOR_VERSION}")
  message(STATUS "Doxygen version : ${DOXYGEN_MAJOR_VERSION}.${DOXYGEN_MINOR_VERSION}.${DOXYGEN_PATCH_VERSION}")

  include(${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/GenerateUserGuide.cmake)
  include(${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/GenerateRefGuide.cmake)

  set(DOC_PNGS 
    ${CMAKE_HOME_DIRECTORY}/doc/webcruft/simgrid_logo_2011.png
    ${CMAKE_HOME_DIRECTORY}/doc/webcruft/simgrid_logo_2011_small.png
  )
  
  configure_file(${CMAKE_HOME_DIRECTORY}/doc/Doxyfile.in ${CMAKE_HOME_DIRECTORY}/doc/Doxyfile @ONLY)
  
  ADD_CUSTOM_TARGET(simgrid_documentation
    COMMENT "Generating the SimGrid documentation..."
    DEPENDS ${DOC_SOURCES} ${DOC_FIGS} ${source_doxygen}
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_HOME_DIRECTORY}/doc/html
      COMMAND ${CMAKE_COMMAND} -E make_directory   ${CMAKE_HOME_DIRECTORY}/doc/html
    WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}/doc
  )

  foreach(file ${DOC_PNGS})
    ADD_CUSTOM_COMMAND(
      TARGET simgrid_documentation
      COMMAND ${CMAKE_COMMAND} -E copy ${file} ${CMAKE_HOME_DIRECTORY}/doc/html/
    )
  endforeach(file ${DOC_PNGS})

  ADD_CUSTOM_COMMAND(TARGET simgrid_documentation
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_HOME_DIRECTORY}/doc/simgrid.css ${CMAKE_HOME_DIRECTORY}/doc/html/
  )
  
  ADD_CUSTOM_COMMAND(TARGET simgrid_documentation
      COMMAND ${CMAKE_COMMAND} -E echo "XX Doxygen pass"
    COMMAND ${DOXYGEN_PATH}/doxygen Doxyfile
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

  ADD_CUSTOM_TARGET(error_doxygen
    COMMAND ${CMAKE_COMMAND} -E echo "Doxygen must be at least version 1.8 to generate documentation"
    COMMAND false
  )

  if(DOXYGEN_MAJOR_VERSION STRLESS "2" AND DOXYGEN_MINOR_VERSION STRLESS "8")
    add_dependencies(simgrid_documentation error_doxygen)
  else(DOXYGEN_MAJOR_VERSION STRLESS "2" AND DOXYGEN_MINOR_VERSION STRLESS "8")
    add_dependencies(simgrid_documentation ref_guide)
    add_dependencies(simgrid_documentation user_guide)
  endif(DOXYGEN_MAJOR_VERSION STRLESS "2" AND DOXYGEN_MINOR_VERSION STRLESS "8")

endif(DOXYGEN_PATH)
