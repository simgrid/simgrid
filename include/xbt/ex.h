/*
**  OSSP ex - Exception Handling (modified to fit into SimGrid)
**  Copyright (c) 2005 Martin Quinson.
**  Copyright (c) 2002-2004 Ralf S. Engelschall <rse@engelschall.com>
**  Copyright (c) 2002-2004 The OSSP Project <http://www.ossp.org/>
**  Copyright (c) 2002-2004 Cable & Wireless <http://www.cw.com/>
**
**  This file is part of OSSP ex, an exception handling library
**  which can be found at http://www.ossp.org/pkg/lib/ex/.
**
**  Permission to use, copy, modify, and distribute this software for
**  any purpose with or without fee is hereby granted, provided that
**  the above copyright notice and this permission notice appear in all
**  copies.
**
**  THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
**  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
**  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
**  IN NO EVENT SHALL THE AUTHORS AND COPYRIGHT HOLDERS AND THEIR
**  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
**  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
**  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
**  USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
**  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
**  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
**  OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
**  SUCH DAMAGE.
**
**  ex.h: exception handling (pre-processor part)
*/

#ifndef __XBT_EX_H__
#define __XBT_EX_H__

#include <xbt/misc.h>
#include <xbt/sysdep.h>

/* required ISO-C standard facilities */
#include <stdio.h>

/* the machine context */
#if defined(__EX_MCTX_MCSC__)
#include <ucontext.h>            /* POSIX.1 ucontext(3) */
#define __ex_mctx_struct         ucontext_t uc;
#define __ex_mctx_save(mctx)     (getcontext(&(mctx)->uc) == 0)
#define __ex_mctx_restored(mctx) /* noop */
#define __ex_mctx_restore(mctx)  (void)setcontext(&(mctx)->uc)

#elif defined(__EX_MCTX_SSJLJ__)
#include <setjmp.h>              /* POSIX.1 sigjmp_buf(3) */
#define __ex_mctx_struct         sigjmp_buf jb;
#define __ex_mctx_save(mctx)     (sigsetjmp((mctx)->jb, 1) == 0)
#define __ex_mctx_restored(mctx) /* noop */
#define __ex_mctx_restore(mctx)  (void)siglongjmp((mctx)->jb, 1)

#elif defined(__EX_MCTX_SJLJ__) || !defined(__EX_MCTX_CUSTOM__)
#include <setjmp.h>              /* ISO-C jmp_buf(3) */
#define __ex_mctx_struct         jmp_buf jb;
#define __ex_mctx_save(mctx)     (setjmp((mctx)->jb) == 0)
#define __ex_mctx_restored(mctx) /* noop */
#define __ex_mctx_restore(mctx)  (void)longjmp((mctx)->jb, 1)
#endif

