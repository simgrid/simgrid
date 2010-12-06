if [ x$1 != x ]  ; then
	echo "Install to prefix=$1";
	prefix=$1;
fi

if [ -e $prefix ] ; then
	echo -n ""
else	
	echo "Creating directory $prefix";
	mkdir -p $prefix;
fi

echo "Downloading GTNetS from SVN SimGrid's repository";
svn checkout svn://scm.gforge.inria.fr/svn/simgrid/contrib/trunk/GTNetS/ --quiet
cd GTNetS
echo "Uncompressing package";
unzip gtnets-current.zip > /dev/null
tar zxvf gtnets-current-patch.tgz  > /dev/null
cd gtnets-current
cat ../00*.patch | patch -p1 > /dev/null

ARCH_32=`uname -m | cut -d'_' -f2`

if [ x$ARCH_32 = x64 ] ; then #only if 64 bit processor family
cat ../AMD64-FATAL-Removed-DUL_SIZE_DIFF-Added-fPIC-compillin.patch | patch -p1 > /dev/null
fi

ln -sf Makefile.linux Makefile
echo "Creating dependencies";
make -j 3 depend > /dev/null
echo "Compiling GTNetS debug libs";
make -j 3 debug > /dev/null 2>&1
echo "Compiling GTNetS optimal libs";
make -j 3 opt > /dev/null 2>&1
wait

if [ -e $prefix/lib ] ; then
	echo -n ""
else	
	echo "Creating directory $prefix/lib";
	mkdir $prefix/lib;
fi

if [ -e $prefix/include ] ; then
	echo -n "";
else	
	echo "Creating directory $prefix/include";
	mkdir $prefix/include;
fi

if [ -e $prefix/include/gtnets ] ; then
	echo -n "";
else	
	echo "Creating directory $prefix/include/gtnets";
	mkdir $prefix/include/gtnets;
fi

echo "Copying files to $prefix/lib";
cp -fr ./*.so $prefix/lib/
ln -sf $prefix/lib/libgtsim-opt.so $prefix/lib/libgtnets.so

echo "Copying files to $prefix/include/gtnets";
cp -fr ./SRC/*.h $prefix/include/gtnets
wait

echo "Done with gtnets installation";

