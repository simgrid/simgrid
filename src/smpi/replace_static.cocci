// FIXME: why can't I just define a static local vardecl the same way as a
// normal local vardecl?

// Function prototype looks like variable dec, but has parentheses
@funcproto@
type T;
identifier func;
position p;
@@
T@p func(...);

// Define a local variable as one whose declaration is encased in brackets
@localvardecl@
type T;
identifier var;
position p;
expression E;
@@
{
<...
(
T@p
var
;
|
T@p
var = E
;
)
...>
}

// global variable is one whose declaration is neither local nor a function
// prototype
@globalvardecl@
type T;
identifier var;
position p != { localvardecl.p, funcproto.p };
expression value;
// expression size;
@@
(
T@p var;
|
T@p var = value;
)

// local static decl is a nonglobal static decl...
@localstaticvardecl@
type T;
identifier var;
position p != globalvardecl.p;
expression value;
@@
(
static T@p
- var
+ *var = SMPI_VARINIT_STATIC(var, T)
;
|
static T@p
- var = value
+ *var = SMPI_VARINIT_STATIC_AND_SET(var, T, value)
;
)

// FIXME: add varaccess...
