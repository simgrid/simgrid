foreach(file bourassa fafard ginette jupiter link3 link4 link5)
  set(txt_files ${txt_files} ${CMAKE_CURRENT_SOURCE_DIR}/${file}_state.trace)
endforeach()

set(txt_files   ${txt_files} ${CMAKE_CURRENT_SOURCE_DIR}/README  PARENT_SCOPE)
