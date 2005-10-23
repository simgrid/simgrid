#! /usr/bin/perl

use strict;

my $progname="sg_unit_extractor";
# Get the args 
die "USAGE: $progname infile [outfile]\n"
  if (scalar @ARGV == 0 || scalar @ARGV > 2);
my ($infile,$outfile) = @ARGV;

if (not defined($outfile)) {
  $outfile =  $infile;
  $outfile =~ s/\.c$/_unit.c/;
  $outfile =~ s|.*/([^/]*)$|$1| if $outfile =~ m|/|;
}

# Get the unit data
my ($unit_source,$suite_name,$suite_title)=("","","");
my (%tests); # to detect multiple definition
my (@tests); # actual content

open IN, "$infile" || die "$progname: Cannot open input file '$infile': $!\n";

my $takeit=0;
while (<IN>) {
  if (m/ifdef +SIMGRID_TEST/) {
    $takeit = 1;
    next;
  }
  if (m/endif.*SIMGRID_TEST/) {
    $takeit = 0;
    next
  }

  if (m/XBT_TEST_SUITE\(\w*"([^"]*)"\w*,(.*?)\);/) { #"
    die "$progname: Multiple suites in the same file ($infile) are not supported yet\n" 
      if length($suite_name);
    ($suite_name,$suite_title)=($1,$2);
    next;
  } 
  
  if (m/XBT_TEST_UNIT\(\w*"([^"]*)"\w*,([^,]*),(.*?)\)/) { #"
    die "$progname: multiply defined test in file $infile: $1\n"
      if (defined($tests{$1}));
      
    my @t=($1,$2,$3);
    push @tests,\@t;
    $tests{$1} = 1;
  }
  $unit_source .= $_ if $takeit;
}
close IN || die "$progname: cannot close input file '$infile': $!\n";


if ($takeit) {
  die "$progname: end of file reached in SIMGRID_TEST block.\n".
     "You should end each of the with a line matching: /endif.*SIMGRID_TEST/\n".
     "Example:\n".
     "#endif /* SIMGRID_TEST */\n"
}

die "$progname: no suite defined in $infile\n"
  unless (length($suite_name));
  
# Write the test

my ($GENERATED)=("/*******************************/\n".
                 "/* GENERATED FILE, DO NOT EDIT */\n".
                 "/*******************************/\n\n");

open OUT,">$outfile" || die "$progname: Cannot open output file '$outfile': $!\n";
print OUT $GENERATED;
print OUT "#include \"xbt.h\"\n";
print OUT $GENERATED;
print OUT "$unit_source";
print OUT $GENERATED;
close OUT || die "$progname: Cannot close output file '$outfile': $!\n";

# write the main skeleton if needed
if (! -e "simgrid_units_main.c") {
  open OUT,">simgrid_units_main.c" || die "$progname: Cannot open main file 'simgrid_units_main.c': $!\n";
  print OUT $GENERATED;
  print OUT "#include \"xbt.h\"\n\n";
  print OUT "/* SGU: BEGIN PROTOTYPES */\n";
  print OUT "/* SGU: END PROTOTYPES */\n\n";
  print OUT $GENERATED;
  print OUT "int main(int argc, char *argv[]) {\n";
  print OUT "  xbt_test_suite_t suite;\n\n";
  print OUT "  /* SGU: BEGIN SUITES DECLARATION */\n";
  print OUT "  /* SGU: END SUITES DECLARATION */\n\n";  
  print OUT "  return xbt_test_run();\n";
  print OUT "}\n";
  print OUT $GENERATED;
  close OUT || die "$progname: Cannot close main file 'simgrid_units_main.c': $!\n";
}

print "  Suite $suite_name: $suite_title (".(scalar @tests)." tests)\n";
map {
  my ($name,$func,$title) = @{$_};
  print "    test $name: func=$func; title=$title\n";
} @tests;

#while (my $t = shift @tests) {

# add this suite to the main
my $newmain="";
open IN,"simgrid_units_main.c" || die "$progname: Cannot open main file 'simgrid_units_main.c': $!\n";
  # search prototypes
  while (<IN>) {
    $newmain .= $_;
#    print "Look for proto: $_";
    last if /SGU: BEGIN PROTOTYPES/;
  }

  # search my prototype
  while (<IN>) {
#    print "Seek protos: $_";
    last if  (/SGU: END PROTOTYPES/ || /SGU: BEGIN FILE $infile/);
    $newmain .= $_;
  }
  if (/SGU: BEGIN FILE $infile/) { # found an old section for this file. Kill it    
    while (<IN>) {
      last if /SGU: END FILE/;
    }
    $_ = <IN>; # pass extra blank line
    chomp;
    die "this line should be blank ($_). Did you edit the file?" if /\W/;
  }
  my ($old_)=($_);
  # add my section
  $newmain .= "  /* SGU: BEGIN FILE $infile */\n";
  map {
    my ($name,$func,$title) = @{$_};
    $newmain .=  "    void $func(xbt_test_unit_t _unit);\n"
  } @tests;

  $newmain .= "  /* SGU: END FILE */\n\n";
  if ($old_ =~ /SGU: BEGIN FILE/ || $old_ =~ /SGU: END PROTOTYPES/) {
    $newmain .= $old_;
  }

  # pass remaining prototypes, search declarations
  while (<IN>) {
    $newmain .= $_ unless /SGU: END PROTOTYPES/;
    last if /SGU: BEGIN SUITES DECLARATION/;
  }

  ### Done with prototypes. And now, the actual code
  
  # search my prototype
  while (<IN>) {
    last if  (/SGU: END SUITES DECLARATION/ || /SGU: BEGIN FILE $infile/);
    $newmain .= $_;
  }
  if (/SGU: BEGIN FILE $infile/) { # found an old section for this file. Kill it    
    while (<IN>) {
      last if /SGU: END FILE/;
    }
    $_ = <IN>; # pass extra blank line
    chomp;
    die "this line should be blank ($_). Did you edit the file?" if /\W/;
  }
  my ($old_)=($_);
  # add my section
  $newmain .= "    /* SGU: BEGIN FILE $infile */\n";
  $newmain .= "      suite = xbt_test_suite_by_name(\"$suite_name\",$suite_title);\n";
  map {
    my ($name,$func,$title) = @{$_};
    $newmain .=  "      xbt_test_suite_push(suite, $func, $title);\n";
  } @tests;

  $newmain .= "    /* SGU: END FILE */\n\n";
  if ($old_ =~ /SGU: BEGIN FILE/ || $old_ =~ /SGU: END SUITES DECLARATION/) {
    $newmain .= $old_;
  }

  # pass the remaining 
  while (<IN>) {
    $newmain .= $_;
  }
close IN || die "$progname: Cannot close main file 'simgrid_units_main.c': $!\n";

# write it back to main
open OUT,">simgrid_units_main.c" || die "$progname: Cannot open main file 'simgrid_units_main.c': $!\n";
print OUT $newmain;
close OUT || die "$progname: Cannot close main file 'simgrid_units_main.c': $!\n";

0;