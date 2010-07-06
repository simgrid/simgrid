message("\tBIBTEX2HTML  : ${BIBTEX2HTML}")

if(BIBTEX2HTML)
set(BIBTEX2HTML_PATH ${BIBTEX2HTML})
else(BIBTEX2HTML)
find_path(BIBTEX2HTML_PATH	NAMES bibtex2html	PATHS NO_DEFAULT_PATHS)
endif(BIBTEX2HTML)

find_path(FIG2DEV_PATH	NAMES fig2dev	PATHS NO_DEFAULT_PATHS)
find_path(DOXYGEN_PATH	NAMES doxygen	PATHS NO_DEFAULT_PATHS)
find_path(BIBTOOL_PATH	NAMES bibtool	PATHS NO_DEFAULT_PATHS)
find_path(ICONV_PATH	NAMES iconv	PATHS NO_DEFAULT_PATHS)

message("\tFIG2DEV_PATH : ${FIG2DEV_PATH}")
message("\tDOXYGEN_PATH : ${DOXYGEN_PATH}")
message("\tBIBTOOL_PATH : ${BIBTOOL_PATH}")
message("\tICONV_PATH   : ${ICONV_PATH}")
message("\tBIBTEX2HTML_PATH   : ${BIBTEX2HTML_PATH}")

exec_program("${BIBTEX2HTML_PATH}/bibtex2html -version" OUTPUT_VARIABLE SORTIE_BIBTEX2HTML_VERSION)
STRING(REPLACE "[-bibtex]" "" SORTIE_BIBTEX2HTML_VERSION_2 ${SORTIE_BIBTEX2HTML_VERSION})

if(BIBTEX2HTML_PATH)
	if(${SORTIE_BIBTEX2HTML_VERSION_2} STREQUAL ${SORTIE_BIBTEX2HTML_VERSION}) # mauvaise version
		message("\nERROR --> NEED to set bibtex2html path with \"ccmake ./\" or with \"cmake -DBIBTEX2HTML=<path_to> ./\"")
		message("\nTake care having install the good bibtex2html \n\t(download it : ftp://ftp-sop.inria.fr/epidaure/Softs/bibtex2html/bibtex2html-1.02.tar.gz)")
		message(FATAL_ERROR "\n")
	endif(${SORTIE_BIBTEX2HTML_VERSION_2} STREQUAL ${SORTIE_BIBTEX2HTML_VERSION})
endif(BIBTEX2HTML_PATH)

exec_program("${CMAKE_COMMAND} -E remove_directory ${PROJECT_DIRECTORY}/doc/html"  "${PROJECT_DIRECTORY}/doc/")
exec_program("${CMAKE_COMMAND} -E make_directory ${PROJECT_DIRECTORY}/doc/html"  "${PROJECT_DIRECTORY}/doc/")

file(GLOB_RECURSE LISTE_UNE
"${PROJECT_DIRECTORY}/tools/gras/*.[chl]"
"${PROJECT_DIRECTORY}/src/*.[chl]"
"${PROJECT_DIRECTORY}/include/*.[chl]"
)

file(GLOB_RECURSE LISTE_DEUX
"${PROJECT_DIRECTORY}/*.doc"
)
string(REGEX REPLACE ";.*logcategories.doc" "" LISTE_DEUX "${LISTE_DEUX}")

set(DOCSSOURCES "${LISTE_UNE}\n${LISTE_DEUX}")
string(REPLACE "\n" ";" DOCSSOURCES ${DOCSSOURCES})

set(FIGS
${PROJECT_DIRECTORY}/doc/fig/simgrid_modules.fig
${PROJECT_DIRECTORY}/doc/fig/simgrid_modules2.fig
${PROJECT_DIRECTORY}/doc/fig/amok_bw_test.fig
${PROJECT_DIRECTORY}/doc/fig/amok_bw_sat.fig
${PROJECT_DIRECTORY}/doc/fig/gras_comm.fig
)

set(PNGS 
${PROJECT_DIRECTORY}/doc/webcruft/simgrid_logo.png
${PROJECT_DIRECTORY}/doc/webcruft/simgrid_logo_small.png
${PROJECT_DIRECTORY}/doc/webcruft/poster_thumbnail.png
)

