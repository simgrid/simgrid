set(tesh_files
  ${tesh_files}
  
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  PARENT_SCOPE
  )
set(examples_src
  ${examples_src}
  PARENT_SCOPE
  )
set(bin_files
  ${bin_files}
  PARENT_SCOPE
  )
  
#set(stdo_std_smpi
#  ${CMAKE_CURRENT_SOURCE_DIR}/topol/cartmap.std
#  ${CMAKE_CURRENT_SOURCE_DIR}/topol/graphtest.std
#  ${CMAKE_CURRENT_SOURCE_DIR}/topol/cartf.std
#)

if("${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_HOME_DIRECTORY}")
else()
    foreach(srcfile ${stdo_std_smpi})
        set(dstfile ${srcfile})
        string(REPLACE "${CMAKE_HOME_DIRECTORY}" "${CMAKE_BINARY_DIR}" dstfile "${dstfile}")
        #message("copy ${srcfile} to ${dstfile}")
        configure_file("${srcfile}" "${dstfile}" COPYONLY)
    endforeach()
endif()

set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/README
  ${CMAKE_CURRENT_SOURCE_DIR}/runtests
  ${CMAKE_CURRENT_SOURCE_DIR}/testlist
  ${CMAKE_CURRENT_SOURCE_DIR}/hostfile
  ${CMAKE_CURRENT_SOURCE_DIR}/checktests
  ${CMAKE_CURRENT_SOURCE_DIR}/util/mtest.c
  ${CMAKE_CURRENT_SOURCE_DIR}/f77/testlist
  ${CMAKE_CURRENT_SOURCE_DIR}/include/mpitestconf.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/mpitest.h
  PARENT_SCOPE)
