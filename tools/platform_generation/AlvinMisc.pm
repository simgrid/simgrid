#!/usr/bin/perl -w
package AlvinMisc;
use strict;
#require 5.005_64;
#our($VERSION, @EXPORT_OK, @ISA);
use Carp;
use Symbol;
use SelectSaver;
use IO ();      # Load the XS module
use IO::Handle;
use English;
require Exporter;
use vars qw($VERSION @ISA @EXPORT_OK);
@ISA = qw(Exporter);

$VERSION = "1.21";

@EXPORT_OK = qw(
   host
   sys_arch   

   min
   max
   beep
   approx

   melange
   alea_gaussien
   random
   canonical_name
);

our($canonical_name);
sub set_canonical_name {
    $canonical_name=$PROCESS_ID.host();
#    $canonical_name="4279moby";
}

############# 
# These subroutines determine hostnames, archs, ...
#
sub host() {
    my($un);
    $un=`uname -n`; chomp($un);
    return $un;
}

sub sys_arch() {
    my($us,$ur,$up);
    $us = `uname -s`; chomp($us);
    $ur = `uname -r`; chomp($ur);
    $up = `uname -p`; chomp($up);
    return $us."-".$ur."-".$up;
}

sub moyenne {
    my($moy)=0;

    if(scalar(@_)==0) {return $moy;}
    foreach (@_) {
	$moy+=$_;
    }
    $moy/=scalar(@_);
    return $moy;
}

#############
# min
#
sub min {
    my $min = shift;
    foreach (@_) { $min = $_ if $_ < $min; }
    $min;
}

#############
# max   
#
sub max {
    my $max = shift;
    foreach (@_) { $max = $_ if $_ > $max; }
    $max;
}


#############
# beep !!
#
sub beep{
  print STDOUT "\a";
  return;
}

#############
# approx
#
sub approx{
  my($number) = shift;
  my($e);

  $e = int($number);
  if (($number-$e) < 0.5) {
    return $e;
  } else {
    return ($e+1);
  }
}

######################## fonctions aléatoires diverses #############################

sub melange {
    my $tableau=shift;
    my($i,$j);
    
    for($i = @$tableau ; --$i; ) {
	$j = int rand ($i+1);
	next if $i==$j;
	@$tableau[$i,$j] = @$tableau[$j,$i];
    }
}

sub alea_gaussien {
    my($u1,$u2);
    my $w;
    my($g1, $g2);

    do {
	$u1 = 2*rand() -1;
	$u2 = 2*rand() -1;
	$w = $u1*$u1 + $u2*$u2 ;
    } while ( $w >=1 || $w==0 );

    $w = sqrt ( (-2*log($w))/$w );
    $g2 = $u1 * $w;
    $g1 = $u2 * $w;
    return($g1);    
}

sub random {
    my($min,$max)=@_;
    return ($min+rand($max-$min));
}


srand;

set_canonical_name;
  
1;
