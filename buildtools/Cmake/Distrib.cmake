set(CMAKE_INSTALL_PREFIX ${prefix} CACHE TYPE INTERNAL FORCE)

#########################################
### Fill in the "make install" target ###
#########################################
	  
# doc
if(EXISTS ${PROJECT_DIRECTORY}/doc/html/)
	install(DIRECTORY "${PROJECT_DIRECTORY}/doc/html/"
	  DESTINATION "$ENV{DESTDIR}${prefix}/doc/simgrid/html/"
	  PATTERN ".svn" EXCLUDE 
	  PATTERN ".git" EXCLUDE 
	  PATTERN "*.o" EXCLUDE
	  PATTERN "*~" EXCLUDE
	)
endif(EXISTS ${PROJECT_DIRECTORY}/doc/html/)

# binaries
install(PROGRAMS ${CMAKE_BINARY_DIR}/bin/smpicc
                 ${CMAKE_BINARY_DIR}/bin/smpirun
                 ${CMAKE_BINARY_DIR}/bin/tesh
		DESTINATION $ENV{DESTDIR}${prefix}/bin/)
	
install(PROGRAMS tools/MSG_visualization/colorize.pl
        DESTINATION $ENV{DESTDIR}${prefix}/bin/
		RENAME simgrid-colorizer)

# libraries
install(TARGETS simgrid gras 
        DESTINATION $ENV{DESTDIR}${prefix}/lib/)
	
install(FILES ${CMAKE_BINARY_DIR}/lib/libsimgrid_static.a 
        RENAME libsimgrid.a
        DESTINATION $ENV{DESTDIR}${prefix}/lib/)
	
if(enable_smpi)	
  install(TARGETS smpi
          DESTINATION $ENV{DESTDIR}${prefix}/lib/)
endif(enable_smpi)	

# include files
foreach(file ${install_HEADERS})
  get_filename_component(location ${file} PATH)
  install(FILES ${file}
          DESTINATION $ENV{DESTDIR}${prefix}/${location})
endforeach(file ${install_HEADERS})

# example files
foreach(file ${examples_to_install_in_doc})
  string(REPLACE "${PROJECT_DIRECTORY}/examples/" "" file ${file})
  get_filename_component(location ${file} PATH)
  install(FILES "examples/${file}"
          DESTINATION $ENV{DESTDIR}${prefix}/doc/simgrid/examples/${location})
endforeach(file ${examples_to_install_in_doc})

# bindings cruft
if(HAVE_JAVA)
  install(FILES ${CMAKE_BINARY_DIR}/simgrid.jar
          DESTINATION $ENV{DESTDIR}${prefix}/share/)
endif(HAVE_JAVA)

if(HAVE_LUA)
  add_custom_target(absolute_liblink ALL
    COMMAND ${CMAKE_COMMAND} -E create_symlink $ENV{DESTDIR}${prefix}/lib/libsimgrid.so ${CMAKE_BINARY_DIR}/libsimgrid.so)
  install(FILES ${CMAKE_BINARY_DIR}/libsimgrid.so
          DESTINATION $ENV{DESTDIR}${prefix}/lib/lua/5.1
	  RENAME simgrid.so)
endif(HAVE_LUA)

if(HAVE_RUBY)
  string(REGEX REPLACE "^.*ruby/" "" install_link_ruby "${RUBY_ARCH_DIR}")
  install(FILES ${CMAKE_BINARY_DIR}/libsimgrid.so
                ${PROJECT_DIRECTORY}/src/bindings/ruby/simgrid.rb
          DESTINATION $ENV{DESTDIR}${prefix}/lib/ruby/${install_link_ruby}/)
endif(HAVE_RUBY)

###########################################
### Fill in the "make uninstall" target ###
###########################################

