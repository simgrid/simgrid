#### Generate the html documentation

if(DOXYGEN_PATH)

  configure_file(${CMAKE_HOME_DIRECTORY}/doc/dev_guide/doxygen/DevGuideDoxyfile.in ${CMAKE_HOME_DIRECTORY}/doc/dev_guide/doxygen/DevGuideDoxyfile @ONLY)

  ADD_CUSTOM_TARGET(dev_guide
    COMMENT "Generating the SimGrid dev guide..."
    DEPENDS ${DEV_GUIDE_SOURCES}
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_HOME_DIRECTORY}/doc/dev_guide/html
    COMMAND ${CMAKE_COMMAND} -E make_directory   ${CMAKE_HOME_DIRECTORY}/doc/dev_guide/html
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_HOME_DIRECTORY}/doc/simgrid.css ${CMAKE_HOME_DIRECTORY}/doc/dev_guide/html/
    WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}/doc/dev_guide/
    )
    
  ADD_CUSTOM_COMMAND(TARGET dev_guide
    COMMAND ${CMAKE_COMMAND} -E echo "XX First Doxygen pass"
    COMMAND ${DOXYGEN_PATH}/doxygen DevGuideDoxyfile
    COMMAND ${CMAKE_HOME_DIRECTORY}/tools/doxygen/index_create.pl ../../shared/doxygen/simgriddevguide.tag index-API.doc

    COMMAND ${CMAKE_COMMAND} -E echo "XX Second Doxygen pass"
    COMMAND ${DOXYGEN_PATH}/doxygen DevGuideDoxyfile

    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_HOME_DIRECTORY}/doc/dev_guide/html/dir*

    WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}/doc/dev_guide/doxygen
    )

else(DOXYGEN_PATH)

  ADD_CUSTOM_TARGET(dev_guide
    COMMENT "Generating the SimGrid documentation..."
    )

  ADD_CUSTOM_COMMAND(TARGET dev_guide
    COMMAND ${CMAKE_COMMAND} -E echo "DOXYGEN_PATH     = ${DOXYGEN_PATH}"
    COMMAND ${CMAKE_COMMAND} -E echo "IN ORDER TO GENERATE THE DOCUMENTATION YOU NEED ALL TOOLS !!!"
    COMMAND ${CMAKE_COMMAND} -E echo "FAIL TO MAKE SIMGRID DOCUMENTATION see previous messages for details ..."
    COMMAND false
    )

endif(DOXYGEN_PATH)

##############################################################################"
