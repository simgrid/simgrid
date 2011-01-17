// Function prototype looks like variable dec, but has parentheses
@funcproto@
type T;
identifier f;
position p;
@@
T f@p(...);

// Define a local variable as one whose declaration is encased in brackets
@localvardecl@
type T;
identifier a;
position p;
expression E;
@@
{
<...
(
 T a@p;
|
 T a@p = E;
)
...>
}

// global variable is one whose declaration is neither local nor a function
// prototype
@globalvardecl@
type T;
identifier b;
position p != { localvardecl.p, funcproto.p };
expression E;
@@
(
-T 
+T*
b@p
+ = SMPI_INITIALIZE_GLOBAL(T)
;
|
-T 
+T*
b@p = 
+ SMPI_INITIALIZE_AND_SET_GLOBAL(T,
E
+)
;
)

@rewritelocalaccess@
local idexpression x;
identifier globalvardecl.b;
@@
(
x
|
+SMPI_GLOBAL_VAR_LOCAL_ACCESS(
b
+)
)
