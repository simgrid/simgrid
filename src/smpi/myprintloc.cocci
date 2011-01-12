
@r@
/* Matching a declaration, ie rewriting site candidate 
   Disqualify the candidate
   ----------------- */
type T;
position p1;
identifier id;
@@

T id@p1;




@funcdecl@
# Matching a /function/ declaration. 
# Disqualify the candidate
#-----------------
type T;
position r.p1;
identifier id;
@@
 T id@p1(...) {...}




@funcproto@
# Matching a function prototype w/o body. 
# Disqualify the candidate
#-----------------
type T;
position r.p1;
identifier id;
@@
 T id@p1(...);

@localdeclaration@
# Matching a declaration at the top level of a function. 
# Disqualify the candidate
#-----------------
type T1,T2;
position r.p1;
identifier id1,id2;
@@

T1 id1(...) {
  ...
  T2 id2@p1;
  ...
}

@localinnerdeclaration@
# The next rule is there to make sure that we are not speaking of a
# local variable declared in an inner block. I don't like it at all:
# It is redundent and gives false negative on the foreach macros that
# get often declared in the code. Example in examples/gras/spawn.c:
# 
# int server() {
#   ...
#   xbt_dynar_foreach(chunk->primes,cursor,data) {
#     char number[100];
#     ...
#   }
#   ...
# } 
#
# Do I really need to complicate this rule further to add every macro
# that we use in our C, or can it be merged with the previous one by
# saying something like "in a function, at whatever level of nesting"?

type T1,T2;
position r.p1;
identifier id1,id2;
expression e1,e2,e3;
@@

T1 id1(...) {
  ...
( 
  for (e1;e2;e3) { ... T2 id2@p1; ... }
| 
  for (;e2;e3) { ... T2 id2@p1; ... }
| 
  for (e1;;e3) { ... T2 id2@p1; ... }
| 
  for (e1;e2;) { ... T2 id2@p1; ... }
| 
  for (e1;;) { ... T2 id2@p1; ... }
| 
  for (;e2;) { ... T2 id2@p1; ... }
| 
  for (;;e3) { ... T2 id2@p1; ... }
| 
  for (;;) { ... T2 id2@p1; ... }
| 
  while (e1) { ... T2 id2@p1; ... }
| 
  do { ... T2 id2@p1; ... } while (e1);
)
  ...
}



@script:python depends on r 
                          && !funcdecl 
			  && !funcproto 
			  && !localdeclaration 
			  && !localinnerdeclaration@

# This rule is only a debugging rule, to print the sites where the
# change must be applied

p1 << r.p1;
T  << r.T;
id << r.id;
@@

c1 = cocci.combine(id,p1)
print "1. symbol %s of type \"%s\" at %s:%s" % (id,T,c1.location.line,c1.location.column)