if(DOXYGEN_PATH AND FIG2DEV_PATH)
	
	exec_program("${FIG2DEV_PATH}/fig2dev -Lmap ${PROJECT_DIRECTORY}/doc/fig/simgrid_modules.fig" OUTPUT_VARIABLE output_fig2dev)
	string(REPLACE "\n" ";" output_fig2dev "${output_fig2dev}")
	
	file(REMOVE ${PROJECT_DIRECTORY}/doc/simgrid_modules.map)	

	foreach(line ${output_fig2dev})
		string(REGEX MATCH "IMG" test_oki1 "${line}")
		string(REGEX MATCH "MAP" test_oki2 "${line}")
		string(REGEX MATCH "AREA" test_oki3 "${line}")
		if(test_oki1 OR test_oki2 OR test_oki3)
			string(REPLACE "imagemap"	"simgrid_modules" line "${line}")
			string(REPLACE ".gif"	".png" line "${line}")
			string(REPLACE "<IMG"	"<IMG style=\"border:0px\""  line "${line}")
		file(APPEND ${PROJECT_DIRECTORY}/doc/simgrid_modules.map "${line}\n")
		endif(test_oki1 OR test_oki2 OR test_oki3)	
	endforeach(line ${output_fig2dev})

	foreach(file ${FIGS})
		string(REPLACE ".fig" ".png" tmp_file ${file})
		string(REPLACE "${PROJECT_DIRECTORY}/doc/fig/" "${PROJECT_DIRECTORY}/doc/html/" tmp_file ${tmp_file})
		exec_program("${FIG2DEV_PATH}/fig2dev -Lpng ${file} > ${tmp_file}"  "${PROJECT_DIRECTORY}/doc/")
	endforeach(file ${FIGS})


	exec_program("${CMAKE_COMMAND} -E touch ${PROJECT_DIRECTORY}/doc/index-API.doc ${PROJECT_DIRECTORY}/doc/.FAQ.doc.toc ${PROJECT_DIRECTORY}/doc/.index.doc.toc ${PROJECT_DIRECTORY}/doc/.contrib.doc.toc ${PROJECT_DIRECTORY}/doc/.history.doc.toc"  "${PROJECT_DIRECTORY}/doc/")
	
	if(NOT EXISTS ${PROJECT_DIRECTORY}/doc/html)
		file(MAKE_DIRECTORY ${PROJECT_DIRECTORY}/doc/html)
	endif(NOT EXISTS ${PROJECT_DIRECTORY}/doc/html)

	foreach(file ${PNGS})
		exec_program("${CMAKE_COMMAND} -E copy ${file} ${PROJECT_DIRECTORY}/doc/html/"  "${PROJECT_DIRECTORY}/doc/")
	endforeach(file ${PNGS})

	exec_program("${CMAKE_COMMAND} -E copy ${PROJECT_DIRECTORY}/doc/webcruft/Paje_MSG_screenshot_thn.jpg ${PROJECT_DIRECTORY}/doc/webcruft/Paje_MSG_screenshot.jpg ${PROJECT_DIRECTORY}/doc/html/"  "${PROJECT_DIRECTORY}/doc/")
	exec_program("${CMAKE_COMMAND} -E copy ${PROJECT_DIRECTORY}/doc/triva-time_interval.png ${PROJECT_DIRECTORY}/doc/html/"  "${PROJECT_DIRECTORY}/doc/")
	exec_program("${CMAKE_COMMAND} -E copy ${PROJECT_DIRECTORY}/doc/triva-graph_configuration.png ${PROJECT_DIRECTORY}/doc/html/"  "${PROJECT_DIRECTORY}/doc/")
	exec_program("${CMAKE_COMMAND} -E copy ${PROJECT_DIRECTORY}/doc/triva-graph_visualization.png ${PROJECT_DIRECTORY}/doc/html/"  "${PROJECT_DIRECTORY}/doc/")
	exec_program("${CMAKE_COMMAND} -E copy ${PROJECT_DIRECTORY}/doc/simgrid.css ${PROJECT_DIRECTORY}/doc/html/"  "${PROJECT_DIRECTORY}/doc/") 

	set(top_srcdir "..")
	set(WARNING "This file is generated, do not edit")
	set(srcdir ".")
	configure_file(${PROJECT_DIRECTORY}/doc/Doxyfile.in ${PROJECT_DIRECTORY}/doc/Doxyfile @ONLY)
