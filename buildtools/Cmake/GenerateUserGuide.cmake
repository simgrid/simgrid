#### Generate the html documentation for the user guide.

file(GLOB_RECURSE source_doxygen
  "${CMAKE_HOME_DIRECTORY}/src/*.[chl]"
  "${CMAKE_HOME_DIRECTORY}/include/*.[chl]"
  )

if(FIG2DEV_PATH)

  configure_file(${CMAKE_HOME_DIRECTORY}/doc/doxygen/UserGuideDoxyfile.in ${CMAKE_HOME_DIRECTORY}/doc/doxygen/UserGuideDoxyfile @ONLY)

  ADD_CUSTOM_TARGET(user_guide
    COMMENT "Generating the SimGrid user guide..."
    DEPENDS ${USER_GUIDE_SOURCES} ${DOC_FIGS} ${source_doxygen}
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_HOME_DIRECTORY}/doc/html
    COMMAND ${CMAKE_COMMAND} -E make_directory   ${CMAKE_HOME_DIRECTORY}/doc/html
    WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}/doc/doxygen
    )

  foreach(file ${DOC_FIGS})
    string(REPLACE ".fig" ".png" tmp_file ${file})
    string(REPLACE "${CMAKE_HOME_DIRECTORY}/doc/shared/fig/" "${CMAKE_HOME_DIRECTORY}/doc/html/" tmp_file ${tmp_file})
    ADD_CUSTOM_COMMAND(TARGET user_guide
      COMMAND ${FIG2DEV_PATH}/fig2dev -Lpng -S 4 ${file} ${tmp_file}
      )
  endforeach(file ${DOC_FIGS})

  foreach(file ${DOC_PNGS})
    ADD_CUSTOM_COMMAND(TARGET user_guide
      COMMAND ${CMAKE_COMMAND} -E copy ${file} ${CMAKE_HOME_DIRECTORY}/doc/html/
      )
  endforeach(file ${DOC_PNGS})

  ADD_CUSTOM_COMMAND(TARGET user_guide
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_HOME_DIRECTORY}/doc/webcruft/Paje_MSG_screenshot_thn.jpg ${CMAKE_HOME_DIRECTORY}/doc/html/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_HOME_DIRECTORY}/doc/webcruft/Paje_MSG_screenshot.jpg     ${CMAKE_HOME_DIRECTORY}/doc/html/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_HOME_DIRECTORY}/doc/triva-graph_configuration.png        ${CMAKE_HOME_DIRECTORY}/doc/html/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_HOME_DIRECTORY}/doc/triva-graph_visualization.png        ${CMAKE_HOME_DIRECTORY}/doc/html/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_HOME_DIRECTORY}/doc/triva-time_interval.png        ${CMAKE_HOME_DIRECTORY}/doc/html/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_HOME_DIRECTORY}/doc/AS_hierarchy.png        ${CMAKE_HOME_DIRECTORY}/doc/html/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_HOME_DIRECTORY}/doc/simgrid.css                          ${CMAKE_HOME_DIRECTORY}/doc/html/
    )

  ADD_CUSTOM_COMMAND(TARGET user_guide
    COMMAND ${FIG2DEV_PATH}/fig2dev -Lmap ${CMAKE_HOME_DIRECTORY}/doc/shared/fig/simgrid_modules.fig | perl -pe 's/imagemap/simgrid_modules/g'| perl -pe 's/<IMG/<IMG style=border:0px/g' | ${CMAKE_HOME_DIRECTORY}/tools/doxygen/fig2dev_postprocessor.pl > ${CMAKE_HOME_DIRECTORY}/doc/doxygen/simgrid_modules.map
    COMMAND ${CMAKE_COMMAND} -E echo "XX First Doxygen pass"
    COMMAND ${DOXYGEN_PATH}/doxygen UserGuideDoxyfile

    COMMAND ${CMAKE_COMMAND} -E echo "XX Second Doxygen pass"
    COMMAND ${DOXYGEN_PATH}/doxygen UserGuideDoxyfile

    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_HOME_DIRECTORY}/doc/html/dir*

    WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}/doc/doxygen
    )

else()

  ADD_CUSTOM_TARGET(user_guide
    COMMENT "Generating the SimGrid user guide..."
    )

  ADD_CUSTOM_COMMAND(TARGET user_guide
    COMMAND ${CMAKE_COMMAND} -E echo "DOXYGEN_PATH 		= ${DOXYGEN_PATH}"
    COMMAND ${CMAKE_COMMAND} -E echo "FIG2DEV_PATH 		= ${FIG2DEV_PATH}"
    COMMAND ${CMAKE_COMMAND} -E echo "IN ORDER TO GENERATE THE DOCUMENTATION YOU NEED ALL TOOLS !!!"
    COMMAND ${CMAKE_COMMAND} -E echo "FAIL TO MAKE SIMGRID DOCUMENTATION see previous messages for details ..."
    COMMAND false
    )

endif()

ADD_CUSTOM_TARGET(user_guide_pdf
    COMMAND ${CMAKE_COMMAND} -E echo "XX First pass simgrid_user_guide.pdf"
    COMMAND make clean
    COMMAND make pdf || true
    COMMAND ${CMAKE_COMMAND} -E echo "XX Second pass simgrid_user_guide.pdf"
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_HOME_DIRECTORY}/doc/latex/refman.pdf
    COMMAND make pdf || true
    COMMAND ${CMAKE_COMMAND} -E echo "XX Write Simgrid_documentation.pdf"
    COMMAND ${CMAKE_COMMAND} -E rename ${CMAKE_HOME_DIRECTORY}/doc/latex/refman.pdf ${CMAKE_HOME_DIRECTORY}/doc/latex/simgrid_documentation.pdf

    WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}/doc/latex/
)
add_dependencies(user_guide_pdf user_guide)