/* declare the machine context type */
typedef struct { __ex_mctx_struct } __ex_mctx_t;
/** @addtogroup XBT_ex
 *
 * This module is a small ISO-C++ style exception handling library
 * for use in the ISO-C language. It allows you to use the paradigm 
 * of throwing and catching exceptions in order to reduce the amount
 * of error handling code without hindering program robustness.
 *               
 * This is achieved by directly transferring exceptional return codes
 * (and the program control flow) from the location where the exception
 * is raised (throw point) to the location where it is handled (catch
 * point) -- usually from a deeply nested sub-routine to a parent 
 * routine. All intermediate routines no longer have to make sure that 
 * the exceptional return codes from sub-routines are correctly passed 
 * back to the parent.
 *
 * These features are brought to you by a modified version of the libex 
 * library, one of the numerous masterpiece of Ralf S. Engelschall.
 *
 * @section Introduction
 * 
 * In SimGrid, exceptions is a triple <\a msg , \a category , \a value> 
 * where \a msg is a human-readable text describing the exceptional 
 * condition, \a code an integer describing what went wrong and \a value
 * providing a sort of sub-category. (this is different in the original libex).
 *
 * @section Basic usage
 *
 * \em xbt_try \b TRIED_BLOCK [\em xbt_cleanup \b CLEANUP_BLOCK] \em xbt_catch (variable) \b CATCH_BLOCK
 *
 * This is the primary syntactical construct provided. It is modeled after the
 * ISO-C++ try-catch clause and should sound familiar to most of you.
 *
 * Any exception thrown directly from the TRIED_BLOCK block or from called
 * subroutines is caught. Cleanups which must be done after this block
 * (whenever an exception arised or not) should be placed into the optionnal
 * CLEANUP_BLOCK. The code dealing with the exceptions when they arise should
 * be placed into the (mandatory) CATCH_BLOCK.
 *
 * 
 * In absence of exception, the control flow goes into the blocks TRIED_BLOCK
 * and CLEANUP_BLOCK (if present); The CATCH_BLOCK block is then ignored. 
 *
 * When an exception is thrown, the control flow goes through the following
 * blocks: TRIED_BLOCK (up to the statement throwing the exception),
 * CLEANUP_BLOCK (if any) and CATCH_BLOCK. The exception is stored in a
 * variable for inspection inside the CATCH_BLOCK. This variable must be
 * declared in the outter scope, but its value is only valid within the
 * CATCH_BLOCK block. 
 *
 * Some notes:
 *  - xbt_try, xbt_cleanup and xbt_catch cannot be used separately, they work
 *    only in combination and form a language clause as a whole.
 *  - In contrast to the syntax of other languages (such as C++ or Jave) there
 *    is only one xbt_catch block and not multiple ones (all exceptions are
 *    of the same C type xbt_t). 
 *  - the variable of xbt_catch can naturally be reused in subsequent 
 *    xbt_catch clauses.
 *  - it is possible to nest xbt_try clauses.
 *
 * The xbt_try block is a regular ISO-C language statement block, but it is not
 * allowed to jump into it via "goto" or longjmp(3) or out of it via "break",
 * "return", "goto" or longjmp(3) because there is some hidden setup and
 * cleanup that needs to be done regardless of whether an exception is
 * caught. Bypassing these steps will break the exception handling facility.
 *     
 * The xbt_cleanup and xbt_catch blocks are regular ISO-C language statement
 * blocks without any restrictions. You are even allowed to throw (and in the
 * xbt_catch block to re-throw) exceptions.
 *
 * There is one subtle detail you should remember about xbt_try blocks:
 * Variables used in the xbt_cleanup or xbt_catch clauses must be declared with
 * the storage class "volatile", otherwise they might contain outdated
 * information if an exception it thrown.
 *
 *
 * This is because you usually do not know which commands in the xbt_try
 * were already successful before the exception was thrown (logically speaking)
 * and because the underlying ISO-C setjmp(3) facility applies those
 * restrictions (technically speaking). As a matter of fact, value changes
 * between the xbt_try and the xbt_throw may be discarded if you forget the
 * "volatile" keyword. 
 * 
 * @section Advanced usage
 *
 * @subsection xbt_defer DEFERING_BLOCK
 *
 * This directive executes DEFERING_BLOCK while deferring the throwing of
 * exceptions, i.e., exceptions thrown within this block are remembered, but
 * the control flow still continues until the end of the block. At its end, the
 * first exception which occured within the block (if any) is rethrown (any
 * subsequent exceptions are ignored).
 *
 * DEFERING_BLOCK is a regular ISO-C language statement block, but it is not
 * allowed to jump into it via "goto" or longjmp(3) or out of it via "break",
 * "return", "goto" or longjmp(3). It is however allowed to nest xbt_defer
 * clauses.
 *
 * @subsection xbt_shield SHIELDED_BLOCK
 *
 * This directive executes SHIELDED_BLOCK while shielding it against the
 * throwing of exceptions, i.e., any exception thrown from this block or its
 * subroutines are silently ignored.
 *
 * SHIELDED_BLOCK is a regular ISO-C language statement block, but it is not
 * allowed to jump into it via "goto" or longjmp(3) or out of it via "break",
 * "return", "goto" or longjmp(3).  It is however allowed to nest xbt_shield
 * clauses.
 *
 * @subsection Retrieving the current execution condition
 *
 * \a xbt_catching, \a xbt_deferred and \a xbt_shielding return a boolean
 * indicating whether the current scope is within a TRYIED_BLOCK,
 * DEFERING_BLOCK and SHIELDED_BLOCK (respectively)
 *
 * \section PROGRAMMING PITFALLS
 *
 * Exception handling is a very elegant and efficient way of dealing with
 * exceptional situation. Nevertheless it requires additional discipline in
 * programming and there are a few pitfalls one must be aware of. Look the
 * following code which shows some pitfalls and contains many errors (assuming
 * a mallocex() function which throws an exception if malloc(3) fails):
 *
 \verbatim
// BAD EXAMPLE
xbt_try {
  char *cp1, *cp2, cp3;

  cp1 = mallocex(SMALLAMOUNT);
  globalcontext->first = cp1;
  cp2 = mallocex(TOOBIG);
  cp3 = mallocex(SMALLAMOUNT);
  strcpy(cp1, "foo");
  strcpy(cp2, "bar");
} xbt_cleanup {
  if (cp3 != NULL) free(cp3);
  if (cp2 != NULL) free(cp2);
  if (cp1 != NULL) free(cp1);
} xbt_catch(ex) {
  printf("cp3=%s", cp3);
  ex_rethrow;
}\endverbatim

 * This example raises a few issues:
 *  -# \b variable scope\n
 *     Variables which are used in the xbt_cleanup or xbt_catch clauses must be
 *     declared before the xbt_try clause, otherwise they only exist inside the
 *     xbt_try block. In the example above, cp1, cp2 and cp3 only exist in the
 *     xbt_try block and are invisible from the xbt_cleanup and xbt_catch
 *     blocks.
 *  -# \b variable initialization \n
 *     Variables which are used in the xbt_cleanup or xbt_catch clauses must
 *     be initialized before the point of the first possible xbt_throw is
 *     reached. In the example above, xbt_cleanup would have trouble using cp3
 *     if mallocex() throws a exception when allocating a TOOBIG buffer.
 *  -# \b volatile variable \n
 *     Variables which are used in the xbt_cleanup or xbt_catch clauses MUST BE
 *     DECLARED AS "volatile", otherwise they might contain outdated
 *     information when an exception is thrown. 
 *  -# \b clean before catch \n
 *     The xbt_cleanup clause is not only place before the xbt_catch clause in
 *     the source code, it also occures before in the control flow. So,
 *     resources being cleaned up cannot be used in the xbt_catch block. In the
 *     example, c3 gets freed before the printf placed in xbt_catch.
 *  -# \b variable uninitialization \n
 *     If resources are passed out of the scope of the
 *     xbt_try/xbt_cleanup/xbt_catch construct, they naturally shouldn't get
 *     cleaned up. The example above does free(3) cp1 in xbt_cleanup although
 *     its value was affected to globalcontext->first, invalidating this
 *     pointer.

 * The following is fixed version of the code (annotated with the pitfall items
 * for reference):
 \verbatim
// GOOD EXAMPLE
{ / *01* /
  char * volatile / *03* / cp1 = NULL / *02* /;
  char * volatile / *03* / cp2 = NULL / *02* /;
  char * volatile / *03* / cp3 = NULL / *02* /;
  try {
    cp1 = mallocex(SMALLAMOUNT);
    globalcontext->first = cp1;
    cp1 = NULL / *05 give away* /;
    cp2 = mallocex(TOOBIG);
    cp3 = mallocex(SMALLAMOUNT);
    strcpy(cp1, "foo");
    strcpy(cp2, "bar");
  }
  clean { / *04* /
    printf("cp3=%s", cp3 == NULL / *02* / ? "" : cp3);
    if (cp3 != NULL)
      free(cp3);
    if (cp2 != NULL)
      free(cp2);
    / *05 cp1 was given away * /
  }
  catch(ex) {
    / *05 global context untouched * /
    rethrow;
  }
}\endverbatim

 *
 * @{
 */