# First pass
	exec_program("${DOXYGEN_PATH}/doxygen ${PROJECT_DIRECTORY}/doc/Doxyfile" "${PROJECT_DIRECTORY}/doc/")

	exec_program("${PROJECT_DIRECTORY}/tools/doxygen/index_create.pl" "${PROJECT_DIRECTORY}/doc/"
	ARGS simgrid.tag index-API.doc)
	exec_program("${PROJECT_DIRECTORY}/tools/doxygen/toc_create.pl" "${PROJECT_DIRECTORY}/doc/"
	ARGS FAQ.doc index.doc contrib.doc gtut-introduction.doc history.doc)
# Second pass
	exec_program("${DOXYGEN_PATH}/doxygen ${PROJECT_DIRECTORY}/doc/Doxyfile" "${PROJECT_DIRECTORY}/doc/")
# Post-processing

	exec_program("${CMAKE_COMMAND} -E remove -f ${PROJECT_DIRECTORY}/doc/html/dir*"  "${PROJECT_DIRECTORY}/doc/")

	exec_program("${PROJECT_DIRECTORY}/tools/doxygen/index_php.pl""${PROJECT_DIRECTORY}/doc/"
	ARGS index.php.in html/index.html index.php)
	exec_program("${PROJECT_DIRECTORY}/tools/doxygen/doxygen_postprocesser.pl"  "${PROJECT_DIRECTORY}/doc/")

#html/gras.html:
	exec_program("${CMAKE_COMMAND} -E echo \"<html><META HTTP-EQUIV='Refresh' content='0;URL=http://simgrid.gforge.inria.fr/doc/group__GRAS__API.html'>\" > ${PROJECT_DIRECTORY}/doc/html/gras.html"  "${PROJECT_DIRECTORY}/doc/")
	exec_program("${CMAKE_COMMAND} -E echo \"<center><h2><br><a href='http://simgrid.gforge.inria.fr/doc/group__GRAS__API.html'>Grid Reality And Simulation.</a></h2></center></html>\" >> ${PROJECT_DIRECTORY}/doc/html/gras.html"  "${PROJECT_DIRECTORY}/doc/")
#html/amok.html:
	exec_program("${CMAKE_COMMAND} -E echo \"<html><META HTTP-EQUIV='Refresh' content='0;URL=http://simgrid.gforge.inria.fr/doc/group__AMOK__API.html'>\" > ${PROJECT_DIRECTORY}/doc/html/amok.html"  "${PROJECT_DIRECTORY}/doc/")
	exec_program("${CMAKE_COMMAND} -E echo \"<center><h2><br><a href='http://simgrid.gforge.inria.fr/doc/group__AMOK__API.html'>Advanced Metacomputing Overlay Kit.</a></h2></center></html>\" >> ${PROJECT_DIRECTORY}/doc/html/amok.html"  "${PROJECT_DIRECTORY}/doc/")
#html/msg.html:
	exec_program("${CMAKE_COMMAND} -E echo \"<html><META HTTP-EQUIV='Refresh' content='0;URL=http://simgrid.gforge.inria.fr/doc/group__MSG__API.html'>\" > ${PROJECT_DIRECTORY}/doc/html/msg.html"  "${PROJECT_DIRECTORY}/doc/")
	exec_program("${CMAKE_COMMAND} -E echo \"<center><h2><br><a href='http://simgrid.gforge.inria.fr/doc/group__MSG__API.html'>Meta SimGrid.</a></h2></center></html>\" >> ${PROJECT_DIRECTORY}/doc/html/msg.html"  "${PROJECT_DIRECTORY}/doc/")
#html/simdag.html:
	exec_program("${CMAKE_COMMAND} -E echo \"<html><META HTTP-EQUIV='Refresh' content='0;URL=http://simgrid.gforge.inria.fr/doc/group__SD__API.html'>\" > ${PROJECT_DIRECTORY}/doc/html/simdag.html"  "${PROJECT_DIRECTORY}/doc/")
	exec_program("${CMAKE_COMMAND} -E echo \"<center><h2><br><a href='http://simgrid.gforge.inria.fr/doc/group__SD__API.html'>DAG Simulator.</a></h2></center></html>\" >> ${PROJECT_DIRECTORY}/doc/html/simdag.html"  "${PROJECT_DIRECTORY}/doc/")

if(BIBTOOL_PATH AND BIBTEX2HTML_PATH AND ICONV_PATH)

#publis_count.html: all.bib
	exec_program("${PROJECT_DIRECTORY}/tools/doxygen/bibtex2html_table_count.pl < ${PROJECT_DIRECTORY}/doc/all.bib > ${PROJECT_DIRECTORY}/doc/publis_count.html"  "${PROJECT_DIRECTORY}/doc/")

