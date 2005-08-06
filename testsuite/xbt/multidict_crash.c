/* $Id$ */

/* multidict_crash - A crash test for multi-level dictionnaries             */

/* Copyright (c) 2003-2005 Martin Quinson. All rights reserved.             */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>

#include "xbt.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(Test,"this test");

#define NB_ELM 100 /*00*/
#define DEPTH 5
#define KEY_SIZE 512
#define NB_TEST 20 /*20*/
int verbose=0;

static void str_free(void *s) {
  char *c=*(char**)s;
  free(c);
}

int main(int argc, char *argv[]) {
  xbt_dict_t mdict = NULL;
  int i,j,k,l;
  xbt_dynar_t keys = xbt_dynar_new(sizeof(char*),str_free);
  void *data;
  char *key;

  xbt_init(&argc,argv);

  printf("\nGeneric multicache: CRASH test:\n");
  printf(" Fill the struct and frees it %d times, using %d elements, depth of multicache=%d\n",NB_TEST,NB_ELM,DEPTH);
  printf(" with %d chars long randomized keys. (a point is a test)\n",KEY_SIZE);

  for (l=0 ; l<DEPTH ; l++) {
    key=xbt_malloc(KEY_SIZE);
    xbt_dynar_push(keys,&key);
  }	


  for (i=0;i<NB_TEST;i++) {
    mdict = xbt_dict_new();
    VERB1("mdict=%p",mdict);
    if (verbose>0) {
      printf("Test %d\n",i);
    } else if (verbose==0) {
      if (i%10) printf("."); else printf("%d",i/10);
    }
    fflush(stdout);
    
    for (j=0;j<NB_ELM;j++) {
      if (verbose>0) printf ("  Add {");
      
      for (l=0 ; l<DEPTH ; l++) {
        key=*(char**)xbt_dynar_get_ptr(keys,l);
        
	for (k=0;k<KEY_SIZE-1;k++) 
	  key[k]=rand() % ('z' - 'a') + 'a';
	  
	key[k]='\0';
	
	if (verbose>0) printf("%p=%s %s ",key, key,(l<DEPTH-1?";":"}"));
      }
      if (verbose>0) printf("in multitree %p.\n",mdict);
                                                        
      xbt_multidict_set(mdict,keys,xbt_strdup(key),free);

      data = xbt_multidict_get(mdict,keys);

      xbt_assert2(data && !strcmp((char*)data,key),
	          "Retrieved value (%s) does not match the entrered one (%s)\n",
		  (char*)data,key);
    }
    xbt_dict_free(&mdict);
  }
  
  xbt_dynar_free(&keys);

/*  if (verbose>0)
    xbt_dict_dump(mdict,&xbt_dict_print);*/
    
  xbt_dict_free(&mdict);
  xbt_dynar_free(&keys);
  printf("\n");

  xbt_exit();
  return 0;
}
