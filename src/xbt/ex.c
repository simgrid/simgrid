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
**  ex.c: exception handling (compiler part)
*/

#include <stdio.h>
#include <stdlib.h>

#include "xbt/ex.h"

/* default __ex_ctx callback function */
ex_ctx_t *__ex_ctx_default(void)
{
    static ex_ctx_t ctx = EX_CTX_INITIALIZER;

    return &ctx;
}

/* default __ex_terminate callback function */
void __ex_terminate_default(ex_t *e)
{
    fprintf(stderr,
            "**EX: UNCAUGHT EXCEPTION: "
            "class=0x%lx object=0x%lx value=0x%lx [%s:%d@%s]\n",
            (long)((e)->ex_class), (long)((e)->ex_object), (long)((e)->ex_value),
            (e)->ex_file, (e)->ex_line, (e)->ex_func);
    abort();
}

/* the externally visible API */
ex_ctx_cb_t  __ex_ctx       = &__ex_ctx_default;
ex_term_cb_t __ex_terminate = &__ex_terminate_default;