/** @brief Structure describing an exception */
typedef struct {
  char *msg;      /**< human readable message; to be freed */
  int   category; /**< category like HTTP (what went wrong) */
  int   value;    /**< like errno (why did it went wrong) */
  /* throw point */
  char *host;     /* NULL for localhost; hostname:port if remote */
  char *procname; 
  char *file;     /**< to be freed only for remote exceptions */
  int   line;     
  char *func;     /**< to be freed only for remote exceptions */
} ex_t;

/* declare the context type (private) */
typedef struct {
    __ex_mctx_t  *ctx_mctx;     /* permanent machine context of enclosing try/catch */
    int           ctx_deferred; /* permanent flag whether exception is deferred */
    int           ctx_deferring;/* permanent counter of exception deferring level */
    int           ctx_defer;    /* temporary flag for exception deferring macro */
    int           ctx_shielding;/* permanent counter of exception shielding level */
    int           ctx_shield;   /* temporary flag for exception shielding macro */
    int           ctx_caught;   /* temporary flag whether exception was caught */
    volatile ex_t ctx_ex;       /* temporary exception storage */
} ex_ctx_t;

/* the static and dynamic initializers for a context structure */
#define XBT_CTX_INITIALIZER \
    { NULL, 0, 0, 0, 0, 0, 0, { /* content */ NULL, 0, 0, \
                                /*throw point*/ NULL, NULL, NULL, 0, NULL } }
