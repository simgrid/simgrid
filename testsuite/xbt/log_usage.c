// $Id$
// Copyright (c) 2001, Bit Farm, Inc. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. The name of the author may not be used to endorse or promote products
//    derived from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
// NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
/**
 *
 * Run it w/o arguments, and also try:
 *
 *    ./test_l4c Test.thresh=0
 *
 * which should print out the DEBUG message.
 *
 * 'make test' will run various test cases.
 *
 */

#include <gras.h>
#include <stdio.h>

GRAS_LOG_NEW_DEFAULT_SUBCATEGORY(Test, Top);
GRAS_LOG_NEW_CATEGORY(Top);

void parse_log_opt(int argc, char **argv,const char *deft);

int main(int argc, char **argv) {
  parse_log_opt(argc,argv,"root.thresh=debug log.thresh=debug");

  DEBUG1("val=%d", 1);
  WARNING1("val=%d", 2);
  CDEBUG2(Top, "val=%d%s", 3, "!");
  CRITICAL6("false alarm%s%s%s%s%s%s", "","","","","","!");
  
  gras_finalize();
  return 0;
}
