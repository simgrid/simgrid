#Need python python-dev
sudo apt-get install python python-dev
	
SYSTEM=`uname`
if [ $SYSTEM = Linux ] ; then
	if [ x$1 != x ]  ; then
		prefix=$1;
	fi
	
	if [ -e $prefix ] ; then
		echo -n ""
	else	
		echo "Creating directory $prefix"
		mkdir -p $prefix;
	fi
	
	if [ -e $prefix/lib ] ; then
		echo -n ""
	else	
		echo "Creating directory $prefix/lib"
		mkdir -p $prefix/lib;
	fi
	
	if [ -e $prefix/include/ns3 ] ; then
		echo -n ""
	else	
		echo "Creating directory $prefix/include/ns3"
		mkdir -p $prefix/include/ns3;
	fi
	
	if [ -e $prefix/doc/html ] ; then
		echo -n ""
	else	
		echo "Creating directory $prefix/doc/html"
		mkdir -p $prefix/doc/html;
	fi
	
	localdir=`pwd`
	cd $prefix
	prefix=`pwd`
	cd $localdir
	echo "Install to prefix = $prefix"
	
	echo "Downloading NS3"
	wget http://ns-3.googlecode.com/files/ns-allinone-3.10.tar.bz2
	
	echo "Uncompressing package";
	tar -xvjf ns-allinone-3.10.tar.bz2
	rm -rf ns-allinone-3.10.tar.bz2
	cd ns-allinone-3.10/ns-3.10
	
	./waf configure
	./waf
	./waf --doxygen
	
	cp -f build/debug/libns3.so $prefix/lib/libns3.so
	cp -f build/debug/ns3/* $prefix/include/ns3/
	cp -f doc/html/* $prefix/doc/html/
fi