#define XBT_CTX_INITIALIZE(ctx) \
    do { \
        (ctx)->ctx_mctx        = NULL; \
        (ctx)->ctx_deferred    = 0;    \
        (ctx)->ctx_deferring   = 0;    \
        (ctx)->ctx_defer       = 0;    \
        (ctx)->ctx_shielding   = 0;    \
        (ctx)->ctx_shield      = 0;    \
        (ctx)->ctx_caught      = 0;    \
        (ctx)->ctx_ex.msg      = NULL; \
        (ctx)->ctx_ex.category = 0;    \
        (ctx)->ctx_ex.value    = 0;    \
        (ctx)->ctx_ex.host     = NULL; \
        (ctx)->ctx_ex.procname = NULL; \
        (ctx)->ctx_ex.file     = NULL; \
        (ctx)->ctx_ex.line     = 0;    \
        (ctx)->ctx_ex.func     = NULL; \
    } while (0)

/* the exception context */
typedef ex_ctx_t *(*ex_ctx_cb_t)(void);
extern ex_ctx_cb_t __xbt_ex_ctx;
extern ex_ctx_t *__xbt_ex_ctx_default(void);

/* the termination handler */
typedef void (*ex_term_cb_t)(ex_t *);
extern ex_term_cb_t __xbt_ex_terminate;
extern void __xbt_ex_terminate_default(ex_t *e);

/** @brief Introduce a block where exception may be dealed with 
 *  @hideinitializer
 */
#define xbt_try \
    { \
        ex_ctx_t *__xbt_ex_ctx_ptr = __xbt_ex_ctx(); \
        int __ex_cleanup = 0; \
        __ex_mctx_t *__ex_mctx_en; \
        __ex_mctx_t __ex_mctx_me; \
        __ex_mctx_en = __xbt_ex_ctx_ptr->ctx_mctx; \
        __xbt_ex_ctx_ptr->ctx_mctx = &__ex_mctx_me; \
        if (__ex_mctx_save(&__ex_mctx_me)) { \
            if (1)

/** @brief optional(!) block for cleanup 
 *  @hideinitializer
 */
#define xbt_cleanup \
            else { \
            } \
            __xbt_ex_ctx_ptr->ctx_caught = 0; \
        } \
        else { \
            __ex_mctx_restored(&__ex_mctx_me); \
            __xbt_ex_ctx_ptr->ctx_caught = 1; \
        } \
        __xbt_ex_ctx_ptr->ctx_mctx = __ex_mctx_en; \
        __ex_cleanup = 1; \
        if (1) { \
            if (1)

/** @brief the block for catching (ie, deal with) an exception 
 *  @hideinitializer
 */
#define xbt_catch(e) \
            else { \
            } \
            if (!(__ex_cleanup)) \
                __xbt_ex_ctx_ptr->ctx_caught = 0; \
        } \
        else { \
            if (!(__ex_cleanup)) { \
                __ex_mctx_restored(&__ex_mctx_me); \
                __xbt_ex_ctx_ptr->ctx_caught = 1; \
            } \
        } \
        __xbt_ex_ctx_ptr->ctx_mctx = __ex_mctx_en; \
    } \
    if (   !(__xbt_ex_ctx()->ctx_caught) \
        || ((e) = __xbt_ex_ctx()->ctx_ex, 0)) { \
    } \
    else

/** @brief Build an exception from the supplied arguments and throws it
 *  @hideinitializer
 *
 * If called from within a sg_try/sg_catch construct, this exception 
 * is copied into the sg_catch relevant variable program control flow 
 * is derouted to the sg_catch (after the optional sg_cleanup). 
 *
 * If no sg_try/sg_catch conctruct embeeds this call, the program calls
 * abort(3). 
 *
 * The sg_throw can be performed everywhere, including inside sg_try, 
 * sg_cleanup and sg_catch blocks.
 */
