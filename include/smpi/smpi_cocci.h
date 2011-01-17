/* Copyright (c) 2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_COCCI_H
#define SMPI_COCCI_H

/* Macros used by coccinelle-generated code */

#define SMPI_INITIALIZE_GLOBAL(name,type)                       \
   NULL;                                                        \
void __attribute__((weak,constructor)) __preinit_##name(void) { \
   if(!name)                                                    \
      name = (type*)malloc(smpi_global_size() * sizeof(type));  \
}                                                               \
void __attribute__((weak,destructor)) __postfini_##name(void) { \
   free(name);                                                  \
   name = NULL;                                                 \
}

#define SMPI_INITIALIZE_AND_SET_GLOBAl(name,type,expr)          \
   NULL;                                                        \
void __attribute__((weak,constructor)) __preinit_##name(void) { \
   size_t size = smpi_global_size();                            \
   size_t i;                                                    \
   type value = expr;                                           \
   if(!name) {                                                  \
      name = (type*)malloc(size * sizeof(type));                \
      for(i = 0; i < size; i++) {                               \
         name[i] = value;                                       \
      }                                                         \
   }                                                            \
}                                                               \
void __attribute__((weak,destructor)) __postfini_##name(void) { \
   free(name);                                                  \
   name = NULL;                                                 \
}

#define SMPI_GLOBAL_VAR_LOCAL_ACCESS(name) name[__rank()]

/* This function stores the rank locally, so that a request in
   SIMIX is not created each time */
int __attribute__((weak)) __rank(void) {
   static __thread int rank = -1;

   if(rank < 0) {
      rank = smpi_global_rank();
   }
   return rank;
}

#endif
