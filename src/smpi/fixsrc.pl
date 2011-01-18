#!/usr/bin/perl

# Add include for mandatory header file
print "#include <smpi_cocci.h>\n";

# FIXME: here we make the assumption that people don't do things like put
# multiple statements on the same line after a declaration, but separated by
# semicolons. It's a reasonable assumption for the time being, but technically
# it could cause problems for some code.

OUTER: while ($line = <STDIN>) {
    if ($line =~ /SMPI_VARINIT/) {
        do {
            chomp $line;         # kill carriage return
            $line =~ s/\s+/ /g;  # remove excessive whitespace added by spatch
            while ($line =~ s/(SMPI_VARINIT[A-Z0-9_]*?\(.*?\))//) {
                print "$1\n";
            } 

            # if varinit continues on to next line
            if ($line =~ /SMPI_VARINIT/) {
                # should only happen for bad code...
                if (!($nextline = <STDIN>)) {
                    last OUTER;
                }
                $line .= $nextline;

            }
        } while ($line =~ /SMPI_VARINIT/);
    } else {
        print $line;
    }
}
