#########################################
### Fill in the "make install" target ###
#########################################
	  
# doc
if(enable_doc)
	if(NOT EXISTS ${PROJECT_DIRECTORY}/doc/html/)
		file(MAKE_DIRECTORY ${PROJECT_DIRECTORY}/doc/html/)
	endif(NOT EXISTS ${PROJECT_DIRECTORY}/doc/html/)
		install(DIRECTORY "${PROJECT_DIRECTORY}/doc/html/"
		  DESTINATION "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/doc/simgrid/html/"
		  PATTERN ".svn" EXCLUDE 
		  PATTERN ".git" EXCLUDE 
		  PATTERN "*.o" EXCLUDE
		  PATTERN "*~" EXCLUDE
		)
endif(enable_doc)

# binaries
install(PROGRAMS ${CMAKE_BINARY_DIR}/bin/smpicc
                 ${CMAKE_BINARY_DIR}/bin/smpif2c
                 ${CMAKE_BINARY_DIR}/bin/smpiff
                 ${CMAKE_BINARY_DIR}/bin/smpirun
		DESTINATION $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/)
if(WIN32)
	install(PROGRAMS ${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/tesh.pl
	DESTINATION $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/
	RENAME tesh)
	add_custom_target(tesh
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/tesh.pl ${CMAKE_BINARY_DIR}/bin/tesh
	)
else(WIN32)
	install(PROGRAMS ${CMAKE_BINARY_DIR}/bin/tesh
	DESTINATION $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/)
endif(WIN32)  
	
install(PROGRAMS ${CMAKE_HOME_DIRECTORY}/tools/MSG_visualization/colorize.pl
        DESTINATION $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/
		RENAME simgrid-colorizer)
		
add_custom_target(simgrid-colorizer ALL
COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_HOME_DIRECTORY}/tools/MSG_visualization/colorize.pl ${CMAKE_BINARY_DIR}/bin/colorize
)
				
install(PROGRAMS ${CMAKE_HOME_DIRECTORY}/tools/simgrid_update_xml.pl
		DESTINATION $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/
		RENAME simgrid_update_xml)
		
add_custom_target(simgrid_update_xml ALL
COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_HOME_DIRECTORY}/tools/simgrid_update_xml.pl ${CMAKE_BINARY_DIR}/bin/simgrid_update_xml
)
		
install(PROGRAMS ${CMAKE_BINARY_DIR}/bin/gras_stub_generator
		DESTINATION $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/)

# libraries
install(TARGETS simgrid gras 
        DESTINATION $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/)
	
if(enable_smpi)	
  install(TARGETS smpi
          DESTINATION $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/)
endif(enable_smpi)

if(enable_lib_static AND NOT WIN32)
	install(TARGETS simgrid_static 
	        DESTINATION $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/)
	if(enable_smpi)	
  		install(TARGETS smpi_static
          		DESTINATION $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/)
	endif(enable_smpi)
endif(enable_lib_static AND NOT WIN32)

# include files
foreach(file ${install_HEADERS})
  get_filename_component(location ${file} PATH)
  string(REPLACE "${CMAKE_CURRENT_BINARY_DIR}/" "" location "${location}")
  install(FILES ${file}
          DESTINATION $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/${location})
endforeach(file ${install_HEADERS})

# example files
foreach(file ${examples_to_install_in_doc})
  string(REPLACE "${PROJECT_DIRECTORY}/examples/" "" file ${file})
  get_filename_component(location ${file} PATH)
  install(FILES "examples/${file}"
          DESTINATION $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/doc/simgrid/examples/${location})
endforeach(file ${examples_to_install_in_doc})

# bindings cruft
if(HAVE_JAVA)
  install(FILES ${CMAKE_BINARY_DIR}/simgrid.jar
          DESTINATION $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/)
endif(HAVE_JAVA)

