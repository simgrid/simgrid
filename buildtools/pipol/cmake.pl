#!/usr/bin/perl -w
use strict;

my($my_cmake_version);
my($cmake);
my($ctest);
my($cpack);

$my_cmake_version = `cmake --version`;
$cmake = `which cmake`;
$ctest = `which ctest`;
$cpack = `which cpack`;
print "$cmake";
print "$ctest";
print "$cpack";

chomp $cmake;
chomp $ctest;
chomp $cpack;

if ( -f "$cmake" )
{
	if($my_cmake_version =~ /.*2.8.[0-9].*$/) 
	{
		print "cmake version : $my_cmake_version";
	}
	else
	{
		if($my_cmake_version =~ /.*2.6.[1-9].*$/) 
		{
			print "version > 2.6.0\n";
			system "wget http://www.cmake.org/files/v2.8/cmake-2.8.2.tar.gz";
			system "tar zxvf cmake-2.8.2.tar.gz";
			chdir("./cmake-2.8.2");
			system "cmake .";
			system "make";
			if (-f "./bin/cmake"){
				system "sudo ln -sf `pwd`/bin/cmake $cmake";
				system "sudo ln -sf `pwd`/bin/cpack $cpack";
				system "sudo ln -sf `pwd`/bin/ctest $ctest";
			}
			else
			{
				return;
			}
	
		}
		else
		{
			print "version < 2.6.1\n";
			system "wget http://www.cmake.org/files/v2.6/cmake-2.6.4.tar.gz";
			system "wget http://www.cmake.org/files/v2.8/cmake-2.8.2.tar.gz";
	
			system "tar zxvf cmake-2.6.4.tar.gz";
			chdir("./cmake-2.6.4/");
			system "cmake .";
			system "make";
			if (-f "./bin/cmake"){
				system "sudo ln -sf `pwd`/bin/cmake $cmake";
				system "sudo ln -sf `pwd`/bin/cpack $cpack";
				system "sudo ln -sf `pwd`/bin/ctest $ctest";
			}
			else
			{
				return;
			}
			chdir("./..");
	
			$my_cmake_version = `cmake --version`;
			print "cmake version : $my_cmake_version";
			$cmake = `which cmake`;
			$ctest = `which ctest`;
			$cpack = `which cpack`;
			print "$cmake";
			print "$ctest";
			print "$cpack";
			chomp $cmake;
			chomp $ctest;
			chomp $cpack;
	
			system "tar zxvf cmake-2.8.2.tar.gz";
			chdir("./cmake-2.8.2/");
			system "cmake .";
			system "make";
			if (-f "./bin/cmake"){
				system "sudo ln -sf `pwd`/bin/cmake $cmake";
				system "sudo ln -sf `pwd`/bin/cpack $cpack";
				system "sudo ln -sf `pwd`/bin/ctest $ctest";
			}
			else
			{
				return;
			}
		}
	$my_cmake_version = `cmake --version`;
	print "cmake version : $my_cmake_version";
	$cmake = `which cmake`;
	$ctest = `which ctest`;
	$cpack = `which cpack`;
	print "$cmake";
	print "$ctest";
	print "$cpack";
	}
}