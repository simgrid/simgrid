#### Generate the html documentation

if(BIBTEX2HTML)
	set(BIBTEX2HTML_PATH ${BIBTEX2HTML})
else(BIBTEX2HTML)
	find_path(BIBTEX2HTML_PATH	NAMES bibtex2html	PATHS NO_DEFAULT_PATHS)
endif(BIBTEX2HTML)

find_path(FIG2DEV_PATH	NAMES fig2dev	PATHS NO_DEFAULT_PATHS)
find_path(DOXYGEN_PATH	NAMES doxygen	PATHS NO_DEFAULT_PATHS)

### Check whether the bibtex2html that we found is the one that Arnaud requires
exec_program("${BIBTEX2HTML_PATH}/bibtex2html -version" OUTPUT_VARIABLE OUTPUT_BIBTEX2HTML_VERSION)
STRING(REPLACE "[-bibtex]" "" OUTPUT_BIBTEX2HTML_VERSION_2 ${OUTPUT_BIBTEX2HTML_VERSION})

file(GLOB_RECURSE source_doxygen
	"${CMAKE_HOME_DIRECTORY}/tools/gras/*.[chl]"
	"${CMAKE_HOME_DIRECTORY}/src/*.[chl]"
	"${CMAKE_HOME_DIRECTORY}/include/*.[chl]"
)

if(${OUTPUT_BIBTEX2HTML_VERSION_2} STREQUAL ${OUTPUT_BIBTEX2HTML_VERSION}) # wrong version
	SET(GOOD_BIBTEX2HTML_VERSION 0)
else(${OUTPUT_BIBTEX2HTML_VERSION_2} STREQUAL ${OUTPUT_BIBTEX2HTML_VERSION}) # good version
	SET(GOOD_BIBTEX2HTML_VERSION 1)
endif(${OUTPUT_BIBTEX2HTML_VERSION_2} STREQUAL ${OUTPUT_BIBTEX2HTML_VERSION})