add_custom_target(uninstall
COMMAND ${CMAKE_COMMAND} -E	remove_directory ${prefix}/doc/simgrid
COMMAND ${CMAKE_COMMAND} -E	echo "uninstall doc ok"
COMMAND ${CMAKE_COMMAND} -E	remove -f ${uninstall_libs}
COMMAND ${CMAKE_COMMAND} -E	echo "uninstall lib ok"
COMMAND ${CMAKE_COMMAND} -E	remove -f ${uninstall_bins}
COMMAND ${CMAKE_COMMAND} -E	remove -f ${prefix}/bin/simgrid_colorizer.pl
COMMAND ${CMAKE_COMMAND} -E	echo "uninstall bin ok"
COMMAND ${CMAKE_COMMAND} -E	remove -f ${uninstall_HEADERS}
COMMAND ${CMAKE_COMMAND} -E	echo "uninstal include ok"
COMMAND ${CMAKE_COMMAND} -E	remove_directory ${prefix}/include/amok
COMMAND ${CMAKE_COMMAND} -E	remove_directory ${prefix}/include/gras
COMMAND ${CMAKE_COMMAND} -E	remove_directory ${prefix}/include/instr
COMMAND ${CMAKE_COMMAND} -E	remove_directory ${prefix}/include/msg 
COMMAND ${CMAKE_COMMAND} -E	remove_directory ${prefix}/include/simdag
COMMAND ${CMAKE_COMMAND} -E	remove_directory ${prefix}/include/smpi
COMMAND ${CMAKE_COMMAND} -E	remove_directory ${prefix}/include/surf
COMMAND ${CMAKE_COMMAND} -E	remove_directory ${prefix}/include/xbt
WORKING_DIRECTORY "${prefix}"
)

if(HAVE_JAVA)
	add_custom_command(TARGET uninstall
	COMMAND ${CMAKE_COMMAND} -E	remove -f ${prefix}/share/simgrid.jar
	COMMAND ${CMAKE_COMMAND} -E echo "uninstall binding java"
	WORKING_DIRECTORY "${PROJECT_DIRECTORY}/"
	)	
endif(HAVE_JAVA)

if(HAVE_LUA)
	add_custom_command(TARGET uninstall
	COMMAND ${CMAKE_COMMAND} -E echo "uninstall binding lua"
	COMMAND ${CMAKE_COMMAND} -E remove -f ${prefix}/lib/lua/5.1/simgrid.so	
	WORKING_DIRECTORY "${PROJECT_DIRECTORY}/"
	)
endif(HAVE_LUA)

if(HAVE_RUBY)
	string(REGEX REPLACE "^.*ruby/" "" install_link_ruby "${RUBY_ARCH_DIR}")
	add_custom_command(TARGET uninstall
	COMMAND ${CMAKE_COMMAND} -E echo "uninstall binding ruby"
	COMMAND ${CMAKE_COMMAND} -E remove -f ${prefix}/lib/ruby/${install_link_ruby}/libsimgrid.so
	COMMAND ${CMAKE_COMMAND} -E remove -f ${prefix}/lib/ruby/${install_link_ruby}/simgrid.rb
	WORKING_DIRECTORY "${PROJECT_DIRECTORY}/"
	)
endif(HAVE_RUBY)

######################################
### Fill in the "make html" target ###
######################################

add_custom_target(html
COMMAND ${CMAKE_COMMAND} -E echo "Make the html doc"
COMMAND ${CMAKE_COMMAND} -E echo "cmake -DBIBTEX2HTML=${BIBTEX2HTML} ./"
COMMAND ${CMAKE_COMMAND} -DBIBTEX2HTML=${BIBTEX2HTML} ./
COMMAND ${CMAKE_COMMAND} -E remove_directory ${PROJECT_DIRECTORY}/buildtools/Cmake/src/doc/CMakeFiles
COMMAND ${CMAKE_COMMAND} -E remove -f ${PROJECT_DIRECTORY}/buildtools/Cmake/src/doc/CMakeCache.txt
COMMAND ${CMAKE_COMMAND} -E remove -f ${PROJECT_DIRECTORY}/buildtools/Cmake/src/doc/cmake_install.cmake
COMMAND ${CMAKE_COMMAND} -E remove -f ${PROJECT_DIRECTORY}/buildtools/Cmake/src/doc/Makefile
WORKING_DIRECTORY "${PROJECT_DIRECTORY}/buildtools/Cmake/src/doc"
)

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
  COMMAND ${CMAKE_COMMAND} -E remove_directory simgrid-${release_version}
  COMMAND ${CMAKE_COMMAND} -E make_directory simgrid-${release_version}
)