if(HAVE_LUA)
	file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/lib/lua/5.1")
	add_custom_target(simgrid_lua ALL
  		DEPENDS simgrid 
  				${CMAKE_BINARY_DIR}/lib/lua/5.1/simgrid.${LIB_EXE}
		)
	add_custom_command(
		OUTPUT ${CMAKE_BINARY_DIR}/lib/lua/5.1/simgrid.${LIB_EXE}
		COMMAND ${CMAKE_COMMAND} -E create_symlink ../../libsimgrid.${LIB_EXE} ${CMAKE_BINARY_DIR}/lib/lua/5.1/simgrid.${LIB_EXE}
	)
	install(FILES ${CMAKE_BINARY_DIR}/lib/lua/5.1/simgrid.${LIB_EXE}
		DESTINATION $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/lua/5.1
		)
endif(HAVE_LUA)

if(HAVE_RUBY)
	string(REGEX REPLACE "^.*ruby/" "" install_link_ruby "${RUBY_ARCH_DIR}")
	file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/lib/ruby/${install_link_ruby}")
	add_custom_target(ruby_simgrid ALL
		DEPENDS simgrid
				${CMAKE_BINARY_DIR}/lib/ruby/${install_link_ruby}/libsimgrid.${LIB_EXE}
	)
	add_custom_command(
		OUTPUT ${CMAKE_BINARY_DIR}/lib/ruby/${install_link_ruby}/libsimgrid.${LIB_EXE}
		COMMAND ${CMAKE_COMMAND} -E create_symlink ../../../libsimgrid.${LIB_EXE} ${CMAKE_BINARY_DIR}/lib/ruby/${install_link_ruby}/libsimgrid.${LIB_EXE}
	)
	install(FILES ${CMAKE_BINARY_DIR}/lib/ruby/${install_link_ruby}/libsimgrid.${LIB_EXE}
		DESTINATION $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/ruby/${install_link_ruby}/
	)
	install(FILES ${PROJECT_DIRECTORY}/src/bindings/ruby/simgrid.rb
		DESTINATION $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/ruby/${install_link_ruby}/)

endif(HAVE_RUBY)

###########################################
### Fill in the "make uninstall" target ###
###########################################

add_custom_target(uninstall
COMMAND ${CMAKE_COMMAND} -E	remove_directory ${CMAKE_INSTALL_PREFIX}/doc/simgrid
COMMAND ${CMAKE_COMMAND} -E	echo "uninstall doc ok"
COMMAND ${CMAKE_COMMAND} -E	remove -f ${CMAKE_INSTALL_PREFIX}/lib/libgras*
COMMAND ${CMAKE_COMMAND} -E	remove -f ${CMAKE_INSTALL_PREFIX}/lib/libsimgrid*
COMMAND ${CMAKE_COMMAND} -E	remove -f ${CMAKE_INSTALL_PREFIX}/lib/libsmpi*
COMMAND ${CMAKE_COMMAND} -E	echo "uninstall lib ok"
COMMAND ${CMAKE_COMMAND} -E	remove -f ${CMAKE_INSTALL_PREFIX}/bin/smpicc
COMMAND ${CMAKE_COMMAND} -E	remove -f ${CMAKE_INSTALL_PREFIX}/bin/smpif2c
COMMAND ${CMAKE_COMMAND} -E	remove -f ${CMAKE_INSTALL_PREFIX}/bin/smpiff
COMMAND ${CMAKE_COMMAND} -E	remove -f ${CMAKE_INSTALL_PREFIX}/bin/smpirun
COMMAND ${CMAKE_COMMAND} -E	remove -f ${CMAKE_INSTALL_PREFIX}/bin/tesh
COMMAND ${CMAKE_COMMAND} -E	remove -f ${CMAKE_INSTALL_PREFIX}/bin/simgrid-colorizer
COMMAND ${CMAKE_COMMAND} -E	remove -f ${CMAKE_INSTALL_PREFIX}/bin/simgrid_update_xml
COMMAND ${CMAKE_COMMAND} -E	remove -f ${CMAKE_INSTALL_PREFIX}/bin/gras_stub_generator
COMMAND ${CMAKE_COMMAND} -E	echo "uninstall bin ok"
COMMAND ${CMAKE_COMMAND} -E	remove_directory ${CMAKE_INSTALL_PREFIX}/include/amok
COMMAND ${CMAKE_COMMAND} -E	remove_directory ${CMAKE_INSTALL_PREFIX}/include/gras
COMMAND ${CMAKE_COMMAND} -E	remove_directory ${CMAKE_INSTALL_PREFIX}/include/instr
COMMAND ${CMAKE_COMMAND} -E	remove_directory ${CMAKE_INSTALL_PREFIX}/include/msg 
COMMAND ${CMAKE_COMMAND} -E	remove_directory ${CMAKE_INSTALL_PREFIX}/include/simdag
COMMAND ${CMAKE_COMMAND} -E	remove_directory ${CMAKE_INSTALL_PREFIX}/include/smpi
COMMAND ${CMAKE_COMMAND} -E	remove_directory ${CMAKE_INSTALL_PREFIX}/include/surf
COMMAND ${CMAKE_COMMAND} -E	remove_directory ${CMAKE_INSTALL_PREFIX}/include/xbt
COMMAND ${CMAKE_COMMAND} -E	remove_directory ${CMAKE_INSTALL_PREFIX}/include/mc
COMMAND ${CMAKE_COMMAND} -E	remove -f ${CMAKE_INSTALL_PREFIX}/include/simgrid_config.h
COMMAND ${CMAKE_COMMAND} -E	remove -f ${CMAKE_INSTALL_PREFIX}/include/gras.h 
COMMAND ${CMAKE_COMMAND} -E	remove -f ${CMAKE_INSTALL_PREFIX}/include/xbt.h
COMMAND ${CMAKE_COMMAND} -E	echo "uninstal include ok"
WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}"
)

