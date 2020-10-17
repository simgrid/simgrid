#! /usr/bin/env perl
use strict;

while (<>) {
    # Skip the line " * Generated 2018/10/15 12:32:06." (Reproducible Builds)
    next if (m#^ \* Generated [0-9/]* [0-9:]*#);

    # Informative error message for files using a very old DTD
    s#"Bad declaration %s."#"Bad declaration %s.\\nIf you are using an XML v3 file (check the version attribute in <platform>), please update it with tools/simgrid_update_xml.pl"#;

    # Accept the alternative DTD location
    if (/DOCTYPE.*simgrid.org.simgrid.dtd/)  {
	print ' "<!DOCTYPE"{S}"platform"{S}SYSTEM{S}("\'http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\'"|"\\"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\\""){s}">" SET(ROOT_surfxml_platform);'."\n";
    }

    # Completely rewrite the error handling mechanism to use exceptions instead of printing to stderr
    if (/fprintf.stderr, .*? flexml_err_msg.;/) {
	print('    surf_parse_error(flexml_err_msg);'."\n");
	next;
    }	
    
    # Actually outputs the resulting line
    print;
}
