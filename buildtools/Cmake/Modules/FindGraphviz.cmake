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
         
        string(REGEX MATCH "-I${file_graphviz_h} " operation "${CMAKE_C_FLAGS}")
    	if(NOT operation)
    		SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}-I${file_graphviz_h} ")
    	endif(NOT operation)
    	
    	string(REGEX MATCH "-I${file_graphviz_h}/graphviz " operation "${CMAKE_C_FLAGS}")
    	if(NOT operation)
    		SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}-I${file_graphviz_h}/graphviz ")
    	endif(NOT operation)
    	
    	string(REGEX MATCH "-L${lib_graphviz} " operation "${CMAKE_C_FLAGS}")
    	if(NOT operation)
    		SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}-L${lib_graphviz} ")
    	endif(NOT operation)
	
    	set(HAVE_GRAPHVIZ "1")
    else(HAVE_GRAPH_H OR HAVE_AGRAPH_H OR HAVE_CGRAPH_H)
        set(HAVE_GRAPHVIZ "0")
    endif(HAVE_GRAPH_H OR HAVE_AGRAPH_H OR HAVE_CGRAPH_H)
    
else(HAVE_CGRAPH_LIB OR HAVE_AGRAPH_LIB)
        set(HAVE_GRAPHVIZ "0")
endif(HAVE_CGRAPH_LIB OR HAVE_AGRAPH_LIB)

endif(HAVE_CDT_LIB)

mark_as_advanced(HAVE_GRAPHVIZ)