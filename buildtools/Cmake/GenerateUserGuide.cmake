#### Generate the html documentation for the user guide.

file(GLOB_RECURSE source_doxygen
  "${CMAKE_HOME_DIRECTORY}/tools/gras/*.[chl]"
  "${CMAKE_HOME_DIRECTORY}/src/*.[chl]"
  "${CMAKE_HOME_DIRECTORY}/include/*.[chl]"
  )

if(FIG2DEV_PATH)
  set(DOC_PNGS
    ${CMAKE_HOME_DIRECTORY}/doc/webcruft/simgrid_logo_2011.png
    ${CMAKE_HOME_DIRECTORY}/doc/webcruft/simgrid_logo_2011_small.png
    ${CMAKE_HOME_DIRECTORY}/doc/webcruft/poster_thumbnail.png
    ${CMAKE_HOME_DIRECTORY}/doc/webcruft/win_install_01.png
    ${CMAKE_HOME_DIRECTORY}/doc/webcruft/win_install_02.png
    ${CMAKE_HOME_DIRECTORY}/doc/webcruft/win_install_03.png
    ${CMAKE_HOME_DIRECTORY}/doc/webcruft/win_install_04.png
    ${CMAKE_HOME_DIRECTORY}/doc/webcruft/win_install_05.png
    ${CMAKE_HOME_DIRECTORY}/doc/webcruft/win_install_06.png
    )

  configure_file(${CMAKE_HOME_DIRECTORY}/doc/user_guide/doxygen/UserGuideDoxyfile.in ${CMAKE_HOME_DIRECTORY}/doc/user_guide/doxygen/UserGuideDoxyfile @ONLY)

  ADD_CUSTOM_TARGET(user_guide
    COMMENT "Generating the SimGrid user guide..."
    DEPENDS ${USER_GUIDE_SOURCES} ${DOC_FIGS} ${source_doxygen}
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_HOME_DIRECTORY}/doc/user_guide/html
    COMMAND ${CMAKE_COMMAND} -E make_directory   ${CMAKE_HOME_DIRECTORY}/doc/user_guide/html
    WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}/doc/user_guide/doxygen
    )

  ADD_CUSTOM_COMMAND(TARGET user_guide
    DEPENDS ${source_doxygen}
    COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_HOME_DIRECTORY}/doc/user_guide/logcategories.doc
    COMMAND ${CMAKE_HOME_DIRECTORY}/tools/doxygen/xbt_log_extract_hierarchy.pl > ${CMAKE_HOME_DIRECTORY}/doc/user_guide/doxygen/logcategories.doc
    WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}
    )

  foreach(file ${DOC_FIGS})
    string(REPLACE ".fig" ".png" tmp_file ${file})
    string(REPLACE "${CMAKE_HOME_DIRECTORY}/doc/shared/fig/" "${CMAKE_HOME_DIRECTORY}/doc/user_guide/html/" tmp_file ${tmp_file})
    ADD_CUSTOM_COMMAND(TARGET user_guide
      COMMAND ${FIG2DEV_PATH}/fig2dev -Lpng -S 4 ${file} ${tmp_file}
      )
  endforeach(file ${DOC_FIGS})

  foreach(file ${DOC_PNGS})
    ADD_CUSTOM_COMMAND(TARGET user_guide
      COMMAND ${CMAKE_COMMAND} -E copy ${file} ${CMAKE_HOME_DIRECTORY}/doc/user_guide/html/
      )
  endforeach(file ${DOC_PNGS})

  ADD_CUSTOM_COMMAND(TARGET user_guide
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_HOME_DIRECTORY}/doc/webcruft/Paje_MSG_screenshot_thn.jpg ${CMAKE_HOME_DIRECTORY}/doc/user_guide/html/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_HOME_DIRECTORY}/doc/webcruft/Paje_MSG_screenshot.jpg     ${CMAKE_HOME_DIRECTORY}/doc/user_guide/html/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_HOME_DIRECTORY}/doc/triva-graph_configuration.png        ${CMAKE_HOME_DIRECTORY}/doc/user_guide/html/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_HOME_DIRECTORY}/doc/triva-graph_visualization.png        ${CMAKE_HOME_DIRECTORY}/doc/user_guide/html/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_HOME_DIRECTORY}/doc/AS_hierarchy.png        ${CMAKE_HOME_DIRECTORY}/doc/user_guide/html/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_HOME_DIRECTORY}/doc/simgrid.css                          ${CMAKE_HOME_DIRECTORY}/doc/user_guide/html/
    )

  ADD_CUSTOM_COMMAND(TARGET user_guide
    COMMAND ${FIG2DEV_PATH}/fig2dev -Lmap ${CMAKE_HOME_DIRECTORY}/doc/shared/fig/simgrid_modules.fig | perl -pe 's/imagemap/simgrid_modules/g'| perl -pe 's/<IMG/<IMG style=border:0px/g' | ${CMAKE_HOME_DIRECTORY}/tools/doxygen/fig2dev_postprocessor.pl > ${CMAKE_HOME_DIRECTORY}/doc/user_guide/doxygen/simgrid_modules.map
    COMMAND ${CMAKE_COMMAND} -E echo "XX First Doxygen pass"
    COMMAND ${DOXYGEN_PATH}/doxygen UserGuideDoxyfile

    COMMAND ${CMAKE_COMMAND} -E echo "XX Second Doxygen pass"
    COMMAND ${DOXYGEN_PATH}/doxygen UserGuideDoxyfile

    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_HOME_DIRECTORY}/doc/user_guide/html/dir*

    WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}/doc/user_guide/doxygen
    )

