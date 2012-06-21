#### Generate the html documentation

file(GLOB_RECURSE source_doxygen
  "${CMAKE_HOME_DIRECTORY}/tools/gras/*.[chl]"
  "${CMAKE_HOME_DIRECTORY}/src/*.[chl]"
  "${CMAKE_HOME_DIRECTORY}/include/*.[chl]"
)

if(FIG2DEV_PATH)

  set(DOC_PNGS 
    ${CMAKE_HOME_DIRECTORY}/doc/webcruft/simgrid_logo_2011.png
    ${CMAKE_HOME_DIRECTORY}/doc/webcruft/simgrid_logo_2011_small.png
  )
  
  configure_file(${CMAKE_HOME_DIRECTORY}/doc/ref_guide/doxygen/RefGuideDoxyfile.in ${CMAKE_HOME_DIRECTORY}/doc/ref_guide/doxygen/RefGuideDoxyfile @ONLY)
  
  ADD_CUSTOM_TARGET(ref_guide
    COMMENT "Generating the SimGrid ref guide..."
    DEPENDS ${REF_GUIDE_SOURCES} ${DOC_FIGS} ${source_doxygen}
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_HOME_DIRECTORY}/doc/ref_guide/html
    COMMAND ${CMAKE_COMMAND} -E make_directory   ${CMAKE_HOME_DIRECTORY}/doc/ref_guide/html    
    WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}/doc/ref_guide/
  )
    
  ADD_CUSTOM_COMMAND(TARGET ref_guide
    DEPENDS ${source_doxygen}
    COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_HOME_DIRECTORY}/doc/ref_guide/doxygen/logcategories.doc
    COMMAND ${CMAKE_HOME_DIRECTORY}/tools/doxygen/xbt_log_extract_hierarchy.pl > ${CMAKE_HOME_DIRECTORY}/doc/ref_guide/doxygen/logcategories.doc
    WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}
  )

  foreach(file ${DOC_FIGS})
    string(REPLACE ".fig" ".png" tmp_file ${file})
    string(REPLACE "${CMAKE_HOME_DIRECTORY}/doc/shared/fig/" "${CMAKE_HOME_DIRECTORY}/doc/ref_guide/html/" tmp_file ${tmp_file})    
    ADD_CUSTOM_COMMAND(TARGET ref_guide
      COMMAND ${FIG2DEV_PATH}/fig2dev -Lpng -S 4 ${file} ${tmp_file}
    )
  endforeach(file ${DOC_FIGS})
  
  foreach(file ${DOC_PNGS})
    ADD_CUSTOM_COMMAND(TARGET ref_guide
      COMMAND ${CMAKE_COMMAND} -E copy ${file} ${CMAKE_HOME_DIRECTORY}/doc/ref_guide/html/
    )
  endforeach(file ${DOC_PNGS})

  ADD_CUSTOM_COMMAND(TARGET ref_guide
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_HOME_DIRECTORY}/doc/simgrid.css                          ${CMAKE_HOME_DIRECTORY}/doc/ref_guide/html/
  )
  
  ADD_CUSTOM_COMMAND(TARGET ref_guide
    COMMAND ${FIG2DEV_PATH}/fig2dev -Lmap ${CMAKE_HOME_DIRECTORY}/doc/shared/fig/simgrid_modules.fig | perl -pe 's/imagemap/simgrid_modules/g'| perl -pe 's/<IMG/<IMG style=border:0px/g' | ${CMAKE_HOME_DIRECTORY}/tools/doxygen/fig2dev_postprocessor.pl > ${CMAKE_HOME_DIRECTORY}/doc/ref_guide/doxygen/simgrid_modules.map
    COMMAND ${CMAKE_COMMAND} -E echo "XX First Doxygen pass"
    COMMAND ${DOXYGEN_PATH}/doxygen RefGuideDoxyfile
    COMMAND ${CMAKE_HOME_DIRECTORY}/tools/doxygen/index_create.pl simgridrefguide.tag index-API.doc

    COMMAND ${CMAKE_COMMAND} -E echo "XX Second Doxygen pass"
    COMMAND ${DOXYGEN_PATH}/doxygen RefGuideDoxyfile
    
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_HOME_DIRECTORY}/doc/ref_guide/html/dir*
    

    WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}/doc/ref_guide/doxygen
  )
  
else(DOXYGEN_PATH AND FIG2DEV_PATH)

  ADD_CUSTOM_TARGET(ref_guide
    COMMENT "Generating the SimGrid documentation..."
  )

  ADD_CUSTOM_COMMAND(TARGET ref_guide
    COMMAND ${CMAKE_COMMAND} -E echo "DOXYGEN_PATH     = ${DOXYGEN_PATH}"
    COMMAND ${CMAKE_COMMAND} -E echo "FIG2DEV_PATH     = ${FIG2DEV_PATH}"
    COMMAND ${CMAKE_COMMAND} -E echo "IN ORDER TO GENERATE THE DOCUMENTATION YOU NEED ALL TOOLS !!!"
    COMMAND ${CMAKE_COMMAND} -E echo "FAIL TO MAKE SIMGRID DOCUMENTATION see previous messages for details ..."
    COMMAND false
  )

    
endif(FIG2DEV_PATH)

##############################################################################"


ADD_CUSTOM_TARGET(ref_guide_pdf
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
add_dependencies(ref_guide_pdf ref_guide)