set(dirs_in_tarball "")
foreach(file ${source_to_pack} ${txt_files})
  # This damn prefix is still set somewhere (seems to be in subdirs)
  string(REPLACE "${PROJECT_DIRECTORY}/" "" file ${file})
  
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
  COMMAND ${CMAKE_COMMAND} -E tar cf simgrid-${release_version}.tar simgrid-${release_version}/
  COMMAND gzip -9v simgrid-${release_version}.tar
  COMMAND ${CMAKE_COMMAND} -E remove_directory simgrid-${release_version}
)
add_dependencies(dist dist-dir)

###########################################
### Fill in the "make distcheck" target ###
###########################################

# Allow to test the "make dist"
add_custom_target(distcheck
  COMMAND ${CMAKE_COMMAND} -E make_directory simgrid-${release_version}/_build
  COMMAND ${CMAKE_COMMAND} -E make_directory simgrid-${release_version}/_inst
  
  # This stupid cmake creates a directory in source, killing the purpose of the chmod
  # (tricking around)
  COMMAND ${CMAKE_COMMAND} -E make_directory simgrid-${release_version}/CMakeFiles 
#  COMMAND chmod -R a-w simgrid-${release_version}/ # FIXME: we should pass without commenting that line
  COMMAND chmod -R a+w simgrid-${release_version}/_build
  COMMAND chmod -R a+w simgrid-${release_version}/_inst
  COMMAND chmod -R a+w simgrid-${release_version}/CMakeFiles
  
  COMMAND ${CMAKE_COMMAND} -E chdir simgrid-${release_version}/_build ${CMAKE_COMMAND} build ..  -Dprefix=../_inst
#  COMMAND ${CMAKE_COMMAND} -E chdir simgrid-${release_version}/_build make dist-dir
  COMMAND ${CMAKE_COMMAND} -E chdir simgrid-${release_version}/_build make 
  COMMAND ${CMAKE_COMMAND} -E chdir simgrid-${release_version}/_build ctest -j5 --output-on-failure
  COMMAND ${CMAKE_COMMAND} -E chdir simgrid-${release_version}/_build make clean
#  COMMAND ${CMAKE_COMMAND} -E remove_directory simgrid-${release_version}/
)
add_dependencies(distcheck dist-dir)

#######################################
### Fill in the "make check" target ###
#######################################

add_custom_target(check
COMMAND make test
#WORKING_DIRECTORY "${PROJECT_DIRECTORY}"
)

#######################################
### Fill in the "make all-clean" target ###
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

