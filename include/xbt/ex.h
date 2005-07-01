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

#ifndef __SG_EX_H__
#define __SG_EX_H__

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

/** Content of an exception */
typedef struct {
  char *msg;      /*< human readable message; to be freed */
  int   code;     /*< category like HTTP (what went wrong) */
  int   value;    /*< like errno (why did it went wrong) */
  /* throw point */
  char *host;     /*< NULL for localhost; hostname:port if remote */
  char *procname; 
  char *file;     /*< to be freed only for remote exceptions */
  int   line;
  char *func;     /*< to be freed only for remote exceptions */
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
#define SG_CTX_INITIALIZER \
    { NULL, 0, 0, 0, 0, 0, 0, { /* content */ NULL, 0, 0, \
                                /*throw point*/ NULL, NULL, NULL, 0, NULL } }
#define SG_CTX_INITIALIZE(ctx) \
    do { \
        (ctx)->ctx_mctx        = NULL; \
        (ctx)->ctx_deferred    = 0;    \
        (ctx)->ctx_deferring   = 0;    \
        (ctx)->ctx_defer       = 0;    \
        (ctx)->ctx_shielding   = 0;    \
        (ctx)->ctx_shield      = 0;    \
        (ctx)->ctx_caught      = 0;    \
        (ctx)->ctx_ex.msg      = NULL; \
        (ctx)->ctx_ex.code     = 0;    \
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

/* the block for trying execution */
#define sg_try \
    { \
        ex_ctx_t *__xbt_ex_ctx_ptr = __xbt_ex_ctx(); \
        int __ex_cleanup = 0; \
        __ex_mctx_t *__ex_mctx_en; \
        __ex_mctx_t __ex_mctx_me; \
        __ex_mctx_en = __xbt_ex_ctx_ptr->ctx_mctx; \
        __xbt_ex_ctx_ptr->ctx_mctx = &__ex_mctx_me; \
        if (__ex_mctx_save(&__ex_mctx_me)) { \
            if (1)

/* the optional(!) block for cleanup */
#define sg_cleanup \
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

/* the block for catching an exception */
#define sg_catch(e) \
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

/* the throwing of a new exception */
#define sg_throw(c,v,m) \
    ((   __xbt_ex_ctx()->ctx_shielding > 0 \
      || (__xbt_ex_ctx()->ctx_deferring > 0 && __xbt_ex_ctx()->ctx_deferred == 1)) ? 0 : \
     (__xbt_ex_ctx()->ctx_ex.msg      = bprintf(m), \
      __xbt_ex_ctx()->ctx_ex.code     = (c), \
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

/* the re-throwing of an already caught exception */
#define sg_rethrow \
    ((   __xbt_ex_ctx()->ctx_shielding > 0 \
      || __xbt_ex_ctx()->ctx_deferring > 0) ? 0 : \
      (  __xbt_ex_ctx()->ctx_mctx == NULL \
       ? (__xbt_ex_terminate((ex_t *)&(__xbt_ex_ctx()->ctx_ex)), -1) \
       : (__ex_mctx_restore(__xbt_ex_ctx()->ctx_mctx), 1) ))

/* shield an operation from exception handling */
#define sg_shield \
    for (__xbt_ex_ctx()->ctx_shielding++, \
         __xbt_ex_ctx()->ctx_shield =  1; \
         __xbt_ex_ctx()->ctx_shield == 1; \
         __xbt_ex_ctx()->ctx_shield =  0, \
         __xbt_ex_ctx()->ctx_shielding--)

/* defer immediate exception handling */
#define sg_defer \
    for (((__xbt_ex_ctx()->ctx_deferring)++ == 0 ? __xbt_ex_ctx()->ctx_deferred = 0 : 0), \
         __xbt_ex_ctx()->ctx_defer =  1;  \
         __xbt_ex_ctx()->ctx_defer == 1;  \
         __xbt_ex_ctx()->ctx_defer =  0,  \
         ((--(__xbt_ex_ctx()->ctx_deferring) == 0 && __xbt_ex_ctx()->ctx_deferred == 1) ? sg_rethrow : 0))

/* exception handling tests */
#define sg_catching \
    (__xbt_ex_ctx()->ctx_mctx != NULL)
#define sg_shielding \
    (__xbt_ex_ctx()->ctx_shielding > 0)
#define sg_deferring \
    (__xbt_ex_ctx()->ctx_deferring > 0)

/* optional namespace mapping */
#if defined(__EX_NS_UCCXX__)
#define Try      sg_try
#define Cleanup  sg_cleanup
#define Catch    sg_catch
#define Throw    sg_throw
#define Rethrow  sg_rethrow
#define Shield   sg_shield
#define Defer    sg_defer
#elif defined(__EX_NS_CXX__) || (!defined(__cplusplus) && !defined(__EX_NS_CUSTOM__))
#define try      sg_try
#define cleanup  sg_cleanup
#define catch    sg_catch
#define throw    sg_throw
#define rethrow  sg_rethrow
#define shield   sg_shield
#define defer    sg_defer
#endif

#endif /* __SG_EX_H__ */
 
