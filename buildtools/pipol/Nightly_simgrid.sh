#!/bin/bash

#PRE-PIPOL /home/mescal/navarro/pre-simgrid.sh

#PIPOL esn i386_mac-mac-osx-server-leopard.dd.gz none 02:00  --user --silent

#PIPOL esn i386-linux-ubuntu-intrepid.dd.gz none 02:00 --user --silent

#PIPOL esn i386-linux-ubuntu-jaunty.dd.gz none 02:00 --user --silent

#PIPOL esn i386-linux-ubuntu-karmic.dd.gz none 02:00 --user --silent
#PIPOL esn amd64-linux-ubuntu-karmic.dd.gz none 02:00 --user --silent

#PIPOL esn i386-linux-debian-lenny.dd.gz none 02:00 --user --silent
#PIPOL esn amd64-linux-debian-lenny.dd.gz none 02:00 --user --silent

#PIPOL esn i386-linux-debian-testing.dd none 02:00 --user --silent

#PIPOL esn i386-linux-fedora-core11.dd.gz none 02:00 --user --silent

#PIPOL esn i386-linux-fedora-core12.dd.gz none 02:00 --user --silent
#PIPOL esn amd64-linux-fedora-core12.dd.gz none 02:00 --user --silent

SYSTEM=`uname`

if [ -e ./pipol ] ; then
	rm -rf ./pipol/$PIPOL_HOST
	mkdir ./pipol/$PIPOL_HOST
else
	mkdir ./pipol
	rm -rf ./pipol/$PIPOL_HOST
	mkdir ./pipol/$PIPOL_HOST
fi
cd ./pipol/$PIPOL_HOST

sh /home/mescal/navarro/liste_install.sh

svn checkout svn://scm.gforge.inria.fr/svn/simgrid/simgrid/trunk simgrid-trunk --quiet
cd simgrid-trunk

#Make the ucontext mode
cmake -Dwith_context=ucontext -Denable_coverage=on ./
ctest -D NightlyStart
ctest -D NightlyConfigure
ctest -D NightlyBuild
ctest -D NightlyTest
ctest -D NightlyCoverage
ctest -D NightlySubmit
make clean

#Make the pthread mode
cmake -Dwith_context=pthread ./
ctest -D NightlyStart
ctest -D NightlyConfigure
ctest -D NightlyBuild
ctest -D NightlyTest
ctest -D NightlyCoverage
ctest -D NightlySubmit
make clean

#Make the tracing mode
cmake -Dwith_context=auto -Denable_tracing=on ./
ctest -D NightlyStart
ctest -D NightlyConfigure
ctest -D NightlyBuild
ctest -D NightlyTest
ctest -D NightlyCoverage
ctest -D NightlySubmit
make clean

#Make the model-checking mode
cmake -Denable_model-checking=on ./
ctest -D NightlyStart
ctest -D NightlyConfigure
ctest -D NightlyBuild
ctest -D NightlyTest
ctest -D NightlyCoverage
ctest -D NightlySubmit
make clean

#Make the full flags mode
cmake -Denable_model-checking=off -Denable_tracing=off -Denable_compile_warnings=on -Denable_compile_optimizations=on -Denable_lua=off -Denable_java=off -Denable_ruby=off ./
ctest -D NightlyStart
ctest -D NightlyConfigure
ctest -D NightlyBuild
ctest -D NightlyTest
ctest -D NightlyCoverage
ctest -D NightlySubmit
make clean

#Make the supernovae mode
cmake -Dsupernovae=on ./
ctest -D NightlyStart
ctest -D NightlyConfigure
ctest -D NightlyBuild
ctest -D NightlyTest
ctest -D NightlyCoverage
ctest -D NightlySubmit
make clean

if [ $SYSTEM = Linux ] ; then
	cd ..
	home_dir=`pwd`

	svn checkout svn://scm.gforge.inria.fr/svn/simgrid/contrib/trunk/GTNetS/ --quiet
	cd GTNetS
	unzip gtnets-current.zip > /dev/null
	tar zxvf gtnets-current-patch.tgz  > /dev/null
	cd gtnets-current
	cat ../00*.patch | patch -p1 > /dev/null

	ARCH_32=`uname -m | cut -d'_' -f2`

	if [ x$ARCH_32 = x64 ] ; then #only if 64 bit processor family
	cat ../AMD64-FATAL-Removed-DUL_SIZE_DIFF-Added-fPIC-compillin.patch | patch -p1 > /dev/null
	fi

	ln -sf Makefile.linux Makefile
	make -j 3 depend > /dev/null
	make -j 3 debug > /dev/null 2>&1
	make -j 3 opt > /dev/null 2>&1
	wait
	cd $home_dir
	absolute_path=`pwd`
	userhome=$absolute_path

	if [ -e $userhome/usr/lib ] ; then
		echo ""
	else
		mkdir $userhome/usr	
		mkdir $userhome/usr/lib
	fi

	if [ -e $userhome/usr/include ] ; then
		echo ""
	else	
		mkdir $userhome/usr/include 
	fi

	cp -fr $absolute_path/GTNetS/gtnets-current/*.so $userhome/usr/lib/
	ln -sf $userhome/usr/lib/libgtsim-opt.so $userhome/usr/lib/libgtnets.so

	export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$userhome/usr/lib/
	mkdir $userhome/usr/include/gtnets
	cp -fr $absolute_path/GTNetS/gtnets-current/SRC/*.h $userhome/usr/include/gtnets
	wait
	cd $home_dir
	rm -rf $absolute_path/GTNetS
	cd simgrid-trunk
	
	if [ -e $userhome/usr/lib/libgtsim-opt.so ] ; then
		#Make gtnets
		cmake -Dsupernovae=off -Denable_compile_warnings=off -Denable_compile_optimizations=off -Dgtnets_path=$absolute_path/usr ./
		ctest -D NightlyStart
		ctest -D NightlyConfigure
		ctest -D NightlyBuild
		ctest -D NightlyTest
		ctest -D NightlyCoverage
		ctest -D NightlySubmit
		make clean
	fi
fi

if( $PIPOL_IMAGE = i386-linux-ubuntu-karmic.dd.gz )
	#Make the memcheck mode
	cmake -Denable_gtnets=off ./
	ctest -D NightlyStart
	ctest -D NightlyConfigure
	ctest -D NightlyBuild
	ctest -D NightlyMemCheck
	ctest -D NightlySubmit
	make clean
fi