add_custom_target(doc-clean
COMMAND ${CMAKE_COMMAND} -E remove -f doc/all_bib.html
COMMAND ${CMAKE_COMMAND} -E remove -f doc/all_bib.latin1.html
COMMAND ${CMAKE_COMMAND} -E remove -f doc/all_bib.latin1.html.tmp
COMMAND ${CMAKE_COMMAND} -E remove -f doc/logcategories.sh
COMMAND ${CMAKE_COMMAND} -E remove -f doc/publis_core.bib
COMMAND ${CMAKE_COMMAND} -E remove -f doc/publis_core_bib.html
COMMAND ${CMAKE_COMMAND} -E remove -f doc/publis_core_bib.latin1.html
COMMAND ${CMAKE_COMMAND} -E remove -f doc/publis_core_bib.latin1.html.tmp
COMMAND ${CMAKE_COMMAND} -E remove -f doc/publis_count.html
COMMAND ${CMAKE_COMMAND} -E remove -f doc/publis_extern.bib
COMMAND ${CMAKE_COMMAND} -E remove -f doc/publis_extern_bib.html
COMMAND ${CMAKE_COMMAND} -E remove -f doc/publis_extern_bib.latin1.html
COMMAND ${CMAKE_COMMAND} -E remove -f doc/publis_extern_bib.latin1.html.tmp
COMMAND ${CMAKE_COMMAND} -E remove -f doc/publis_intra.bib
COMMAND ${CMAKE_COMMAND} -E remove -f doc/publis_intra_bib.html
COMMAND ${CMAKE_COMMAND} -E remove -f doc/publis_intra_bib.latin1.html
COMMAND ${CMAKE_COMMAND} -E remove -f doc/publis_intra_bib.latin1.html.tmp
COMMAND ${CMAKE_COMMAND} -E remove -f doc/tmp.realtoc
COMMAND ${CMAKE_COMMAND} -E remove -f doc/using_bib.html
COMMAND ${CMAKE_COMMAND} -E remove -f doc/using_bib.latin1.html
COMMAND ${CMAKE_COMMAND} -E remove -f doc/using_bib.latin1.html.tmp
COMMAND ${CMAKE_COMMAND} -E remove -f doc/realtoc.sh
WORKING_DIRECTORY "${PROJECT_DIRECTORY}"
)

add_custom_target(java-clean
COMMAND ${CMAKE_COMMAND} -E remove -f src/simgrid.jar
COMMAND ${CMAKE_COMMAND} -E remove_directory src/.classes
COMMAND ${CMAKE_COMMAND} -E remove -f examples/java/basic/BasicTest.class
COMMAND ${CMAKE_COMMAND} -E remove -f examples/java/basic/FinalizeTask.class
COMMAND ${CMAKE_COMMAND} -E remove -f examples/java/basic/Forwarder.class
COMMAND ${CMAKE_COMMAND} -E remove -f examples/java/basic/Master.class
COMMAND ${CMAKE_COMMAND} -E remove -f examples/java/basic/Slave.class
COMMAND ${CMAKE_COMMAND} -E remove -f examples/java/comm_time/CommTimeTest.class
COMMAND ${CMAKE_COMMAND} -E remove -f examples/java/comm_time/FinalizeTask.class
COMMAND ${CMAKE_COMMAND} -E remove -f examples/java/comm_time/Master.class
COMMAND ${CMAKE_COMMAND} -E remove -f examples/java/comm_time/Slave.class
COMMAND ${CMAKE_COMMAND} -E remove -f examples/java/ping_pong/PingPongTask.class
COMMAND ${CMAKE_COMMAND} -E remove -f examples/java/ping_pong/PingPongTest.class
COMMAND ${CMAKE_COMMAND} -E remove -f examples/java/ping_pong/Receiver.class
COMMAND ${CMAKE_COMMAND} -E remove -f examples/java/ping_pong/Sender.class
COMMAND ${CMAKE_COMMAND} -E remove -f examples/java/suspend/DreamMaster.class
COMMAND ${CMAKE_COMMAND} -E remove -f examples/java/suspend/LazyGuy.class
COMMAND ${CMAKE_COMMAND} -E remove -f examples/java/suspend/SuspendTest.class
WORKING_DIRECTORY "${PROJECT_DIRECTORY}"
)

add_custom_target(all-clean
COMMAND make clean
COMMAND make java-clean
COMMAND make doc-clean
COMMAND make supernovae-clean
)
if(enable_maintainer_mode)
	add_custom_command(TARGET all-clean
	COMMAND make maintainer-clean
	)
endif(enable_maintainer_mode)

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
