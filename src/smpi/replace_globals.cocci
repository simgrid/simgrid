// FIXME: problems
//   - does not want to match dynamic arrays...
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
expression value;
constant size;
@@
(
T@p 
- var
+ *var = SMPI_INITIALIZE_GLOBAL(var, T)
;
|
T@p
- var = value 
+ *var = SMPI_INITIALIZE_AND_SET_GLOBAL(var, T, value)
;
|
T@p
- var[]
+ *var[] = SMPI_INITIALIZE_GLOBAL_ARRAY(T, size)
;
|
T@p
- var[size]
+ *var[] = SMPI_INITIALIZE_GLOBAL_STATIC_ARRAY(T, size)
;
|
T@p
- var[size] = { ... }
+ *var[] = SMPI_INITIALIZE_AND_SET_GLOBAL_STATIC_ARRAY(T, size)
;
)

@rewritelocalaccess@
local idexpression lvar;
identifier globalvardecl.var;
@@
{
<...
(
lvar
|
+SMPI_GLOBAL_VAR_LOCAL_ACCESS(
var
+)
)
...>
}