if(HAVE_JAVA)
	add_custom_command(TARGET uninstall
	COMMAND ${CMAKE_COMMAND} -E	remove -f ${CMAKE_INSTALL_PREFIX}/share/simgrid.jar
	COMMAND ${CMAKE_COMMAND} -E echo "uninstall binding java ok"
	WORKING_DIRECTORY "${PROJECT_DIRECTORY}/"
	)	
endif(HAVE_JAVA)

if(HAVE_LUA)
	add_custom_command(TARGET uninstall
	COMMAND ${CMAKE_COMMAND} -E echo "uninstall binding lua ok"
	COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_INSTALL_PREFIX}/lib/lua/5.1/simgrid.${LIB_EXE}
	WORKING_DIRECTORY "${PROJECT_DIRECTORY}/"
	)
endif(HAVE_LUA)

if(HAVE_RUBY)
	string(REGEX REPLACE "^.*ruby/" "" install_link_ruby "${RUBY_ARCH_DIR}")
	add_custom_command(TARGET uninstall
	COMMAND ${CMAKE_COMMAND} -E echo "uninstall binding ruby ok"
	COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_INSTALL_PREFIX}/lib/ruby/${install_link_ruby}/libsimgrid.${LIB_EXE}
	COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_INSTALL_PREFIX}/lib/ruby/${install_link_ruby}/simgrid.rb
	WORKING_DIRECTORY "${PROJECT_DIRECTORY}/"
	)
endif(HAVE_RUBY)

################################################################
## Build a sain "make dist" target to build a source package ###
##   containing only the files that I explicitely state      ###
##   (instead of any cruft laying on my disk as CPack does)  ###
################################################################

##########################################
### Fill in the "make dist-dir" target ###
##########################################

add_custom_target(dist-dir
  COMMAND test -e simgrid-${release_version}/ && chmod -R a+w simgrid-${release_version}/ || true
  COMMAND ${CMAKE_COMMAND} -E remove_directory simgrid-${release_version}/
  COMMAND ${CMAKE_COMMAND} -E make_directory simgrid-${release_version}
  COMMAND ${CMAKE_COMMAND} -E make_directory simgrid-${release_version}/doc/html/
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_DIRECTORY}/doc/html/ simgrid-${release_version}/doc/html/
)

