find_path(HAVE_CGRAPH_H cgraph.h
  HINTS
  $ENV{LD_LIBRARY_PATH}
  PATH_SUFFIXES include/graphviz include
  PATHS
  /opt
  /opt/local
  /opt/csw
  /sw
  /usr
  )

find_path(HAVE_AGRAPH_H agraph.h
  HINTS
  $ENV{LD_LIBRARY_PATH}
  PATH_SUFFIXES include/graphviz include
  PATHS
  /opt
  /opt/local
  /opt/csw
  /sw
  /usr
  )

find_path(HAVE_GRAPH_H graph.h
  HINTS
  $ENV{LD_LIBRARY_PATH}
  PATH_SUFFIXES include/graphviz include
  PATHS
  /opt
  /opt/local
  /opt/csw
  /sw
  /usr
  )

find_library(HAVE_CGRAPH_LIB
  NAME cgraph
  HINTS
  $ENV{LD_LIBRARY_PATH}
  PATH_SUFFIXES lib/graphviz lib
  PATHS
  /opt
  /opt/local
  /opt/csw
  /sw
  /usr
  )

find_library(HAVE_AGRAPH_LIB
  NAME agraph
  HINTS
  $ENV{LD_LIBRARY_PATH}
  PATH_SUFFIXES lib/graphviz lib
  PATHS
  /opt
  /opt/local
  /opt/csw
  /sw
  /usr
  )

find_library(HAVE_GRAPH_LIB
  NAME graph
  HINTS
  $ENV{LD_LIBRARY_PATH}
  PATH_SUFFIXES lib/graphviz lib
  PATHS
  /opt
  /opt/local
  /opt/csw
  /sw
  /usr
  )

find_library(HAVE_CDT_LIB
  NAME cdt
  HINTS
  $ENV{LD_LIBRARY_PATH}
  PATH_SUFFIXES lib/graphviz lib
  PATHS
  /opt
  /opt/local
  /opt/csw
  /sw
  /usr
  )

mark_as_advanced(HAVE_AGRAPH_H)
mark_as_advanced(HAVE_CGRAPH_H)
mark_as_advanced(HAVE_GRAPH_H)
mark_as_advanced(HAVE_GRAPH_LIB)
mark_as_advanced(HAVE_CGRAPH_LIB)
mark_as_advanced(HAVE_AGRAPH_LIB)
mark_as_advanced(HAVE_CDT_LIB)

### Initialize of cgraph
if(HAVE_CDT_LIB)
  if(HAVE_CGRAPH_LIB OR HAVE_AGRAPH_LIB)

    if(HAVE_AGRAPH_LIB)
      string(REGEX REPLACE "/libagraph.*" "" lib_graphviz ${HAVE_AGRAPH_LIB})
    else(HAVE_AGRAPH_LIB)
      if(HAVE_CGRAPH_LIB)
     	string(REGEX REPLACE "/libcgraph.*" "" lib_graphviz ${HAVE_CGRAPH_LIB})
      endif(HAVE_CGRAPH_LIB)
    endif(HAVE_AGRAPH_LIB)

    if(HAVE_GRAPH_H OR HAVE_AGRAPH_H OR HAVE_CGRAPH_H)

      if(HAVE_GRAPH_H)
	string(REPLACE "/graphviz/graph.h" "" file_graphviz_h ${HAVE_GRAPH_H})
	string(REPLACE "/graphviz" "" file_graphviz_h ${file_graphviz_h})
	set(GRAPH_H 1)
      endif(HAVE_GRAPH_H)

      if(HAVE_AGRAPH_H)
	string(REPLACE "/graphviz/agraph.h" "" file_graphviz_h ${HAVE_AGRAPH_H})
	string(REPLACE "/graphviz" "" file_graphviz_h ${file_graphviz_h})
	set(AGRAPH_H 1)
      endif(HAVE_AGRAPH_H)

      if(HAVE_CGRAPH_H)
	string(REPLACE "/graphviz/cgraph.h" "" file_graphviz_h ${HAVE_CGRAPH_H})
	string(REPLACE "/graphviz" "" file_graphviz_h ${file_graphviz_h})
	set(CGRAPH_H 1)
      endif(HAVE_CGRAPH_H)

      include_directories(${file_graphviz_h} ${file_graphviz_h}/graphviz)
      link_directories(${lib_graphviz})

      set(HAVE_GRAPHVIZ "1")
    else(HAVE_GRAPH_H OR HAVE_AGRAPH_H OR HAVE_CGRAPH_H)
      set(HAVE_GRAPHVIZ "0")
    endif(HAVE_GRAPH_H OR HAVE_AGRAPH_H OR HAVE_CGRAPH_H)

  else(HAVE_CGRAPH_LIB OR HAVE_AGRAPH_LIB)
    set(HAVE_GRAPHVIZ "0")
  endif(HAVE_CGRAPH_LIB OR HAVE_AGRAPH_LIB)

endif(HAVE_CDT_LIB)

mark_as_advanced(HAVE_GRAPHVIZ)

message(STATUS "Looking for agraph.h")
if(HAVE_AGRAPH_H)
  message(STATUS "Looking for agraph.h - found")
else(HAVE_AGRAPH_H)
  message(STATUS "Looking for agraph.h - not found")
endif(HAVE_AGRAPH_H)

message(STATUS "Looking for cgraph.h")
if(HAVE_CGRAPH_H)
  message(STATUS "Looking for cgraph.h - found")
else(HAVE_CGRAPH_H)
  message(STATUS "Looking for cgraph.h - not found")
endif(HAVE_CGRAPH_H)

message(STATUS "Looking for graph.h")
if(HAVE_GRAPH_H)
  message(STATUS "Looking for graph.h - found")
else(HAVE_GRAPH_H)
  message(STATUS "Looking for graph.h - not found")
endif(HAVE_GRAPH_H)

message(STATUS "Looking for lib agraph")
if(HAVE_AGRAPH_LIB)
  message(STATUS "Looking for lib agraph - found")
else(HAVE_AGRAPH_LIB)
  message(STATUS "Looking for lib agraph - not found")
endif(HAVE_AGRAPH_LIB)

message(STATUS "Looking for lib cgraph")
if(HAVE_CGRAPH_LIB)
  message(STATUS "Looking for lib cgraph - found")
else(HAVE_CGRAPH_LIB)
  message(STATUS "Looking for lib cgraph - not found")
endif(HAVE_CGRAPH_LIB)

message(STATUS "Looking for lib graph")
if(HAVE_GRAPH_LIB)
  message(STATUS "Looking for lib graph - found")
else(HAVE_GRAPH_LIB)
  message(STATUS "Looking for lib graph - not found")
endif(HAVE_GRAPH_LIB)

message(STATUS "Looking for lib cdt")
if(HAVE_CDT_LIB)
  message(STATUS "Looking for lib cdt - found")
else(HAVE_CDT_LIB)
  message(STATUS "Looking for lib cdt - not found")
endif(HAVE_CDT_LIB)