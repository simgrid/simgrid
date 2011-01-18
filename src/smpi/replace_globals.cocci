// FIXME: seems like cocci has problems manipulating the declarations, at least
// when there is more than one on the same line. We already need perl to split
// up the declarations after the fact, is there another tool we can use to patch
// up and match the declarations? In that case we could consider dropping cocci,
// or just using it to alter global variable accesses.
//
// FIXME: problems
//   - array declarations not properly matched...can fix, but then can't have
//   multiple declarations on one line
//   - does not match array initializers
//   - probably won't fix structure declarations with initialization either

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
expression size;
@@
(
T@p 
- var
+ *var = SMPI_VARINIT_GLOBAL(var, T)
;
|
T@p
- var = value 
+ *var = SMPI_VARINIT_GLOBAL_AND_SET(var, T, value)
;
//|
//T@p // FIXME: matches, but complains if more than one decl on a line...
//- var[size]
//+ *var[size] = SMPI_VARINIT_GLOBAL_ARRAY(T, size)
//;
//|
//T@p // FIXME: how to match initializer?
//- var[size] = { ... }
//+ *var[] = SMPI_VARINIT_GLOBAL_ARRAY_AND_SET(T, size, { ... })
//;
//|
//T@p // FIXME: how to match initializer? how to figure out size?
//- var[] = { ... }
//+ *var[] = SMPI_VARINIT_GLOBAL_ARRAY_AND_SET(T, size, { ... }) // size = ?
//;
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
+SMPI_VARGET_GLOBAL(
var
+)
)
...>
}
