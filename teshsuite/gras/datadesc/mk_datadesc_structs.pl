#! /usr/bin/perl

use strict;
use warnings;

my @types;
my @abrev;
my @val;

push @types,"char";          push @abrev, "c"; push @val, "'w'";
push @types,"short int";     push @abrev, "s"; push @val, '134';
push @types,"int";           push @abrev, "i"; push @val, '-11249';
push @types,"long int";      push @abrev, "l"; push @val, '31319919';
push @types,"long long int"; push @abrev, "L"; push @val, '-232130010';
push @types,"float";         push @abrev, "f"; push @val, '-11313.1135';
push @types,"double";        push @abrev, "d"; push @val, '1424420.11331';

die 'scalar @types != scalar @abrev' unless (scalar @types == scalar @abrev);

print "/* This file is perl-generated, of course */\n\n";

print "#include \"gras.h\"\n\n";

print "XBT_LOG_NEW_DEFAULT_SUBCATEGORY(structs,test,\"Logs about the gigantic struct test\");\n\n";

print "#define READ  0\n#define WRITE 1\n#define RW    2\n\n";
  
print "void write_read(const char *type,void *src, void *dst, xbt_socket_t *sock, int direction);\n\n";

my ($i,$j,$k,$l);
my $max=scalar @types;
# Those max variables can be used to tune how much test you are ready to do.
# But WARNING! changing them naturally invalidate any existing data file.
my $max_i=$max-1;
my $max_j=0;     
my $max_k=$max-1;
my $max_l=0;     
for $i (0..$max_i) { for $j (0..$max_j) { for $k (0..$max_k) { for $l (0..$max_l) {
    print "XBT_DEFINE_TYPE(".$abrev[$i].$abrev[$j].$abrev[$k].$abrev[$l].",".
      "struct ".$abrev[$i].$abrev[$j].$abrev[$k].$abrev[$l]." { ".
        $types[$i]." a; ".
        $types[$j]." b; ".
        $types[$k]." c;".
        $types[$l]." d;".
      "};)\n";
}}}}

# print "\n#define test(a) do {if (!(a)) { failed = 1; XBT_ERROR(\"%s failed\",#a);}} while (0)\n";
 print "\n#define test(a) xbt_assert(a)\n";

print "void register_structures(void);\n";
print "void register_structures(void) {\n";
for $i (0..$max_i) { for $j (0..$max_j) { for $k (0..$max_k) { for $l (0..$max_l) {
    my $struct=$abrev[$i].$abrev[$j].$abrev[$k].$abrev[$l];
    print "  gras_msgtype_declare(\"$struct\", xbt_datadesc_by_symbol($struct));\n";
}}}}
print "}\n";

print "void test_structures(xbt_socket_t *sock, int direction);\n";
print "void test_structures(xbt_socket_t *sock, int direction) {\n";
for $i (0..$max_i) { for $j (0..$max_j) { for $k (0..$max_k) { for $l (0..$max_l) {
    my $struct=$abrev[$i].$abrev[$j].$abrev[$k].$abrev[$l];
    print "  struct $struct my_$struct = {".$val[$i]."+(".$types[$i].")1,"
                                           .$val[$j]."+(".$types[$j].")2,"
                                           .$val[$k]."+(".$types[$k].")3,"
                                           .$val[$l]."+(".$types[$l].")4}, my_${struct}2;\n";
}}}}

print "  XBT_INFO(\"---- Test on all possible struct having 4 fields (".(($max_i+1)*($max_j+1)*($max_k+1)*($max_l+1))." structs) ----\");\n";
for $i (0..$max_i) { for $j (0..$max_j) { for $k (0..$max_k) { for $l (0..$max_l) {
    my $struct=$abrev[$i].$abrev[$j].$abrev[$k].$abrev[$l];
    print "  write_read(\"$struct\", &my_$struct, &my_${struct}2, sock,direction);\n";
    print "  if (direction == READ || direction == RW) {\n";
    print "     int failed = 0;\n";
    print "     test(my_$struct.a == my_${struct}2.a);\n";
    print "     test(my_$struct.b == my_${struct}2.b);\n";
    print "     test(my_$struct.c == my_${struct}2.c);\n";
    print "     test(my_$struct.d == my_${struct}2.d);\n";
    print "     if (!failed) XBT_VERB(\"Passed $struct\");\n";
    print "  }\n";
}}}}
    print "}\n";
    ;
