#### Generate the html documentation

if(DOXYGEN_PATH)

  set(DOC_PNGS
    ${CMAKE_HOME_DIRECTORY}/doc/webcruft/simgrid_logo_2011.png
    ${CMAKE_HOME_DIRECTORY}/doc/webcruft/simgrid_logo_2011_small.png
    )

  configure_file(${CMAKE_HOME_DIRECTORY}/doc/dev_guide/doxygen/DevGuideDoxyfile.in ${CMAKE_HOME_DIRECTORY}/doc/dev_guide/doxygen/DevGuideDoxyfile @ONLY)

  ADD_CUSTOM_TARGET(dev_guide
    COMMENT "Generating the SimGrid dev guide..."
    DEPENDS ${DEV_GUIDE_SOURCES}
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_HOME_DIRECTORY}/doc/dev_guide/html
    COMMAND ${CMAKE_COMMAND} -E make_directory   ${CMAKE_HOME_DIRECTORY}/doc/dev_guide/html
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_HOME_DIRECTORY}/doc/simgrid.css ${CMAKE_HOME_DIRECTORY}/doc/dev_guide/html/
    WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}/doc/dev_guide/
    )
    
 foreach(file ${DOC_FIGS})
    string(REPLACE ".fig" ".png" tmp_file ${file})
    string(REPLACE "${CMAKE_HOME_DIRECTORY}/doc/shared/fig/" "${CMAKE_HOME_DIRECTORY}/doc/dev_guide/html/" tmp_file ${tmp_file})
    ADD_CUSTOM_COMMAND(TARGET dev_guide
      COMMAND ${FIG2DEV_PATH}/fig2dev -Lpng -S 4 ${file} ${tmp_file}
      )
  endforeach(file ${DOC_FIGS})

  foreach(file ${DOC_PNGS})
    ADD_CUSTOM_COMMAND(TARGET dev_guide
      COMMAND ${CMAKE_COMMAND} -E copy ${file} ${CMAKE_HOME_DIRECTORY}/doc/dev_guide/html/
      )
  endforeach(file ${DOC_PNGS})
    
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