set(dirs_in_tarball "")
foreach(file ${source_to_pack})
  #message(${file})
  # This damn prefix is still set somewhere (seems to be in subdirs)
  string(REPLACE "${PROJECT_DIRECTORY}/" "" file "${file}")
  
  # Create the directory on need
  get_filename_component(file_location ${file} PATH)
  string(REGEX MATCH ";${file_location};" OPERATION "${dirs_in_tarball}")
  if(NOT OPERATION)
       set(dirs_in_tarball "${dirs_in_tarball};${file_location};")
       add_custom_command(
         TARGET dist-dir
         COMMAND ${CMAKE_COMMAND} -E make_directory simgrid-${release_version}/${file_location}/
       )       
   endif(NOT OPERATION)
   
   # Actually copy the file
   add_custom_command(
     TARGET dist-dir
     COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_DIRECTORY}/${file} simgrid-${release_version}/${file_location}/
   )
endforeach(file ${source_to_pack})

######################################
### Fill in the "make dist" target ###
######################################

add_custom_target(dist
  DEPENDS ${CMAKE_BINARY_DIR}/simgrid-${release_version}.tar.gz
)
add_custom_command(
	OUTPUT ${CMAKE_BINARY_DIR}/simgrid-${release_version}.tar.gz	
	COMMAND ${CMAKE_COMMAND} -E tar cf simgrid-${release_version}.tar simgrid-${release_version}/
  	COMMAND gzip -9v simgrid-${release_version}.tar
  	COMMAND ${CMAKE_COMMAND} -E remove_directory simgrid-${release_version}/
)
add_dependencies(dist dist-dir)

###########################################
### Fill in the "make distcheck" target ###
###########################################

# Allow to test the "make dist"
add_custom_target(distcheck
  COMMAND ${CMAKE_COMMAND} -E echo "XXX remove old copy"
  COMMAND ${CMAKE_COMMAND} -E remove_directory simgrid-${release_version}.cpy 
  COMMAND ${CMAKE_COMMAND} -E echo "XXX copy again the source tree"
  COMMAND ${CMAKE_COMMAND} -E copy_directory simgrid-${release_version}/ simgrid-${release_version}.cpy 
  COMMAND ${CMAKE_COMMAND} -E echo "XXX create build and install subtrees"
  COMMAND ${CMAKE_COMMAND} -E make_directory simgrid-${release_version}/_build
  COMMAND ${CMAKE_COMMAND} -E make_directory simgrid-${release_version}/_inst
 
  # This stupid cmake creates a directory in source, killing the purpose of the chmod
  # (tricking around)
  COMMAND ${CMAKE_COMMAND} -E echo "XXX change the modes of directories"
  COMMAND ${CMAKE_COMMAND} -E make_directory simgrid-${release_version}/CMakeFiles 
  COMMAND chmod -R a-w simgrid-${release_version}/ # FIXME: we should pass without commenting that line
  COMMAND chmod -R a+w simgrid-${release_version}/_build
  COMMAND chmod -R a+w simgrid-${release_version}/_inst
  COMMAND chmod -R a+w simgrid-${release_version}/CMakeFiles
  
  COMMAND ${CMAKE_COMMAND} -E echo "XXX Configure"
  COMMAND ${CMAKE_COMMAND} -E chdir simgrid-${release_version}/_build ${CMAKE_COMMAND} build ..  -DCMAKE_INSTALL_PREFIX=../_inst -Wno-dev -Denable_doc=OFF
#  COMMAND ${CMAKE_COMMAND} -E chdir simgrid-${release_version}/_build make dist-dir
  COMMAND ${CMAKE_COMMAND} -E echo "XXX Build"
  COMMAND ${CMAKE_COMMAND} -E chdir simgrid-${release_version}/_build make VERBOSE=1
  
  # This fails, unfortunately, because GRAS is broken for now
  COMMAND ${CMAKE_COMMAND} -E chdir simgrid-${release_version}/_build ctest -j5 --output-on-failure

  COMMAND ${CMAKE_COMMAND} -E echo "XXX Check that cleaning works"
  COMMAND ${CMAKE_COMMAND} -E chdir simgrid-${release_version}/_build make clean
  COMMAND ${CMAKE_COMMAND} -E echo "XXX Display what is remaining after make clean"
  COMMAND ${CMAKE_COMMAND} -E chdir simgrid-${release_version}/_build ls -lR
  COMMAND ${CMAKE_COMMAND} -E echo "XXX Remove _build and _inst directories"
  COMMAND chmod a+w simgrid-${release_version}/
  COMMAND ${CMAKE_COMMAND} -E remove_directory simgrid-${release_version}/_build
  COMMAND ${CMAKE_COMMAND} -E remove_directory simgrid-${release_version}/_inst
  COMMAND ${CMAKE_COMMAND} -E echo "XXX The output of the diff follows"
  COMMAND diff -ruN simgrid-${release_version}.cpy simgrid-${release_version}
  COMMAND ${CMAKE_COMMAND} -E echo "XXX end of the diff, random cleanups now"
  COMMAND ${CMAKE_COMMAND} -E remove_directory simgrid-${release_version}.cpy 
  COMMAND ${CMAKE_COMMAND} -E remove_directory simgrid-${release_version}
)
add_dependencies(distcheck dist-dir)

