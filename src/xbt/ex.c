/*
**  OSSP ex - Exception Handling (modified to fit into SimGrid)
**  Copyright (c) 2005 Martin Quinson
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
ex_ctx_t *__xbt_ex_ctx_default(void) {
    static ex_ctx_t ctx = SG_CTX_INITIALIZER;

    return &ctx;
}

/* default __ex_terminate callback function */
void __xbt_ex_terminate_default(ex_t *e) {

    fprintf(stderr,
            "** SimGrid: UNCAUGHT EXCEPTION:\n"
	    "** (%d/%d) %s\n"
            "** Thrown by %s%s%s at %s:%d:%s\n",
	    e->code, e->value, e->msg,
	    e->procname, (e->host?"@":""),(e->host?e->host:""),
	    e->file,e->line,e->func);
    abort();
}

/* the externally visible API */
ex_ctx_cb_t  __xbt_ex_ctx       = &__xbt_ex_ctx_default;
ex_term_cb_t __xbt_ex_terminate = &__xbt_ex_terminate_default;