if(DOXYGEN_PATH AND FIG2DEV_PATH AND BIBTEX2HTML_PATH AND GOOD_BIBTEX2HTML_VERSION)

	#DOC_SOURCE=doc/*.doc, defined in DefinePackage
	set(DOCSSOURCES "${source_doxygen}\n${DOC_SOURCE}")
	string(REPLACE "\n" ";" DOCSSOURCES ${DOCSSOURCES})


	set(DOC_PNGS 
		${CMAKE_HOME_DIRECTORY}/doc/webcruft/simgrid_logo.png
		${CMAKE_HOME_DIRECTORY}/doc/webcruft/simgrid_logo_2011.png
		${CMAKE_HOME_DIRECTORY}/doc/webcruft/simgrid_logo_small.png
		${CMAKE_HOME_DIRECTORY}/doc/webcruft/poster_thumbnail.png
		${CMAKE_HOME_DIRECTORY}/doc/webcruft/win_install_01.png
		${CMAKE_HOME_DIRECTORY}/doc/webcruft/win_install_02.png
		${CMAKE_HOME_DIRECTORY}/doc/webcruft/win_install_03.png
		${CMAKE_HOME_DIRECTORY}/doc/webcruft/win_install_04.png
		${CMAKE_HOME_DIRECTORY}/doc/webcruft/win_install_05.png
		${CMAKE_HOME_DIRECTORY}/doc/webcruft/win_install_06.png
	)
	
	configure_file(${CMAKE_HOME_DIRECTORY}/doc/Doxyfile.in ${CMAKE_HOME_DIRECTORY}/doc/Doxyfile @ONLY)
	configure_file(${CMAKE_HOME_DIRECTORY}/doc/footer.html.in ${CMAKE_HOME_DIRECTORY}/doc/footer.html @ONLY)		
	
	ADD_CUSTOM_TARGET(simgrid_documentation
		COMMENT "Generating the SimGrid documentation..."
		DEPENDS ${DOC_SOURCES} ${DOC_FIGS} ${source_doxygen}
		COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_HOME_DIRECTORY}/doc/html
	    COMMAND ${CMAKE_COMMAND} -E make_directory   ${CMAKE_HOME_DIRECTORY}/doc/html
		COMMAND ${FIG2DEV_PATH}/fig2dev -Lmap ${CMAKE_HOME_DIRECTORY}/doc/fig/simgrid_modules.fig | perl -pe 's/imagemap/simgrid_modules/g'| perl -pe 's/<IMG/<IMG style=border:0px/g' | ${CMAKE_HOME_DIRECTORY}/tools/doxygen/fig2dev_postprocessor.pl > ${CMAKE_HOME_DIRECTORY}/doc/simgrid_modules.map
		WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}/doc
	)
		
	ADD_CUSTOM_COMMAND(
		OUTPUT ${CMAKE_HOME_DIRECTORY}/doc/logcategories.doc
		DEPENDS ${source_doxygen}
		COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_HOME_DIRECTORY}/doc/logcategories.doc
		COMMAND ${CMAKE_HOME_DIRECTORY}/tools/doxygen/xbt_log_extract_hierarchy.pl > ${CMAKE_HOME_DIRECTORY}/doc/logcategories.doc
		WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}
	)

	foreach(file ${DOC_FIGS})
		string(REPLACE ".fig" ".png" tmp_file ${file})
		string(REPLACE "${CMAKE_HOME_DIRECTORY}/doc/fig/" "${CMAKE_HOME_DIRECTORY}/doc/html/" tmp_file ${tmp_file})
		ADD_CUSTOM_COMMAND(TARGET simgrid_documentation
			COMMAND ${FIG2DEV_PATH}/fig2dev -Lpng ${file} ${tmp_file}
		)
	endforeach(file ${DOC_FIGS})
	
	foreach(file ${DOC_PNGS})
		ADD_CUSTOM_COMMAND(TARGET simgrid_documentation
			COMMAND ${CMAKE_COMMAND} -E copy ${file} ${CMAKE_HOME_DIRECTORY}/doc/html/
		)
	endforeach(file ${DOC_PNGS})

	ADD_CUSTOM_COMMAND(TARGET simgrid_documentation
		COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_HOME_DIRECTORY}/doc/webcruft/Paje_MSG_screenshot_thn.jpg ${CMAKE_HOME_DIRECTORY}/doc/html/
		COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_HOME_DIRECTORY}/doc/webcruft/Paje_MSG_screenshot.jpg     ${CMAKE_HOME_DIRECTORY}/doc/html/
		COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_HOME_DIRECTORY}/doc/triva-graph_configuration.png        ${CMAKE_HOME_DIRECTORY}/doc/html/
		COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_HOME_DIRECTORY}/doc/triva-graph_visualization.png        ${CMAKE_HOME_DIRECTORY}/doc/html/
		COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_HOME_DIRECTORY}/doc/AS_hierarchy.png        ${CMAKE_HOME_DIRECTORY}/doc/html/		
		COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_HOME_DIRECTORY}/doc/simgrid.css                          ${CMAKE_HOME_DIRECTORY}/doc/html/
	)
	
	ADD_CUSTOM_COMMAND(TARGET simgrid_documentation
	    COMMAND ${CMAKE_COMMAND} -E echo "XX First Doxygen pass"
		COMMAND ${DOXYGEN_PATH}/doxygen Doxyfile
		COMMAND ${CMAKE_HOME_DIRECTORY}/tools/doxygen/index_create.pl simgrid.tag index-API.doc
		COMMAND ${CMAKE_HOME_DIRECTORY}/tools/doxygen/toc_create.pl pls.doc index.doc FAQ.doc gtut-introduction.doc install.doc bindings.doc options.doc tracing.doc platform.doc
		
		COMMAND ${CMAKE_COMMAND} -E echo "XX Second Doxygen pass"
		COMMAND ${DOXYGEN_PATH}/doxygen Doxyfile
		
		COMMAND ${CMAKE_COMMAND} -E echo "XX Post-processing Doxygen result"
		COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_HOME_DIRECTORY}/doc/html/dir*
		COMMAND ${CMAKE_HOME_DIRECTORY}/tools/doxygen/doxygen_postprocesser.pl
		
		COMMAND ${CMAKE_COMMAND} -E echo "XX Create shortcuts pages"
		COMMAND ${CMAKE_COMMAND} -E echo \"<html><META HTTP-EQUIV='Refresh' content='0;URL=http://simgrid.gforge.inria.fr/doc/group__GRAS__API.html'>\" > ${CMAKE_HOME_DIRECTORY}/doc/html/gras.html
		COMMAND ${CMAKE_COMMAND} -E echo \"<center><h2><br><a href='http://simgrid.gforge.inria.fr/doc/group__GRAS__API.html'>Grid Reality And Simulation.</a></h2></center></html>\" >> ${CMAKE_HOME_DIRECTORY}/doc/html/gras.html
		
		COMMAND ${CMAKE_COMMAND} -E echo \"<html><META HTTP-EQUIV='Refresh' content='0;URL=http://simgrid.gforge.inria.fr/doc/group__AMOK__API.html'>\" > ${CMAKE_HOME_DIRECTORY}/doc/html/amok.html
		COMMAND ${CMAKE_COMMAND} -E echo \"<center><h2><br><a href='http://simgrid.gforge.inria.fr/doc/group__AMOK__API.html'>Advanced Metacomputing Overlay Kit.</a></h2></center></html>\" >> ${CMAKE_HOME_DIRECTORY}/doc/html/amok.html
	
		COMMAND ${CMAKE_COMMAND} -E echo \"<html><META HTTP-EQUIV='Refresh' content='0;URL=http://simgrid.gforge.inria.fr/doc/group__MSG__API.html'>\" > ${CMAKE_HOME_DIRECTORY}/doc/html/msg.html
		COMMAND ${CMAKE_COMMAND} -E echo \"<center><h2><br><a href='http://simgrid.gforge.inria.fr/doc/group__MSG__API.html'>Meta SimGrid.</a></h2></center></html>\" >> ${CMAKE_HOME_DIRECTORY}/doc/html/msg.html
	
		COMMAND ${CMAKE_COMMAND} -E echo \"<html><META HTTP-EQUIV='Refresh' content='0;URL=http://simgrid.gforge.inria.fr/doc/group__SD__API.html'>\" > ${CMAKE_HOME_DIRECTORY}/doc/html/simdag.html
		COMMAND ${CMAKE_COMMAND} -E echo \"<center><h2><br><a href='http://simgrid.gforge.inria.fr/doc/group__SD__API.html'>DAG Simulator.</a></h2></center></html>\" >> ${CMAKE_HOME_DIRECTORY}/doc/html/simdag.html
		WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}/doc/
	)
	
