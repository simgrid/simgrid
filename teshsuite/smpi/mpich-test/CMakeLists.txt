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
  
set(stdo_std_smpi
  ${CMAKE_CURRENT_SOURCE_DIR}/mpich-test/context/attrt.std
  ${CMAKE_CURRENT_SOURCE_DIR}/mpich-test/context/attrerr.std
  ${CMAKE_CURRENT_SOURCE_DIR}/mpich-test/context/context.std
  ${CMAKE_CURRENT_SOURCE_DIR}/mpich-test/context/commnames.std
  ${CMAKE_CURRENT_SOURCE_DIR}/mpich-test/pt2pt/self.std
  ${CMAKE_CURRENT_SOURCE_DIR}/mpich-test/pt2pt/sndrcvrep.std
  ${CMAKE_CURRENT_SOURCE_DIR}/mpich-test/pt2pt/waitany.std
  ${CMAKE_CURRENT_SOURCE_DIR}/mpich-test/pt2pt/reqcreate.std
  ${CMAKE_CURRENT_SOURCE_DIR}/mpich-test/pt2pt/persistent.std
  ${CMAKE_CURRENT_SOURCE_DIR}/mpich-test/pt2pt/allpair2.std
  ${CMAKE_CURRENT_SOURCE_DIR}/mpich-test/pt2pt/typeub3.std
  ${CMAKE_CURRENT_SOURCE_DIR}/mpich-test/pt2pt/waitall.std
  ${CMAKE_CURRENT_SOURCE_DIR}/mpich-test/pt2pt/nullproc2.std
  ${CMAKE_CURRENT_SOURCE_DIR}/mpich-test/pt2pt/allpair.std
  ${CMAKE_CURRENT_SOURCE_DIR}/mpich-test/pt2pt/hindexed.std
  ${CMAKE_CURRENT_SOURCE_DIR}/mpich-test/pt2pt/ssendtest2.std
  ${CMAKE_CURRENT_SOURCE_DIR}/mpich-test/pt2pt/sendmany.std
  ${CMAKE_CURRENT_SOURCE_DIR}/mpich-test/pt2pt/nullproc.std
  ${CMAKE_CURRENT_SOURCE_DIR}/mpich-test/pt2pt/nbtest.std
  ${CMAKE_CURRENT_SOURCE_DIR}/mpich-test/pt2pt/sendcplx.stdo
  ${CMAKE_CURRENT_SOURCE_DIR}/mpich-test/pt2pt/hvec.std
  ${CMAKE_CURRENT_SOURCE_DIR}/mpich-test/pt2pt/typebase.std
  ${CMAKE_CURRENT_SOURCE_DIR}/mpich-test/pt2pt/typeub2.std
  ${CMAKE_CURRENT_SOURCE_DIR}/mpich-test/pt2pt/structf.std
  ${CMAKE_CURRENT_SOURCE_DIR}/mpich-test/pt2pt/typecreate.std
  ${CMAKE_CURRENT_SOURCE_DIR}/mpich-test/pt2pt/nblock.std
  ${CMAKE_CURRENT_SOURCE_DIR}/mpich-test/topol/cartmap.std
  ${CMAKE_CURRENT_SOURCE_DIR}/mpich-test/topol/graphtest.std
  ${CMAKE_CURRENT_SOURCE_DIR}/mpich-test/topol/cartf.std
  ${CMAKE_CURRENT_SOURCE_DIR}/mpich-test/env/aborttest.std
  ${CMAKE_CURRENT_SOURCE_DIR}/mpich-test/env/errstringsf.std
  ${CMAKE_CURRENT_SOURCE_DIR}/mpich-test/env/env.std
  ${CMAKE_CURRENT_SOURCE_DIR}/mpich-test/profile/ptest.std
  ${CMAKE_CURRENT_SOURCE_DIR}/mpich-test/coll/allredmany.std
  ${CMAKE_CURRENT_SOURCE_DIR}/mpich-test/coll/allred.std
  ${CMAKE_CURRENT_SOURCE_DIR}/mpich-test/coll/allredf.std
  ${CMAKE_CURRENT_SOURCE_DIR}/mpich-test/coll/scatterv.std
)

if("${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_HOME_DIRECTORY}")
else()
    foreach(srcfile ${stdo_std_smpi})
        set(dstfile ${srcfile})
        string(REPLACE "${CMAKE_HOME_DIRECTORY}" "${CMAKE_BINARY_DIR}" dstfile "${dstfile}")
        string(REGEX REPLACE "/[^/]*.std" "/" dstfile "${dstfile}")
        string(REGEX REPLACE "/[^/]*.stdo" "/" dstfile "${dstfile}")
        #message("copy ${srcfile} to ${dstfile}")
        file(COPY ${srcfile} DESTINATION ${dstfile})
    endforeach()
endif()

set(txt_files
  ${txt_files}
  ${stdo_std_smpi}
  PARENT_SCOPE
  )