#publis_core.bib: all.bib 
	exec_program("${BIBTOOL_PATH}/bibtool -- 'select.by.string={category \"core\"}' -- 'preserve.key.case={on}' -- 'preserve.keys={on}' ${PROJECT_DIRECTORY}/doc/all.bib -o ${PROJECT_DIRECTORY}/doc/publis_core.bib"  "${PROJECT_DIRECTORY}/doc/")

#publis_extern.bib: all.bib 
	exec_program("${BIBTOOL_PATH}/bibtool -- 'select.by.string={category \"extern\"}' -- 'preserve.key.case={on}' -- 'preserve.keys={on}' ${PROJECT_DIRECTORY}/doc/all.bib -o ${PROJECT_DIRECTORY}/doc/publis_extern.bib"  "${PROJECT_DIRECTORY}/doc/")

#publis_intra.bib: all.bib 
	exec_program("${BIBTOOL_PATH}/bibtool -- 'select.by.string={category \"intra\"}' -- 'preserve.key.case={on}' -- 'preserve.keys={on}' ${PROJECT_DIRECTORY}/doc/all.bib -o ${PROJECT_DIRECTORY}/doc/publis_intra.bib"  "${PROJECT_DIRECTORY}/doc/")

#%_bib.latin1.html: %.bib
	file(GLOB_RECURSE LISTE_QUATRE
	"${PROJECT_DIRECTORY}/doc/*.bib"
	)
	foreach(file ${LISTE_QUATRE})
		string(REPLACE ".bib" "_bib.latin1.html" file_tmp "${file}")
		string(REPLACE ".html" ".html.tmp" file_tmp2 "${file_tmp}")
		exec_program("${BIBTEX2HTML_PATH}/bibtex2html -single-output -nv -force -sort year -copy-icons ${file} -output ${file_tmp2} 2>&1" "${PROJECT_DIRECTORY}/doc/") 
		file(READ ${file_tmp2} READ_TMP)
		file(REMOVE ${file_tmp2})
		string(REPLACE "\n" ";" READ_TMP ${READ_TMP})
		
		foreach(line ${READ_TMP})
			string(REGEX MATCH "WARNING: unknown field type" line1 ${line})
			if(NOT line1)
				file(APPEND   ${file_tmp2} "${line}\n")
			endif(NOT line1)
		endforeach(line ${READ_TMP})
		exec_program("${PROJECT_DIRECTORY}/tools/doxygen/bibtex2html_postprocessor.pl < ${file_tmp2} > ${file_tmp}"  "${PROJECT_DIRECTORY}/doc/")
	endforeach(file ${LISTE_QUATRE})


#%_bib.html: %_bib.latin1.html
	file(GLOB_RECURSE LISTE_CINQ
	"${PROJECT_DIRECTORY}/doc/*_bib.latin1.html"
	)
	foreach(file ${LISTE_CINQ})
		string(REPLACE "_bib.latin1.html" "_bib.html" file_tmp "${file}")
		exec_program("${ICONV_PATH}/iconv --from-code latin1 --to-code utf8 ${file} --output ${file_tmp}"  "${PROJECT_DIRECTORY}/doc/")
	endforeach(file ${LISTE_CINQ})
endif(BIBTOOL_PATH AND BIBTEX2HTML_PATH AND ICONV_PATH)

endif(DOXYGEN_PATH AND FIG2DEV_PATH)

file(REMOVE ${PROJECT_DIRECTORY}/doc/logcategories.doc)
file(APPEND ${PROJECT_DIRECTORY}/doc/logcategories.doc "/* Generated file, do not edit */\n")
file(APPEND ${PROJECT_DIRECTORY}/doc/logcategories.doc "/** \\addtogroup XBT_log_cats\n")
file(APPEND ${PROJECT_DIRECTORY}/doc/logcategories.doc "	@{\n")
exec_program("${PROJECT_DIRECTORY}/tools/doxygen/xbt_log_extract_hierarchy.pl" "${PROJECT_DIRECTORY}/src" OUTPUT_VARIABLE output_log_extract_hierarchy)
file(APPEND ${PROJECT_DIRECTORY}/doc/logcategories.doc "${output_log_extract_hierarchy}\n")
file(APPEND ${PROJECT_DIRECTORY}/doc/logcategories.doc 	"@}*/")