else(DOXYGEN_PATH AND FIG2DEV_PATH AND BIBTEX2HTML_PATH AND GOOD_BIBTEX2HTML_VERSION)

	ADD_CUSTOM_TARGET(simgrid_documentation
			COMMENT "Generating the SimGrid documentation..."
			)

	if(NOT GOOD_BIBTEX2HTML_VERSION) # wrong version
		ADD_CUSTOM_COMMAND(TARGET simgrid_documentation
			COMMAND ${CMAKE_COMMAND} -E echo "This is not the good bibtex2html program !!!"
			COMMAND ${CMAKE_COMMAND} -E echo  "You can download it from:"
			COMMAND ${CMAKE_COMMAND} -E echo  "  ftp://ftp-sop.inria.fr/epidaure/Softs/bibtex2html/bibtex2html-1.02.tar.gz"
			COMMAND ${CMAKE_COMMAND} -E echo  "There is also an unofficial Debian/Ubuntu package, see:"
		        COMMAND ${CMAKE_COMMAND} -E echo  "  http://www.loria.fr/~lnussbau/bibtex2html/README"
			)
	endif(NOT GOOD_BIBTEX2HTML_VERSION)

	ADD_CUSTOM_COMMAND(TARGET simgrid_documentation
			COMMAND ${CMAKE_COMMAND} -E echo "DOXYGEN_PATH 		= ${DOXYGEN_PATH}"
			COMMAND ${CMAKE_COMMAND} -E echo "FIG2DEV_PATH 		= ${FIG2DEV_PATH}"
			COMMAND ${CMAKE_COMMAND} -E echo "BIBTEX2HTML_PATH 	= ${BIBTEX2HTML_PATH}"
			COMMAND ${CMAKE_COMMAND} -E echo "IN ORDER TO GENERATE THE DOCUMENTATION YOU NEED ALL TOOLS !!!"
			COMMAND ${CMAKE_COMMAND} -E echo "FAIL TO MAKE SIMGRID DOCUMENTATION see previous messages for details ..."
			COMMAND false
			)

		
