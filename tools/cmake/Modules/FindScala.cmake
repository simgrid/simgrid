
find_program(SCALA_COMPILE
  NAMES scalac
  PATH_SUFFIXES bin/
  PATHS
  /opt
  /opt/local
  /opt/csw
  /sw
  /usr
  )

message(STATUS "Looking for scalac")
if(SCALA_COMPILE)
  message(STATUS "Looking for scalac - found")
else()
  message(STATUS "Looking for scalac - not found")
endif()

set(SCALA_JARS "/usr/share/java/scala-compiler.jar:/usr/share/java/scala-library.jar:/usr/share/java/scalap.jar:/usr/share/java/scala/jline.jar:/usr/share/java/jansi.jar")

