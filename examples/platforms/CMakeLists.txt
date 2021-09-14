add_custom_target(platf_cpp COMMENT "C++ platform description")
add_dependencies(tests platf_cpp)
foreach (platf routing_cluster griffon)
  add_library       (${platf} SHARED ${platf}.cpp)
  target_link_libraries(${platf} simgrid)
  add_dependencies(platf_cpp ${platf})
  set_target_properties(${platf}
      PROPERTIES SUFFIX .so
  )
endforeach()
