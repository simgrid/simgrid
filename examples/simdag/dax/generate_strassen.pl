#! /usr/bin/perl

use strict;

sub job {
    print "<job id=\"$_[0]\" namespace=\"$_[1]\" name=\"$_[2]\" version=\"1.0\" runtime=\"$_[3]\">\n";
}
sub file {
    print "  <uses file=\"$_[0]\" link=\"$_[1]\" register=\"true\" transfer=\"true\" optional=\"false\" type=\"data\" size=\"$_[2]\"/>\n";
}
sub strassen {
    my $matSize=shift;
    my $max_level=shift||1;
    my $level=shift||1;
    my $A = shift||"A";
    my $B = shift||"B";

    if ($level<$max_level) {
      die "Recursive Strassen don't work yet\n";
    }
    
    my $sizeAdd=$matSize*$matSize/4;
    my $sizeMult=$matSize*$matSize*$matSize/8;
    my $sizeBlock=$sizeAdd;

    # T1 = A11 + A22
    job("T1","Strassen","l$level",$sizeAdd);
    file("${A}11l$level","input",$sizeBlock);
    file("${A}22l$level","input",$sizeBlock);
    file("T1l$level","output",$sizeBlock);
    print("</job>\n");

    # T2 = A21 + A22
    job("T2","Strassen","l$level",$sizeAdd);
    file("${A}21l$level","input",$sizeBlock);
    file("${A}22l$level","input",$sizeBlock);
    file("T2l$level","output",$sizeBlock);
    print("</job>\n");

    # T3 = A11 + A12
    job("T3","Strassen","l$level",$sizeAdd);
    file("${A}11l$level","input",$sizeBlock);
    file("${A}12l$level","input",$sizeBlock);
    file("T3l$level","output",$sizeBlock);
    print("</job>\n");

    # T4 = A21 − A11
    job("T4","Strassen","l$level",$sizeAdd);
    file("${A}21l$level","input",$sizeBlock);
    file("${A}11l$level","input",$sizeBlock);
    file("T4l$level","output",$sizeBlock);
    print("</job>\n");

    # T5 = A12 − A22
    job("T5","Strassen","l$level",$sizeAdd);
    file("${A}12l$level","input",$sizeBlock);
    file("${A}22l$level","input",$sizeBlock);
    file("T5l$level","output",$sizeBlock);
    print("</job>\n");
    
    # T6 = B11 + B22
    job("T6","Strassen","l$level",$sizeAdd);
    file("${B}11l$level","input",$sizeBlock);
    file("${B}22l$level","input",$sizeBlock);
    file("T6l$level","output",$sizeBlock);
    print("</job>\n");
    
    # T7 = B12 − B22
    job("T7","Strassen","l$level",$sizeAdd);
    file("${B}12l$level","input",$sizeBlock);
    file("${B}22l$level","input",$sizeBlock);
    file("T7l$level","output",$sizeBlock);
    print("</job>\n");
    
    # T8 = B21 − B11
    job("T8","Strassen","l$level",$sizeAdd);
    file("${B}21l$level","input",$sizeBlock);
    file("${B}11l$level","input",$sizeBlock);
    file("T8l$level","output",$sizeBlock);
    print("</job>\n");
         
    # T9 = B11 + B12
    job("T9","Strassen","l$level",$sizeAdd);
    file("${B}11l$level","input",$sizeBlock);
    file("${B}12l$level","input",$sizeBlock);
    file("T9l$level","output",$sizeBlock);
    print("</job>\n");
    
    # T10 = B21 + B22
    job("T10","Strassen","l$level",$sizeAdd);
    file("${B}21l$level","input",$sizeBlock);
    file("${B}22l$level","input",$sizeBlock);
    file("T10l$level","output",$sizeBlock);
    print("</job>\n");

    # Q1 = T1 × T6
    if ($level<$max_level) {
	strassen($sizeBlock,$max_level,$level+1,"T1_","T6_");
    } else {
	job("Q1","Strassen","l$level",$sizeMult);
	file("T1l$level","input",$sizeBlock);
	file("T6l$level","input",$sizeBlock);
	file("Q1l$level","output",$sizeBlock);
	print("</job>\n");
    }

    # Q2 = T2 × B11
    if ($level<$max_level) {
	strassen($sizeBlock,$max_level,$level+1,"T2_","B11_");
    } else {
	job("Q2","Strassen","l$level",$sizeMult);
	file("T2l$level","input",$sizeBlock);
	file("${B}11l$level","input",$sizeBlock);
	file("Q2l$level","output",$sizeBlock);
	print("</job>\n");
    }

    # Q3 = A11 × T7         
    if ($level<$max_level) {
	strassen($sizeBlock,$max_level,$level+1,"A11_","T7_");
    } else {
	job("Q3","Strassen","l$level",$sizeMult);
	file("${A}11l$level","input",$sizeBlock);
	file("T7l$level","input",$sizeBlock);
	file("Q3l$level","output",$sizeBlock);
	print("</job>\n");
    }
    
    # Q4 = A22 × T8
    if ($level<$max_level) {
	strassen($sizeBlock,$max_level,$level+1,"A22_","T8_");
    } else {
	job("Q4","Strassen","l$level",$sizeMult);
	file("${A}22l$level","input",$sizeBlock);
	file("T8l$level","input",$sizeBlock);
	file("Q4l$level","output",$sizeBlock);
	print("</job>\n");
    }
    
    # Q5 = T3 × B22
    if ($level<$max_level) {
	strassen($sizeBlock,$max_level,$level+1,"T3_","B22_");
    } else {
	job("Q5","Strassen","l$level",$sizeMult);
	file("T3l$level","input",$sizeBlock);
	file("${B}22l$level","input",$sizeBlock);
	file("Q5l$level","output",$sizeBlock);
	print("</job>\n");
    }
    
    # Q6 = T4 × T9
    if ($level<$max_level) {
	strassen($sizeBlock,$max_level,$level+1,"T4_","T9_");
    } else {
	job("Q6","Strassen","l$level",$sizeMult);
	file("T4l$level","input",$sizeBlock);
	file("T9l$level","input",$sizeBlock);
	file("Q6l$level","output",$sizeBlock);
	print("</job>\n");
    }
    
    # Q7 = T5 × T10
    if ($level<$max_level) {
	strassen($sizeBlock,$max_level,$level+1,"T5_","T10_");
    } else {
	job("Q7","Strassen","l$level",$sizeMult);
	file("T5l$level","input",$sizeBlock);
	file("T10l$level","input",$sizeBlock);
	file("Q7l$level","output",$sizeBlock);
	print("</job>\n");
    }
    
    # C11 = Q1 + Q4 − Q5 + Q7
    job("Q11","Strassen","l$level",$sizeAdd*3);
    file("Q1l$level","input",$sizeBlock);
    file("Q4l$level","input",$sizeBlock);
    file("Q5l$level","input",$sizeBlock);
    file("Q7l$level","input",$sizeBlock);
    file("C11l$level","output",$sizeBlock);
    print("</job>\n");

    # C12 = Q3 + Q5
    job("C12","Strassen","l$level",$sizeAdd);
    file("Q3l$level","input",$sizeBlock);
    file("Q5l$level","input",$sizeBlock);
    file("C12l$level","output",$sizeBlock);
    print("</job>\n");
    
    # C21 = Q2 + Q4
    job("C21","Strassen","l$level",$sizeAdd);
    file("Q2l$level","input",$sizeBlock);
    file("Q4l$level","input",$sizeBlock);
    file("C21l$level","output",$sizeBlock);
    print("</job>\n");

    # C22 = Q1 − Q2 + Q3 + Q6
    job("C22","Strassen","l$level",$sizeMult);
    file("Q1l$level","input",$sizeBlock);
    file("Q2l$level","input",$sizeBlock);
    file("Q3l$level","input",$sizeBlock);
    file("Q6l$level","input",$sizeBlock);
    file("C22l$level","output",$sizeBlock);
    print("</job>\n");          
}

print "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
print "<!-- generated: ".(localtime())." -->\n";
print "<adag xmlns=\"http://pegasus.isi.edu/schema/DAX\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://pegasus.isi.edu/schema/DAX http://pegasus.isi.edu/schema/dax-2.1.xsd\" version=\"2.1\" count=\"1\" index=\"0\" name=\"test\" jobCount=\"25\" fileCount=\"0\" childCount=\"20\">\n";

strassen(2000);

print "</adag>\n";

