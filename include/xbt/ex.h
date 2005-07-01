/*
**  OSSP ex - Exception Handling
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

#ifndef __EX_H__
#define __EX_H__

/* required ISO-C standard facilities */
#include <stdio.h>

/* convenience define */
#ifndef NULL
#define NULL (void *)0
#endif

/* determine how the current function name can be fetched */
#if (   defined(__STDC__) \
     && defined(__STDC_VERSION__) \
     && __STDC_VERSION__ >= 199901L)
#define __EX_FUNC__ __func__      /* ISO-C99 compliant */
#elif (   defined(__GNUC__) \
       && defined(__GNUC_MINOR__) \
       && (   __GNUC__ > 2 \
           || (__GNUC__ == 2 && __GNUC_MINOR__ >= 8)))
#define __EX_FUNC__ __FUNCTION__  /* gcc >= 2.8 */
#else
#define __EX_FUNC__ "#NA#"        /* not available */
#endif

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

/* declare the exception type (public) */
typedef struct {
    /* throw value */
    void       *ex_class;
    void       *ex_object;
    void       *ex_value;
    /* throw point */
    const char *ex_file;
    int         ex_line;
    const char *ex_func;
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
#define EX_CTX_INITIALIZER \
    { NULL, 0, 0, 0, 0, 0, 0, { NULL, NULL, NULL, NULL, 0, NULL } }
#define EX_CTX_INITIALIZE(ctx) \
    do { \
        (ctx)->ctx_mctx         = NULL; \
        (ctx)->ctx_deferred     = 0;    \
        (ctx)->ctx_deferring    = 0;    \
        (ctx)->ctx_defer        = 0;    \
        (ctx)->ctx_shielding    = 0;    \
        (ctx)->ctx_shield       = 0;    \
        (ctx)->ctx_caught       = 0;    \
        (ctx)->ctx_ex.ex_class  = NULL; \
        (ctx)->ctx_ex.ex_object = NULL; \
        (ctx)->ctx_ex.ex_value  = NULL; \
        (ctx)->ctx_ex.ex_file   = NULL; \
        (ctx)->ctx_ex.ex_line   = 0;    \
        (ctx)->ctx_ex.ex_func   = NULL; \
    } while (0)

/* the exception context */
typedef ex_ctx_t *(*ex_ctx_cb_t)(void);
extern ex_ctx_cb_t __ex_ctx;
extern ex_ctx_t *__ex_ctx_default(void);

/* the termination handler */
typedef void (*ex_term_cb_t)(ex_t *);
extern ex_term_cb_t __ex_terminate;
extern void __ex_terminate_default(ex_t *e);

/* the block for trying execution */
#define ex_try \
    { \
        ex_ctx_t *__ex_ctx_ptr = __ex_ctx(); \
        int __ex_cleanup = 0; \
        __ex_mctx_t *__ex_mctx_en; \
        __ex_mctx_t __ex_mctx_me; \
        __ex_mctx_en = __ex_ctx_ptr->ctx_mctx; \
        __ex_ctx_ptr->ctx_mctx = &__ex_mctx_me; \
        if (__ex_mctx_save(&__ex_mctx_me)) { \
            if (1)

/* the optional(!) block for cleanup */
#define ex_cleanup \
            else { \
            } \
            __ex_ctx_ptr->ctx_caught = 0; \
        } \
        else { \
            __ex_mctx_restored(&__ex_mctx_me); \
            __ex_ctx_ptr->ctx_caught = 1; \
        } \
        __ex_ctx_ptr->ctx_mctx = __ex_mctx_en; \
        __ex_cleanup = 1; \
        if (1) { \
            if (1)

/* the block for catching an exception */
#define ex_catch(e) \
            else { \
            } \
            if (!(__ex_cleanup)) \
                __ex_ctx_ptr->ctx_caught = 0; \
        } \
        else { \
            if (!(__ex_cleanup)) { \
                __ex_mctx_restored(&__ex_mctx_me); \
                __ex_ctx_ptr->ctx_caught = 1; \
            } \
        } \
        __ex_ctx_ptr->ctx_mctx = __ex_mctx_en; \
    } \
    if (   !(__ex_ctx()->ctx_caught) \
        || ((e) = __ex_ctx()->ctx_ex, 0)) { \
    } \
    else

/* the throwing of a new exception */
#define ex_throw(c,o,v) \
    ((   __ex_ctx()->ctx_shielding > 0 \
      || (__ex_ctx()->ctx_deferring > 0 && __ex_ctx()->ctx_deferred == 1)) ? 0 : \
     (__ex_ctx()->ctx_ex.ex_class  = (void *)(c), \
      __ex_ctx()->ctx_ex.ex_object = (void *)(o), \
      __ex_ctx()->ctx_ex.ex_value  = (void *)(v), \
      __ex_ctx()->ctx_ex.ex_file   = __FILE__, \
      __ex_ctx()->ctx_ex.ex_line   = __LINE__, \
      __ex_ctx()->ctx_ex.ex_func   = __EX_FUNC__, \
      __ex_ctx()->ctx_deferred     = 1, \
      (__ex_ctx()->ctx_deferring > 0 ? 0 : \
       (__ex_ctx()->ctx_mctx == NULL \
        ? (__ex_terminate((ex_t *)&(__ex_ctx()->ctx_ex)), -1) \
        : (__ex_mctx_restore(__ex_ctx()->ctx_mctx), 1) ))))

/* the re-throwing of an already caught exception */
#define ex_rethrow \
    ((   __ex_ctx()->ctx_shielding > 0 \
      || __ex_ctx()->ctx_deferring > 0) ? 0 : \
      (  __ex_ctx()->ctx_mctx == NULL \
       ? (__ex_terminate((ex_t *)&(__ex_ctx()->ctx_ex)), -1) \
       : (__ex_mctx_restore(__ex_ctx()->ctx_mctx), 1) ))

/* shield an operation from exception handling */
#define ex_shield \
    for (__ex_ctx()->ctx_shielding++, \
         __ex_ctx()->ctx_shield =  1; \
         __ex_ctx()->ctx_shield == 1; \
         __ex_ctx()->ctx_shield =  0, \
         __ex_ctx()->ctx_shielding--)

/* defer immediate exception handling */
#define ex_defer \
    for (((__ex_ctx()->ctx_deferring)++ == 0 ? __ex_ctx()->ctx_deferred = 0 : 0), \
         __ex_ctx()->ctx_defer =  1;  \
         __ex_ctx()->ctx_defer == 1;  \
         __ex_ctx()->ctx_defer =  0,  \
         ((--(__ex_ctx()->ctx_deferring) == 0 && __ex_ctx()->ctx_deferred == 1) ? ex_rethrow : 0))

/* exception handling tests */
#define ex_catching \
    (__ex_ctx()->ctx_mctx != NULL)
#define ex_shielding \
    (__ex_ctx()->ctx_shielding > 0)
#define ex_deferring \
    (__ex_ctx()->ctx_deferring > 0)

/* optional namespace mapping */
#if defined(__EX_NS_UCCXX__)
#define Try      ex_try
#define Cleanup  ex_cleanup
#define Catch    ex_catch
#define Throw    ex_throw
#define Rethrow  ex_rethrow
#define Shield   ex_shield
#define Defer    ex_defer
#elif defined(__EX_NS_CXX__) || (!defined(__cplusplus) && !defined(__EX_NS_CUSTOM__))
#define try      ex_try
#define cleanup  ex_cleanup
#define catch    ex_catch
#define throw    ex_throw
#define rethrow  ex_rethrow
#define shield   ex_shield
#define defer    ex_defer
#endif

#endif /* __EX_H__ */

