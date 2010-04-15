#!/usr/bin/perl -w
use Term::ANSIColor qw{:constants};
$Term::ANSIColor::AUTORESET = 1;

while(<>){
    if($_ =~ m/^[-]\s.*/){
	print BOLD RED $_;
    }elsif ($_ =~ m/^[+]\s.*/){
	print BOLD GREEN $_;
    }else{
	print BOLD $_;
    }
}


