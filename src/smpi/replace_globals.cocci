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

// define a local variable declaration as one at some level of nesting
@localvardecl@
type T;
identifier var;
position p;
expression E;
@@
<...
( // default case
T@p
var
;
| // variable has initializer
T@p
var = E
;
)
...>

// define a global variable declaration as one that is neither a function
// prototype nor a local variable declaration
@globalvardecl@
type T;
identifier var;
position p != { funcproto.p, localvardecl.p };
expression value;
// expression size;
@@
( // default case
T@p 
- var
+ *var = SMPI_VARINIT_GLOBAL(var, T)
;
| // variable has initializer (not a struct or array)
T@p
- var = value 
+ *var = SMPI_VARINIT_GLOBAL_AND_SET(var, T, value)
;
//| // array of specified size
//T@p // FIXME: matches, but complains if more than one decl on a line...
//- var[size]
//+ *var[size] = SMPI_VARINIT_GLOBAL_ARRAY(T, size)
//;
//| // array of specified size with initializer
//T@p // FIXME: how to match initializer?
//- var[size] = { ... }
//+ *var[] = SMPI_VARINIT_GLOBAL_ARRAY_AND_SET(T, size, { ... })
//;
//| // array without specified size, but with initializer
//T@p // FIXME: how to match initializer? how to figure out size?
//- var[] = { ... }
//+ *var[] = SMPI_VARINIT_GLOBAL_ARRAY_AND_SET(T, size, { ... }) // size = ?
//;
//| struct with initializer?
)

// rewrite access to global variables based on name, but avoid the declaration
// and local variables that might have the same name
@rewriteglobalaccess@
type T;
local idexpression lvar;
identifier globalvardecl.var;
@@
<...
( // local variable
lvar
| // rewrite access
+SMPI_VARGET_GLOBAL(
var
+)
)
...>

// define a local static variable declaration as one at some level of nesting
// starting with the word static (exceptions?)
@staticvardecl@
type T;
identifier var;
expression value;
@@
<...
( // default case
static T
- var
+ *var = SMPI_VARINIT_STATIC(T, var)
;
| // variable has initializer (not a struct or array)
T
- var = value 
+ *var = SMPI_VARINIT_STATIC_AND_SET(var, T, value)
;
)
...>

// 
@rewritestaticaccess@
type T;
identifier staticvardecl.var;
@@
( // declaration
T
var
;
| // rewrite access
+SMPI_VARGET_STATIC(
var
+)
)
