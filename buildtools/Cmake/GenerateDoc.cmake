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
  include(${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/GenerateDevGuide.cmake)

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
  else()
    add_dependencies(simgrid_documentation ref_guide)
    add_dependencies(simgrid_documentation user_guide)
    add_dependencies(simgrid_documentation dev_guide)
  endif()

endif()

#############################################
### Fill in the "make sync-gforge" target ###
#############################################

### TODO: LBO: CHECK IF CORRECT
add_custom_target(sync-gforge-doc
  COMMAND chmod g+rw -R doc/
  COMMAND chmod a+rX -R doc/
  COMMAND ssh scm.gforge.inria.fr mkdir /home/groups/simgrid/htdocs/simgrid/${release_version}/ || true
  COMMAND ssh scm.gforge.inria.fr mkdir /home/groups/simgrid/htdocs/simgrid/${release_version}/user_guide/ || true
  COMMAND ssh scm.gforge.inria.fr mkdir /home/groups/simgrid/htdocs/simgrid/${release_version}/ref_guide/ || true
  COMMAND ssh scm.gforge.inria.fr mkdir /home/groups/simgrid/htdocs/simgrid/${release_version}/dev_guide/ || true
  COMMAND ssh scm.gforge.inria.fr mkdir /home/groups/simgrid/htdocs/simgrid/${release_version}/user_guide/html/ || true
  COMMAND ssh scm.gforge.inria.fr mkdir /home/groups/simgrid/htdocs/simgrid/${release_version}/ref_guide/html/ || true
  COMMAND ssh scm.gforge.inria.fr mkdir /home/groups/simgrid/htdocs/simgrid/${release_version}/dev_guide/html/ || true

  COMMAND rsync --verbose --cvs-exclude --compress --delete --delete-excluded --rsh=ssh --ignore-times --recursive --links --perms --times --omit-dir-times
  doc/html/ scm.gforge.inria.fr:/home/groups/simgrid/htdocs/simgrid/${release_version}/doc/ || true

  COMMAND rsync --verbose --cvs-exclude --compress --delete --delete-excluded --rsh=ssh --ignore-times --recursive --links --perms --times --omit-dir-times
  doc/user_guide/html/ scm.gforge.inria.fr:/home/groups/simgrid/htdocs/simgrid/${release_version}/user_guide/html/ || true

  COMMAND rsync --verbose --cvs-exclude --compress --delete --delete-excluded --rsh=ssh --ignore-times --recursive --links --perms --times --omit-dir-times
  doc/ref_guide/html/ scm.gforge.inria.fr:/home/groups/simgrid/htdocs/simgrid/${release_version}/ref_guide/html || true

  COMMAND rsync --verbose --cvs-exclude --compress --delete --delete-excluded --rsh=ssh --ignore-times --recursive --links --perms --times --omit-dir-times
  doc/dev_guide/html/ scm.gforge.inria.fr:/home/groups/simgrid/htdocs/simgrid/${release_version}/dev_guide/html || true

  COMMAND scp doc/user_guide/html/simgrid_modules2.png doc/user_guide/html/simgrid_modules.png doc/webcruft/simgrid_logo_2011.png
  doc/webcruft/simgrid_logo_2011_small.png scm.gforge.inria.fr:/home/groups/simgrid/htdocs/simgrid/${release_version}/
  WORKING_DIRECTORY "${CMAKE_HOME_DIRECTORY}"
  )
add_dependencies(sync-gforge-doc simgrid_documentation)

add_custom_target(sync-gforge-dtd
  COMMAND scp src/surf/simgrid.dtd scm.gforge.inria.fr:/home/groups/simgrid/htdocs/simgrid/${release_version}/simgrid.dtd
  COMMAND scp src/surf/simgrid.dtd scm.gforge.inria.fr:/home/groups/simgrid/htdocs/simgrid.dtd
  WORKING_DIRECTORY "${CMAKE_HOME_DIRECTORY}"
  )

##############################################################################"
# GTUT FILES

#message(STATUS "Check individual TOCs")

#foreach(file_name ${LISTE_GTUT})
#   file(REMOVE ${CMAKE_HOME_DIRECTORY}/doc/doxygen/tmp.curtoc)
#   file(REMOVE ${CMAKE_HOME_DIRECTORY}/doc/doxygen/tmp.realtoc)
#
#   file(READ "${file_name}" file_content)
#   string(REGEX MATCH "Table of Contents.*<hr>" valeur_line "${file_content}")
#   string(REPLACE "\n" ";" valeur_line "${valeur_line}")
#   string(REPLACE "\n" ";" file_content "${file_content}")
#
#   file(APPEND ${CMAKE_HOME_DIRECTORY}/doc/doxygen/tmp.realtoc "\n") # make sure it exists
#   foreach(line ${file_content})
#       string(REGEX MATCH "[\\]s?u?b?s?u?b?section.*" line2 "${line}")
#       string(REGEX MATCH ".*_toc.*" line3 "${line}")
#       if(line2 AND NOT line3)
#           string(REPLACE "\\section " "" line2 ${line2})
#           string(REPLACE "\\subsection " "subsection" line2 ${line2})
#           string(REPLACE "\\subsubsection " "subsubsection" line2 ${line2})
#           string(REGEX REPLACE " .*" "" line2 ${line2})
#           set(line2                               " - \\ref ${line2}")
#           string(REPLACE " - \\ref subsection"    "   - \\ref " line2 ${line2})
#           string(REPLACE " - \\ref subsubsection" "     - \\ref " line2 ${line2})
#           file(APPEND ${CMAKE_HOME_DIRECTORY}/doc/doxygen/tmp.realtoc "${line2}\n")
#       endif()
#   endforeach(line ${file_content})
#
#   file(APPEND ${CMAKE_HOME_DIRECTORY}/doc/doxygen/tmp.curtoc "\n") # make sure it exists
#   foreach(line ${valeur_line})
#       string(REGEX MATCH ".*ref.*" line_ok ${line})
#       if(line_ok)
#           file(APPEND ${CMAKE_HOME_DIRECTORY}/doc/doxygen/tmp.curtoc "${line_ok}\n")
#       endif()
#   endforeach(line ${valeur_line})
#
#   exec_program("${CMAKE_COMMAND} -E compare_files ${CMAKE_HOME_DIRECTORY}/doc/doxygen/tmp.curtoc ${CMAKE_HOME_DIRECTORY}/doc/doxygen/tmp.realtoc" OUTPUT_VARIABLE compare_files)
#   if(compare_files)
#       message(STATUS "Wrong toc for ${file_name}. Should be:")
#       file(READ "${CMAKE_HOME_DIRECTORY}/doc/doxygen/tmp.realtoc" file_content)
#       message("${file_content}")
#       exec_program("diff -u ${CMAKE_HOME_DIRECTORY}/doc/doxygen/tmp.curtoc ${CMAKE_HOME_DIRECTORY}/doc/doxygen/tmp.realtoc")
#        endif()
#endforeach(file_name ${LISTE_GTUT})
#
#file(REMOVE ${CMAKE_HOME_DIRECTORY}/doc/doxygen/tmp.curtoc)
#file(REMOVE ${CMAKE_HOME_DIRECTORY}/doc/doxygen/tmp.realtoc)
#
#message(STATUS "Check main TOC")
#
#foreach(file_name ${LISTE_GTUT})
#   file(READ "${file_name}" file_content)
#   string(REGEX MATCH "Table of Contents.*<hr>" valeur_line "${file_content}")
#   string(REPLACE "\n" ";" valeur_line "${valeur_line}")
#   string(REPLACE "\n" ";" file_content "${file_content}")
#
#   foreach(line ${file_content})
#       string(REGEX MATCH ".*@page.*" line2 "${line}")
#       if(line2)
#           string(REPLACE "@page " "" line2 "${line2}")
#           string(REGEX REPLACE " .*" "" line2 "${line2}")
#           set(line2 " - \\ref ${line2}")
#           file(APPEND ${CMAKE_HOME_DIRECTORY}/doc/doxygen/tmp.realtoc "${line2}\n")
#       endif()
#   endforeach(line ${file_content})
#
#   foreach(line ${valeur_line})
#       string(REGEX MATCH ".*toc.*" line1 "${line}")
#       string(REGEX MATCH ".*<hr>.*" line2 "${line}")
#       string(REGEX MATCH "^[ ]*$" line3 "${line}")
#       string(REGEX MATCH "Table of Contents" line4 "${line}")
#       if(NOT line1 AND NOT line2 AND NOT line3 AND NOT line4)
#           file(APPEND ${CMAKE_HOME_DIRECTORY}/doc/doxygen/tmp.realtoc "   ${line}\n")
#       endif()
#   endforeach(line ${valeur_line})
#endforeach(file_name ${LISTE_GTUT})
#
#file(READ "${CMAKE_HOME_DIRECTORY}/doc/doxygen/gtut-tour.doc" file_content)
#string(REPLACE "\n" ";" file_content "${file_content}")
#foreach(line ${file_content})
#   string(REGEX MATCH "^[ ]+.*\\ref" line1 "${line}")
#   if(line1)
#       file(APPEND ${CMAKE_HOME_DIRECTORY}/doc/doxygen/tmp.curtoc "${line}\n")
#   endif()
#endforeach(line ${file_content})
#
#exec_program("${CMAKE_COMMAND} -E compare_files ${CMAKE_HOME_DIRECTORY}/doc/doxygen/tmp.curtoc ${CMAKE_HOME_DIRECTORY}/doc/doxygen/tmp.realtoc" OUTPUT_VARIABLE compare_files)
#if(compare_files)
#   message(STATUS "Wrong toc for gtut-tour.doc Right one is in tmp.realtoc")
#   exec_program("diff -u ${CMAKE_HOME_DIRECTORY}/doc/doxygen/tmp.curtoc ${CMAKE_HOME_DIRECTORY}/doc/doxygen/tmp.realtoc")
#else()
#   file(REMOVE ${CMAKE_HOME_DIRECTORY}/doc/doxygen/tmp.realtoc)
#endif()
#
#file(REMOVE ${CMAKE_HOME_DIRECTORY}/doc/doxygen/tmp.curtoc)