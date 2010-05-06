#! /usr/bin/perl

use strict;
use Fcntl ':flock';

open SELF, "< $0" or die "Cannot open the lock file";
if (!flock SELF, LOCK_EX | LOCK_NB) {
    print STDERR "sg_unit_extractor already running. Cancelling...\n";
    exit;
}

my $progname="sg_unit_extractor";
# Get the args 
die "USAGE: $progname infile [infile+]\n"
  if (scalar @ARGV == 0);

map {process_one($_)} @ARGV;

sub process_one($) {
    
    my $infile = shift;
    my $outfile;
    
    $outfile =  $infile;
    $outfile =~ s/\.c$/_unit.c/;
    $outfile =~ s|.*/([^/]*)$|$1| if $outfile =~ m|/|;
    
    
    # Get the unit data
    my ($unit_source,$suite_name,$suite_title)=("","","");
    my (%tests); # to detect multiple definition
    my (@tests); # actual content
    
    open IN, "$infile" || die "$progname: Cannot open input file '$infile': $!\n";
    
    my $takeit=0;
    my $line=0;
    my $beginline=0;
    while (<IN>) {
	$line++;
	if (m/ifdef +SIMGRID_TEST/) {
	    $beginline = $line;
	    $takeit = 1;
	    next;
	}
	if (m/endif.*SIMGRID_TEST/) {
	    $takeit = 0;
	    next
	}
	
	if (m/XBT_TEST_SUITE\(\w*"([^"]*)"\w*, *(.*?)\);/) { #" {
	    die "$progname: Multiple suites in the same file ($infile) are not supported yet\n" if length($suite_name);
	    ($suite_name,$suite_title)=($1,$2);
	    die "$progname: Empty suite name in $infile" unless length($suite_name);
	    die "$progname: Empty suite title in $infile" unless length($suite_title);
	    next;
        } elsif (m/XBT_TEST_SUITE/) {
	    die "$progname: Parse error: This line seem to be a test suite declaration, but failed to parse it\n$_\n";
	}

        if (m/XBT_TEST_UNIT\(\w*"([^"]*)"\w*,([^,]*),(.*?)\)/) { #"{
	    die "$progname: multiply defined unit in file $infile: $1\n" if (defined($tests{$1}));
            
	    my @t=($1,$2,$3);
	    push @tests,\@t;
	    $tests{$1} = 1;
	} elsif (m/XBT_TEST_UNIT/) {
	    die "$progname: Parse error: This line seem to be a test unit, but failed to parse it\n$_\n";
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

    die "$progname: no suite defined in $infile\n" unless (length($suite_name));
  
    # Write the test

    my ($GENERATED)=("/*******************************/\n".
	             "/* GENERATED FILE, DO NOT EDIT */\n".
                     "/*******************************/\n\n");
    $beginline+=2;
    open OUT,">$outfile" || die "$progname: Cannot open output file '$outfile': $!\n";
    print OUT $GENERATED;
    print OUT "#include <stdio.h>\n";
    print OUT "#include \"xbt.h\"\n";
    print OUT $GENERATED;
    print OUT "# $beginline \"$infile\" \n";
    print OUT "$unit_source";
    print OUT $GENERATED;
    close OUT || die "$progname: Cannot close output file '$outfile': $!\n";

    # write the main skeleton if needed
    if (! -e "simgrid_units_main.c") {
	open OUT,">simgrid_units_main.c" || die "$progname: Cannot open main file 'simgrid_units_main.c': $!\n";
	print OUT $GENERATED;
	print OUT "#include <stdio.h>\n\n";
	print OUT "#include \"xbt.h\"\n\n";
	print OUT "extern xbt_test_unit_t _xbt_current_unit;\n\n";
	print OUT "/* SGU: BEGIN PROTOTYPES */\n";
	print OUT "/* SGU: END PROTOTYPES */\n\n";
	print OUT $GENERATED;
	#  print OUT "# 93 \"sg_unit_extractor.pl\"\n";
	print OUT <<EOF;
int main(int argc, char *argv[]) {
  xbt_test_suite_t suite; 
  char selection[1024];
  int i;\n
  int res;\n
  /* SGU: BEGIN SUITES DECLARATION */
  /* SGU: END SUITES DECLARATION */
      
  xbt_init(&argc,argv);
    
  /* Search for the tests to do */
    selection[0]='\\0';
    for (i=1;i<argc;i++) {
      if (!strncmp(argv[i],\"--tests=\",strlen(\"--tests=\"))) {
        char *p=strchr(argv[i],'=')+1;
        if (selection[0] == '\\0') {
          strcpy(selection, p);
        } else {
          strcat(selection, \",\");
          strcat(selection, p);
        }
      } else if (!strncmp(argv[i],\"--dump-only\",strlen(\"--dump-only\"))||
 	         !strncmp(argv[i],\"--dump\",     strlen(\"--dump\"))) {
        xbt_test_dump(selection);
        return 0;
      } else if (!strncmp(argv[i],\"--help\",strlen(\"--help\"))) {
	  printf(
	      "Usage: testall [--help] [--tests=selection] [--dump-only]\\n\\n"
	      "--help: display this help\\n"
	      "--dump-only: don't run the tests, but display some debuging info about the tests\\n"
	      "--tests=selection: Use argument to select which suites/units/tests to run\\n"
	      "                   --tests can be used more than once, and selection may be a comma\\n"
	      "                   separated list of directives.\\n\\n"
	      "Directives are of the form:\\n"
	      "   [-]suitename[:unitname]\\n\\n"
	      "If the first char is a '-', the directive disables its argument instead of enabling it\\n"
	      "suitename/unitname is the set of tests to en/disable. If a unitname is not specified,\\n"
	      "it applies on any unit.\\n\\n"
	      "By default, everything is enabled.\\n\\n"
	      "'all' as suite name apply to all suites.\\n\\n"
	      "Example 1: \\"-toto,+toto:tutu\\"\\n"
	      "  disables the whole toto testsuite (any unit in it),\\n"
	      "  then reenables the tutu unit of the toto test suite.\\n\\n"
	      "Example 2: \\"-all,+toto\\"\\n"
	      "  Run nothing but the toto suite.\\n");
	  return 0;
      } else {
        printf("testall: Unknown option: %s\\n",argv[i]);
        return 1;
      }
    }
  /* Got all my tests to do */
      
  res = xbt_test_run(selection);
  xbt_test_exit();
  return res;
}
EOF
	print OUT $GENERATED;
	close OUT || die "$progname: Cannot close main file 'simgrid_units_main.c': $!\n";
    }

   print "  Suite $suite_name: $suite_title (".(scalar @tests)." tests)\n";
   map {
       my ($name,$func,$title) = @{$_};
       print "    unit $name: func=$func; title=$title\n";
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
	   $newmain .=  "    void $func(void);\n"
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
	   $newmain .=  "      xbt_test_suite_push(suite, \"$name\", $func, $title);\n";
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
} # end if process_one($)

0;