#######################################
### Fill in the "make check" target ###
#######################################

if(enable_memcheck)
	add_custom_target(check
	COMMAND ctest -D ExperimentalMemCheck
	)
else(enable_memcheck)
	add_custom_target(check
	COMMAND make test
	)
endif(enable_memcheck)

#######################################
### Fill in the "make xxx-clean" target ###
#######################################

add_custom_target(maintainer-clean
COMMAND ${CMAKE_COMMAND} -E remove -f src/config_unit.c
COMMAND ${CMAKE_COMMAND} -E remove -f src/cunit_unit.c
COMMAND ${CMAKE_COMMAND} -E remove -f src/dict_unit.c
COMMAND ${CMAKE_COMMAND} -E remove -f src/dynar_unit.c
COMMAND ${CMAKE_COMMAND} -E remove -f src/ex_unit.c
COMMAND ${CMAKE_COMMAND} -E remove -f src/set_unit.c
COMMAND ${CMAKE_COMMAND} -E remove -f src/simgrid_units_main.c
COMMAND ${CMAKE_COMMAND} -E remove -f src/swag_unit.c
COMMAND ${CMAKE_COMMAND} -E remove -f src/xbt_sha_unit.c
COMMAND ${CMAKE_COMMAND} -E remove -f src/xbt_str_unit.c
COMMAND ${CMAKE_COMMAND} -E remove -f src/xbt_strbuff_unit.c
COMMAND ${CMAKE_COMMAND} -E remove -f src/xbt_synchro_unit.c
WORKING_DIRECTORY "${PROJECT_DIRECTORY}"
)

add_custom_target(supernovae-clean
COMMAND ${CMAKE_COMMAND} -E remove -f src/supernovae_gras.c
COMMAND ${CMAKE_COMMAND} -E remove -f src/supernovae_sg.c
COMMAND ${CMAKE_COMMAND} -E remove -f src/supernovae_smpi.c
WORKING_DIRECTORY "${PROJECT_DIRECTORY}"
)

#############################################
### Fill in the "make sync-gforge" target ###
#############################################

add_custom_target(sync-gforge
COMMAND chmod g+rw -R doc/
COMMAND chmod a+rX -R doc/
COMMAND rsync --verbose --cvs-exclude --compress --delete --delete-excluded --rsh=ssh --ignore-times --recursive --links --perms --times --omit-dir-times doc/html/ scm.gforge.inria.fr:/home/groups/simgrid/htdocs/doc/ || true
COMMAND scp doc/index.php doc/webcruft/robots.txt scm.gforge.inria.fr:/home/groups/simgrid/htdocs/
COMMAND scp doc/html/simgrid_modules2.png doc/html/simgrid_modules.png doc/webcruft/simgrid_logo.png doc/webcruft/simgrid_logo_small.png scm.gforge.inria.fr:/home/groups/simgrid/htdocs/
WORKING_DIRECTORY "${PROJECT_DIRECTORY}"
)

include(CPack)