endif(DOXYGEN_PATH AND FIG2DEV_PATH AND BIBTEX2HTML_PATH AND GOOD_BIBTEX2HTML_VERSION)

##############################################################################"

message(STATUS "Check individual TOCs")
set(LISTE_GTUT
	doc/gtut-tour-00-install.doc
	doc/gtut-tour-01-bones.doc
	doc/gtut-tour-02-simple.doc
	doc/gtut-tour-03-args.doc
	doc/gtut-tour-04-callback.doc
	doc/gtut-tour-05-globals.doc
	doc/gtut-tour-06-logs.doc
	doc/gtut-tour-07-timers.doc
	doc/gtut-tour-08-exceptions.doc
	doc/gtut-tour-09-simpledata.doc
	doc/gtut-tour-10-rpc.doc
	doc/gtut-tour-11-explicitwait.doc
	doc/gtut-tour-recap-messages.doc
	doc/gtut-tour-12-staticstruct.doc
	doc/gtut-tour-13-pointers.doc
	doc/gtut-tour-14-dynar.doc
	doc/gtut-tour-15-manualdatadef.doc
	doc/gtut-tour-16-exchangecb.doc
)

foreach(file_name ${LISTE_GTUT})
	file(REMOVE ${CMAKE_HOME_DIRECTORY}/doc/tmp.curtoc)
	file(REMOVE ${CMAKE_HOME_DIRECTORY}/doc/tmp.realtoc)
	
	file(READ "${file_name}" file_content)
	string(REGEX MATCH "Table of Contents.*<hr>" valeur_line "${file_content}")
	string(REPLACE "\n" ";" valeur_line "${valeur_line}")
	string(REPLACE "\n" ";" file_content "${file_content}")
	       
	file(APPEND ${CMAKE_HOME_DIRECTORY}/doc/tmp.realtoc "\n") # make sure it exists
	foreach(line ${file_content})
		string(REGEX MATCH "[\\]s?u?b?s?u?b?section.*" line2 "${line}")
		string(REGEX MATCH ".*_toc.*" line3 "${line}")
		if(line2 AND NOT line3)
			string(REPLACE "\\section " "" line2 ${line2})
			string(REPLACE "\\subsection " "subsection" line2 ${line2})
			string(REPLACE "\\subsubsection " "subsubsection" line2 ${line2})
			string(REGEX REPLACE " .*" "" line2 ${line2})
			set(line2                               " - \\ref ${line2}")
			string(REPLACE " - \\ref subsection"    "   - \\ref " line2 ${line2})
			string(REPLACE " - \\ref subsubsection" "     - \\ref " line2 ${line2})
			file(APPEND ${CMAKE_HOME_DIRECTORY}/doc/tmp.realtoc "${line2}\n")
		endif(line2 AND NOT line3)
	endforeach(line ${file_content})
	
	file(APPEND ${CMAKE_HOME_DIRECTORY}/doc/tmp.curtoc "\n") # make sure it exists
	foreach(line ${valeur_line})
		string(REGEX MATCH ".*ref.*" line_ok ${line})
		if(line_ok)
			file(APPEND ${CMAKE_HOME_DIRECTORY}/doc/tmp.curtoc "${line_ok}\n")
		endif(line_ok)
	endforeach(line ${valeur_line})
	
	exec_program("${CMAKE_COMMAND} -E compare_files ${CMAKE_HOME_DIRECTORY}/doc/tmp.curtoc ${CMAKE_HOME_DIRECTORY}/doc/tmp.realtoc" OUTPUT_VARIABLE compare_files)
	if(compare_files)
		message(STATUS "Wrong toc for ${file_name}. Should be:")
		file(READ "${CMAKE_HOME_DIRECTORY}/doc/tmp.realtoc" file_content)
		message("${file_content}")
		exec_program("diff -u ${CMAKE_HOME_DIRECTORY}/doc/tmp.curtoc ${CMAKE_HOME_DIRECTORY}/doc/tmp.realtoc")
        endif(compare_files)
