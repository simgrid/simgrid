# Testing the Java bindings

## Dependencies: SWIG and Open JDK

```
apt install swig openjdk-21-jdk-headless
```

## Configuring SimGrid

Configure SimGrid to enable the Java bindings. Since this option was
blacklisted recently, you may need to edit your CMakeCache.txt and
remove everything about Java from that file. Or simply wipe that file,
or start a new build tree for Java.

```
cmake -Denable_java=ON -GNinja .
```

## Compiling the bindings

```
ninja simgrid_jar simgrid-java
```

Mind the _ in `simgrid_jar`. There is a target called `simgrid.jar`,
but it's not complete: the jarfile produced with `ninja simgrid.jar`
is almost empty. I dunno how to do otherwise, because the Java files
are generated so they are not here at configure time when cmake
expects them.

## Preparing the test directory

Create a directory with all these files together. The best is to make
symbolic links so that your copy won't get obsolete when recompiling.

- simgrid.jar: You'll find it at the root of your build tree
- libsimgrid-java.so  libsimgrid-java.so.3.36.1 libsimgrid.so libsimgrid.so.3.36.1: search under lib/
- src/bindings/swig/simgrid_runme.java
- examples/platforms/small_platform.xml

## Compile the test case

```
javac -cp simgrid.jar:. simgrid_runme.java
```

## Let it roll, baby

```
LD_LIBRARY_PATH=. java -cp simgrid.jar:. simgrid_runme
```

## Observe the segfault

Try to fix it.

Repeat.