else(FIG2DEV_PATH)

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

endif(FIG2DEV_PATH)

##############################################################################"

#message(STATUS "Check individual TOCs")
#set(LISTE_GTUT
#	doc/user_guide/doxygen/gtut-tour-00-install.doc
#	doc/user_guide/doxygen/gtut-tour-01-bones.doc
#	doc/user_guide/doxygen/gtut-tour-02-simple.doc
#	doc/user_guide/doxygen/gtut-tour-03-args.doc
#	doc/user_guide/doxygen/gtut-tour-04-callback.doc
#	doc/user_guide/doxygen/gtut-tour-05-globals.doc
#	doc/user_guide/doxygen/gtut-tour-06-logs.doc
#	doc/user_guide/doxygen/gtut-tour-07-timers.doc
#	doc/user_guide/doxygen/gtut-tour-08-exceptions.doc
#	doc/user_guide/doxygen/gtut-tour-09-simpledata.doc
#	doc/user_guide/doxygen/gtut-tour-10-rpc.doc
#	doc/user_guide/doxygen/gtut-tour-11-explicitwait.doc
#	doc/user_guide/doxygen/gtut-tour-recap-messages.doc
#	doc/user_guide/doxygen/gtut-tour-12-staticstruct.doc
#	doc/user_guide/doxygen/gtut-tour-13-pointers.doc
#	doc/user_guide/doxygen/gtut-tour-14-dynar.doc
#	doc/user_guide/doxygen/gtut-tour-15-manualdatadef.doc
#	doc/user_guide/doxygen/gtut-tour-16-exchangecb.doc
#)
#
#foreach(file_name ${LISTE_GTUT})
#	file(REMOVE ${CMAKE_HOME_DIRECTORY}/doc/user_guide/doxygen/tmp.curtoc)
#	file(REMOVE ${CMAKE_HOME_DIRECTORY}/doc/user_guide/doxygen/tmp.realtoc)
#
#	file(READ "${file_name}" file_content)
#	string(REGEX MATCH "Table of Contents.*<hr>" valeur_line "${file_content}")
#	string(REPLACE "\n" ";" valeur_line "${valeur_line}")
#	string(REPLACE "\n" ";" file_content "${file_content}")
#
#	file(APPEND ${CMAKE_HOME_DIRECTORY}/doc/user_guide/doxygen/tmp.realtoc "\n") # make sure it exists
#	foreach(line ${file_content})
#		string(REGEX MATCH "[\\]s?u?b?s?u?b?section.*" line2 "${line}")
#		string(REGEX MATCH ".*_toc.*" line3 "${line}")
#		if(line2 AND NOT line3)
#			string(REPLACE "\\section " "" line2 ${line2})
#			string(REPLACE "\\subsection " "subsection" line2 ${line2})
#			string(REPLACE "\\subsubsection " "subsubsection" line2 ${line2})
#			string(REGEX REPLACE " .*" "" line2 ${line2})
#			set(line2                               " - \\ref ${line2}")
#			string(REPLACE " - \\ref subsection"    "   - \\ref " line2 ${line2})
#			string(REPLACE " - \\ref subsubsection" "     - \\ref " line2 ${line2})
#			file(APPEND ${CMAKE_HOME_DIRECTORY}/doc/user_guide/doxygen/tmp.realtoc "${line2}\n")
#		endif(line2 AND NOT line3)
#	endforeach(line ${file_content})
#
#	file(APPEND ${CMAKE_HOME_DIRECTORY}/doc/user_guide/doxygen/tmp.curtoc "\n") # make sure it exists
#	foreach(line ${valeur_line})
#		string(REGEX MATCH ".*ref.*" line_ok ${line})
#		if(line_ok)
#			file(APPEND ${CMAKE_HOME_DIRECTORY}/doc/user_guide/doxygen/tmp.curtoc "${line_ok}\n")
#		endif(line_ok)
#	endforeach(line ${valeur_line})
#
#	exec_program("${CMAKE_COMMAND} -E compare_files ${CMAKE_HOME_DIRECTORY}/doc/user_guide/doxygen/tmp.curtoc ${CMAKE_HOME_DIRECTORY}/doc/user_guide/doxygen/tmp.realtoc" OUTPUT_VARIABLE compare_files)
#	if(compare_files)
#		message(STATUS "Wrong toc for ${file_name}. Should be:")
#		file(READ "${CMAKE_HOME_DIRECTORY}/doc/user_guide/doxygen/tmp.realtoc" file_content)
#		message("${file_content}")
#		exec_program("diff -u ${CMAKE_HOME_DIRECTORY}/doc/user_guide/doxygen/tmp.curtoc ${CMAKE_HOME_DIRECTORY}/doc/user_guide/doxygen/tmp.realtoc")
#        endif(compare_files)
#endforeach(file_name ${LISTE_GTUT})
#
#file(REMOVE ${CMAKE_HOME_DIRECTORY}/doc/user_guide/doxygen/tmp.curtoc)
#file(REMOVE ${CMAKE_HOME_DIRECTORY}/doc/user_guide/doxygen/tmp.realtoc)
#
#message(STATUS "Check main TOC")
#
#foreach(file_name ${LISTE_GTUT})
#	file(READ "${file_name}" file_content)
#	string(REGEX MATCH "Table of Contents.*<hr>" valeur_line "${file_content}")
#	string(REPLACE "\n" ";" valeur_line "${valeur_line}")
#	string(REPLACE "\n" ";" file_content "${file_content}")
#
#	foreach(line ${file_content})
#		string(REGEX MATCH ".*@page.*" line2 "${line}")
#		if(line2)
#			string(REPLACE "@page " "" line2 "${line2}")
#			string(REGEX REPLACE " .*" "" line2 "${line2}")
#			set(line2 " - \\ref ${line2}")
#			file(APPEND ${CMAKE_HOME_DIRECTORY}/doc/user_guide/doxygen/tmp.realtoc "${line2}\n")
#		endif(line2)
#	endforeach(line ${file_content})
#
#	foreach(line ${valeur_line})
#		string(REGEX MATCH ".*toc.*" line1 "${line}")
#		string(REGEX MATCH ".*<hr>.*" line2 "${line}")
#		string(REGEX MATCH "^[ ]*$" line3 "${line}")
#		string(REGEX MATCH "Table of Contents" line4 "${line}")
#		if(NOT line1 AND NOT line2 AND NOT line3 AND NOT line4)
#			file(APPEND ${CMAKE_HOME_DIRECTORY}/doc/user_guide/doxygen/tmp.realtoc "   ${line}\n")
#		endif(NOT line1 AND NOT line2 AND NOT line3 AND NOT line4)
#	endforeach(line ${valeur_line})
#endforeach(file_name ${LISTE_GTUT})
#
#file(READ "${CMAKE_HOME_DIRECTORY}/doc/user_guide/doxygen/gtut-tour.doc" file_content)
#string(REPLACE "\n" ";" file_content "${file_content}")
#foreach(line ${file_content})
#	string(REGEX MATCH "^[ ]+.*\\ref" line1 "${line}")
#	if(line1)
#		file(APPEND ${CMAKE_HOME_DIRECTORY}/doc/user_guide/doxygen/tmp.curtoc "${line}\n")
#	endif(line1)
#endforeach(line ${file_content})
#
#exec_program("${CMAKE_COMMAND} -E compare_files ${CMAKE_HOME_DIRECTORY}/doc/user_guide/doxygen/tmp.curtoc ${CMAKE_HOME_DIRECTORY}/doc/user_guide/doxygen/tmp.realtoc" OUTPUT_VARIABLE compare_files)
#if(compare_files)
#	message(STATUS "Wrong toc for gtut-tour.doc Right one is in tmp.realtoc")
#	exec_program("diff -u ${CMAKE_HOME_DIRECTORY}/doc/user_guide/doxygen/tmp.curtoc ${CMAKE_HOME_DIRECTORY}/doc/user_guide/doxygen/tmp.realtoc")
#else(compare_files)
#	file(REMOVE ${CMAKE_HOME_DIRECTORY}/doc/user_guide/doxygen/tmp.realtoc)
#endif(compare_files)
#
#file(REMOVE ${CMAKE_HOME_DIRECTORY}/doc/user_guide/doxygen/tmp.curtoc)

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