file(WRITE ${PROJECT_DIRECTORY}/doc/realtoc.sh 	"\#! /bin/sh")

message("Check individual TOCs")
file(GLOB_RECURSE LISTE_GTUT
"${PROJECT_DIRECTORY}/doc/gtut-tour-*.doc"
)



foreach(file_name ${LISTE_GTUT})
	file(REMOVE ${PROJECT_DIRECTORY}/doc/tmp.curtoc)
	file(REMOVE ${PROJECT_DIRECTORY}/doc/tmp.realtoc)
	
	file(READ "${file_name}" file_content)
	string(REGEX MATCH "Table of Contents.*<hr>" valeur_line "${file_content}")
	string(REPLACE "\n" ";" valeur_line "${valeur_line}")
	string(REPLACE "\n" ";" file_content "${file_content}")
	
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
			file(APPEND ${PROJECT_DIRECTORY}/doc/tmp.realtoc "${line2}\n")
		endif(line2 AND NOT line3)
	endforeach(line ${file_content})
	
	foreach(line ${valeur_line})
		string(REGEX MATCH ".*ref.*" line_ok ${line})
		if(line_ok)
			file(APPEND ${PROJECT_DIRECTORY}/doc/tmp.curtoc "${line_ok}\n")
		endif(line_ok)
	endforeach(line ${valeur_line})
	
	exec_program("${CMAKE_COMMAND} -E compare_files ${PROJECT_DIRECTORY}/doc/tmp.curtoc ${PROJECT_DIRECTORY}/doc/tmp.realtoc" OUTPUT_VARIABLE compare_files)
	if(compare_files)
		message("Wrong toc for ${file_name}. Should be:")
		file(READ "${PROJECT_DIRECTORY}/doc/tmp.realtoc" file_content)
		message("${file_content}")
		exec_program("diff -u ${PROJECT_DIRECTORY}/doc/tmp.curtoc ${PROJECT_DIRECTORY}/doc/tmp.realtoc")
        endif(compare_files)
endforeach(file_name ${LISTE_GTUT})

file(REMOVE ${PROJECT_DIRECTORY}/doc/tmp.curtoc)
file(REMOVE ${PROJECT_DIRECTORY}/doc/tmp.realtoc)

message("Check main TOC")

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
			file(APPEND ${PROJECT_DIRECTORY}/doc/tmp.realtoc "${line2}\n")
		endif(line2)
	endforeach(line ${file_content})
	
	foreach(line ${valeur_line})
		string(REGEX MATCH ".*toc.*" line1 "${line}")
		string(REGEX MATCH ".*<hr>.*" line2 "${line}")
		string(REGEX MATCH "^[ ]*$" line3 "${line}")
		string(REGEX MATCH "Table of Contents" line4 "${line}")
		if(NOT line1 AND NOT line2 AND NOT line3 AND NOT line4)
			file(APPEND ${PROJECT_DIRECTORY}/doc/tmp.realtoc "   ${line}\n")
		endif(NOT line1 AND NOT line2 AND NOT line3 AND NOT line4)
	endforeach(line ${valeur_line})
endforeach(file_name ${LISTE_GTUT})	

file(READ "${PROJECT_DIRECTORY}/doc/gtut-tour.doc" file_content)
string(REPLACE "\n" ";" file_content "${file_content}")
foreach(line ${file_content})
	string(REGEX MATCH "^[ ]+.*\\ref" line1 "${line}")
	if(line1)
		file(APPEND ${PROJECT_DIRECTORY}/doc/tmp.curtoc "${line}\n")
	endif(line1)
endforeach(line ${file_content})
	
exec_program("${CMAKE_COMMAND} -E compare_files ${PROJECT_DIRECTORY}/doc/tmp.curtoc ${PROJECT_DIRECTORY}/doc/tmp.realtoc" OUTPUT_VARIABLE compare_files)
if(compare_files)
	message("Wrong toc for gtut-tour.doc Right one is in tmp.realtoc")
	exec_program("diff -u ${PROJECT_DIRECTORY}/doc/tmp.curtoc ${PROJECT_DIRECTORY}/doc/tmp.realtoc")
else(compare_files)
	file(REMOVE ${PROJECT_DIRECTORY}/doc/tmp.realtoc)
endif(compare_files)	
  
file(REMOVE ${PROJECT_DIRECTORY}/doc/tmp.curtoc)

