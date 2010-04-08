#! /bin/sh

set -e

if [ -e /home/mescal/navarro/version/bckup_version ] ; then
	svn cleanup version/
	cd /home/mescal/navarro/version
	svn up README --quiet
else
	if [ -e /home/mescal/navarro/version ] ; then
		svn cleanup version/
		echo "0000P" > /home/mescal/navarro/version/bckup_version
	else
		mkdir /home/mescal/navarro/version
		echo "0000P" > /home/mescal/navarro/version/bckup_version
	fi
	cd /home/mescal/navarro
	svn checkout svn://scm.gforge.inria.fr/svn/simgrid/simgrid/trunk /home/mescal/navarro/version --depth empty --quiet

	cd /home/mescal/navarro/version
	svn up README --quiet
fi

old_version=`cat bckup_version`
new_version=`svnversion`

echo version old : $old_version
echo version svn : $new_version
echo `svnversion` > bckup_version

if [ "$old_version" = "$new_version" ] ; then
	echo "matches"	
	exit 1
else
	echo "not matches"
	exit 0
fi
