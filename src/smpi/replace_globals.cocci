// FIXME: problems
//   - does not match array initializers

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
expression E;
@@
(
T@p 
- var
+ *var = SMPI_INITIALIZE_GLOBAL(T)
;
|
T@p
- var = E
+ *var = SMPI_INITIALIZE_AND_SET_GLOBAL(T, E)
;
)

@rewritelocalaccess@
local idexpression x;
identifier globalvardecl.var;
@@
{
<...
(
x
|
+SMPI_GLOBAL_VAR_LOCAL_ACCESS(
var
+)
)
...>
}
