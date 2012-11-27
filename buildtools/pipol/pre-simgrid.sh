#! /bin/sh

set -e

rm -rf ~/.pipol/log/*

#if [ -e ~/version/bckup_version ] ; then
#	svn cleanup version/
#	cd ~/version
#	svn up README --quiet
#else
#	if [ -e ~/version ] ; then
#		svn cleanup version/
#		echo "0000P" > ~/version/bckup_version
#	else
#		mkdir ~/version
#		echo "0000P" > ~/version/bckup_version
#	fi
#	cd ~/
#	svn checkout svn://scm.gforge.inria.fr/svn/simgrid/simgrid/trunk ~/version --depth empty --quiet
#
#	cd ~/version
#	svn up README --quiet
#fi

#old_version=`cat bckup_version`
#new_version=`svnversion`
Date=`date`

echo date : $date
#echo version old : $old_version
#echo version svn : $new_version
#echo `svnversion` > bckup_version

#if [ "$old_version" = "$new_version" ] ; then
#	echo "matches"	
#	exit 1
#else
#	echo "not matches"
	exit 0
#fi
