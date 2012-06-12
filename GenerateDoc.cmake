#### Generate the html documentation
find_path(DOXYGEN_PATH	NAMES doxygen	PATHS NO_DEFAULT_PATHS)
find_path(JAVADOC_PATH  NAMES javadoc   PATHS NO_DEFAULT_PATHS)

if(DOXYGEN_PATH AND JAVADOC_PATH)
	
	configure_file(${CMAKE_HOME_DIRECTORY}/doc/Doxyfile.in ${CMAKE_HOME_DIRECTORY}/doc/Doxyfile @ONLY)
	configure_file(${CMAKE_HOME_DIRECTORY}/doc/footer.html.in ${CMAKE_HOME_DIRECTORY}/doc/footer.html @ONLY)		
	
	ADD_CUSTOM_TARGET(documentation
		COMMENT "Generating the SimGrid documentation..."
		COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_HOME_DIRECTORY}/doc/html
	    COMMAND ${CMAKE_COMMAND} -E make_directory   ${CMAKE_HOME_DIRECTORY}/doc/html
		WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}/doc
	)
	
	ADD_CUSTOM_COMMAND(TARGET documentation
	    COMMAND ${CMAKE_COMMAND} -E echo "XX Doxygen pass"
		COMMAND ${DOXYGEN_PATH}/doxygen Doxyfile
		
        COMMAND ${CMAKE_COMMAND} -E echo "XX Javadoc pass"
        COMMAND ${JAVADOC_PATH}/javadoc -d ${CMAKE_HOME_DIRECTORY}/doc/html/javadoc/ ${CMAKE_HOME_DIRECTORY}/org/simgrid/msg/*.java
		
		WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}/doc/
	)
	
	ADD_CUSTOM_COMMAND(TARGET documentation
        COMMAND ${CMAKE_COMMAND} -E echo "XX Post-processing Doxygen result"
        COMMAND ${CMAKE_HOME_DIRECTORY}/doxygen_postprocesser.pl
        
        WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}/doc
    )
	
else(DOXYGEN_PATH AND JAVADOC_PATH)
	ADD_CUSTOM_TARGET(documentation
			COMMENT "Generating the SimGrid documentation..."
			)

	ADD_CUSTOM_COMMAND(TARGET documentation
			COMMAND ${CMAKE_COMMAND} -E echo "DOXYGEN_PATH 		= ${DOXYGEN_PATH}"
			COMMAND ${CMAKE_COMMAND} -E echo "JAVADOC_PATH      = ${JAVADOC_PATH}"
			COMMAND ${CMAKE_COMMAND} -E echo "FAIL TO MAKE SIMGRID DOCUMENTATION see previous messages for details ..."
			COMMAND false
			)	
endif(DOXYGEN_PATH AND JAVADOC_PATH)

ADD_CUSTOM_TARGET(pdf
    COMMAND ${CMAKE_COMMAND} -E echo "XX First pass simgridJava_documentation.pdf"
    COMMAND make clean
    COMMAND make pdf || true
    COMMAND ${CMAKE_COMMAND} -E echo "XX Second pass simgridJava_documentation.pdf"
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_HOME_DIRECTORY}/doc/latex/refman.pdf
    COMMAND make pdf || true
    COMMAND ${CMAKE_COMMAND} -E echo "XX Write SimgridJava_documentation.pdf"
    COMMAND ${CMAKE_COMMAND} -E rename ${CMAKE_HOME_DIRECTORY}/doc/latex/refman.pdf ${CMAKE_HOME_DIRECTORY}/doc/latex/SG_Java_doc.pdf
  
    WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}/doc/latex/
)
add_dependencies(pdf simgrid_documentation)

add_custom_target(sync-gforge-doc
COMMAND chmod g+rw -R doc/
COMMAND chmod a+rX -R doc/
COMMAND ssh scm.gforge.inria.fr mkdir /home/groups/simgrid/htdocs/simgrid-java/${SIMGRID_JAVA_VERSION_MAJOR}.${SIMGRID_JAVA_VERSION_MINOR} || true 
COMMAND rsync --verbose --cvs-exclude --compress --delete --delete-excluded --rsh=ssh --ignore-times --recursive --links --perms --times --omit-dir-times 
doc/html/ scm.gforge.inria.fr:/home/groups/simgrid/htdocs/simgrid-java/${SIMGRID_JAVA_VERSION_MAJOR}.${SIMGRID_JAVA_VERSION_MINOR}/doc/ || true
WORKING_DIRECTORY "${CMAKE_HOME_DIRECTORY}"
)
add_dependencies(sync-gforge-doc documentation)