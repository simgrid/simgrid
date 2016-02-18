LIB_JARS= lib.jar

all: compile doc

compile:
	mkdir -p classes
	javac -sourcepath src -classpath $(LIB_JARS):../../simgrid.jar -d classes `find -L -name "*.java"`
doc:
	mkdir -p doc
	javadoc -sourcepath src -classpath $(LIB_JARS):../../simgrid.jar -d doc psgsim
test:
	./test.sh 
clean: 
	rm -rf classes doc outputs
help:
	@echo "The following are a valid targets for this Makefile:"
	@echo "................ all (the default if no target is provided)"
	@echo "................ compile"
	@echo "................ doc"
	@echo "................ test"
	@echo "................ clean"