endforeach(file_name ${LISTE_GTUT})

file(REMOVE ${CMAKE_HOME_DIRECTORY}/doc/tmp.curtoc)
file(REMOVE ${CMAKE_HOME_DIRECTORY}/doc/tmp.realtoc)

message(STATUS "Check main TOC")

foreach(file_name ${LISTE_GTUT})
	file(READ "${file_name}" file_content)	
	string(REGEX MATCH "Table of Contents.*<hr>" valeur_line "${file_content}")
	string(REPLACE "\n" ";" valeur_line "${valeur_line}")
	string(REPLACE "\n" ";" file_content "${file_content}")
	
	foreach(line ${file_content})
		string(REGEX MATCH ".*@page.*" line2 "${line}")
		if(line2)
			string(REPLACE "@page " "" line2 "${line2}")
			string(REGEX REPLACE " .*" "" line2 "${line2}")
			set(line2 " - \\ref ${line2}")
			file(APPEND ${CMAKE_HOME_DIRECTORY}/doc/tmp.realtoc "${line2}\n")
		endif(line2)
	endforeach(line ${file_content})
	
	foreach(line ${valeur_line})
		string(REGEX MATCH ".*toc.*" line1 "${line}")
		string(REGEX MATCH ".*<hr>.*" line2 "${line}")
		string(REGEX MATCH "^[ ]*$" line3 "${line}")
		string(REGEX MATCH "Table of Contents" line4 "${line}")
		if(NOT line1 AND NOT line2 AND NOT line3 AND NOT line4)
			file(APPEND ${CMAKE_HOME_DIRECTORY}/doc/tmp.realtoc "   ${line}\n")
		endif(NOT line1 AND NOT line2 AND NOT line3 AND NOT line4)
	endforeach(line ${valeur_line})
endforeach(file_name ${LISTE_GTUT})	

file(READ "${CMAKE_HOME_DIRECTORY}/doc/gtut-tour.doc" file_content)
string(REPLACE "\n" ";" file_content "${file_content}")
foreach(line ${file_content})
	string(REGEX MATCH "^[ ]+.*\\ref" line1 "${line}")
	if(line1)
		file(APPEND ${CMAKE_HOME_DIRECTORY}/doc/tmp.curtoc "${line}\n")
	endif(line1)
endforeach(line ${file_content})
	
exec_program("${CMAKE_COMMAND} -E compare_files ${CMAKE_HOME_DIRECTORY}/doc/tmp.curtoc ${CMAKE_HOME_DIRECTORY}/doc/tmp.realtoc" OUTPUT_VARIABLE compare_files)
if(compare_files)
	message(STATUS "Wrong toc for gtut-tour.doc Right one is in tmp.realtoc")
	exec_program("diff -u ${CMAKE_HOME_DIRECTORY}/doc/tmp.curtoc ${CMAKE_HOME_DIRECTORY}/doc/tmp.realtoc")
else(compare_files)
	file(REMOVE ${CMAKE_HOME_DIRECTORY}/doc/tmp.realtoc)
endif(compare_files)	
  
file(REMOVE ${CMAKE_HOME_DIRECTORY}/doc/tmp.curtoc)

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