#define xbt_throw(c,v,m) \
    ((   __xbt_ex_ctx()->ctx_shielding > 0 \
      || (__xbt_ex_ctx()->ctx_deferring > 0 && __xbt_ex_ctx()->ctx_deferred == 1)) ? 0 : \
     (__xbt_ex_ctx()->ctx_ex.msg      = bprintf(m), \
      __xbt_ex_ctx()->ctx_ex.category = (c), \
      __xbt_ex_ctx()->ctx_ex.value    = (v), \
      __xbt_ex_ctx()->ctx_ex.host     = (char*)NULL, \
      __xbt_ex_ctx()->ctx_ex.procname = strdup(xbt_procname()), \
      __xbt_ex_ctx()->ctx_ex.file     = (char*)__FILE__, \
      __xbt_ex_ctx()->ctx_ex.line     = __LINE__, \
      __xbt_ex_ctx()->ctx_ex.func     = (char*)_XBT_FUNCTION, \
      __xbt_ex_ctx()->ctx_deferred     = 1, \
      (__xbt_ex_ctx()->ctx_deferring > 0 ? 0 : \
       (__xbt_ex_ctx()->ctx_mctx == NULL \
        ? (__xbt_ex_terminate((ex_t *)&(__xbt_ex_ctx()->ctx_ex)), -1) \
        : (__ex_mctx_restore(__xbt_ex_ctx()->ctx_mctx), 1) ))))

/** @brief re-throwing of an already caught exception (ie, pass it to the upper catch block) 
 *  @hideinitializer
 */
#define xbt_rethrow \
    ((   __xbt_ex_ctx()->ctx_shielding > 0 \
      || __xbt_ex_ctx()->ctx_deferring > 0) ? 0 : \
      (  __xbt_ex_ctx()->ctx_mctx == NULL \
       ? (__xbt_ex_terminate((ex_t *)&(__xbt_ex_ctx()->ctx_ex)), -1) \
       : (__ex_mctx_restore(__xbt_ex_ctx()->ctx_mctx), 1) ))

/** @brief shield an operation from exception handling 
 *  @hideinitializer
 */
#define xbt_shield \
    for (__xbt_ex_ctx()->ctx_shielding++, \
         __xbt_ex_ctx()->ctx_shield =  1; \
         __xbt_ex_ctx()->ctx_shield == 1; \
         __xbt_ex_ctx()->ctx_shield =  0, \
         __xbt_ex_ctx()->ctx_shielding--)

/** @brief defer immediate exception handling 
 *  @hideinitializer
 */
#define xbt_defer \
    for (((__xbt_ex_ctx()->ctx_deferring)++ == 0 ? __xbt_ex_ctx()->ctx_deferred = 0 : 0), \
         __xbt_ex_ctx()->ctx_defer =  1;  \
         __xbt_ex_ctx()->ctx_defer == 1;  \
         __xbt_ex_ctx()->ctx_defer =  0,  \
         ((--(__xbt_ex_ctx()->ctx_deferring) == 0 && __xbt_ex_ctx()->ctx_deferred == 1) ? xbt_rethrow : 0))

/** @brief exception handling tests 
 *  @hideinitializer
 */
#define xbt_catching \
    (__xbt_ex_ctx()->ctx_mctx != NULL)
/** @brief exception handling tests 
 *  @hideinitializer
 */
#define xbt_shielding \
    (__xbt_ex_ctx()->ctx_shielding > 0)
/** @brief exception handling tests 
 *  @hideinitializer
 */
#define xbt_deferring \
    (__xbt_ex_ctx()->ctx_deferring > 0)

/* optional namespace mapping */
#if defined(__EX_NS_UCCXX__)
#define Try      xbt_try
#define Cleanup  xbt_cleanup
#define Catch    xbt_catch
#define Throw    xbt_throw
#define Rethrow  xbt_rethrow
#define Shield   xbt_shield
#define Defer    xbt_defer
#elif defined(__EX_NS_CXX__) || (!defined(__cplusplus) && !defined(__EX_NS_CUSTOM__))
#define try      xbt_try
#define cleanup  xbt_cleanup
#define catch    xbt_catch
#define throw    xbt_throw
#define rethrow  xbt_rethrow
#define shield   xbt_shield
#define defer    xbt_defer
#endif

/** @} */
#endif /* __XBT_EX_H__ */
 
