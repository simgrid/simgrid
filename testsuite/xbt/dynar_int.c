/* $Id$ */

/* dynar_int: A test case for the dynar using integers as content           */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003 the OURAGAN project.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <gras.h>

#define NB_ELEM 5000
GRAS_LOG_NEW_DEFAULT_CATEGORY(test,"Logging specific to this test");

int main(int argc,char *argv[]) {
   gras_dynar_t *d;
   gras_error_t errcode;
   int i,cpt,cursor;
   
   gras_init_defaultlog(&argc,argv,"dynar.thresh=debug");

   INFO0("==== Traverse the empty dynar");
   gras_dynar_new(&d,sizeof(int),NULL);
   gras_dynar_foreach(d,cursor,i){
     gras_assert0(0,"Damnit, there is something in the empty dynar");
   }
   gras_dynar_free(d);

   INFO1("==== Push %d int, set them again 3 times, traverse them, shift them",
	NB_ELEM);
   gras_dynar_new(&d,sizeof(int),NULL);
   for (cpt=0; cpt< NB_ELEM; cpt++) {
     gras_dynar_push(d,&cpt);
     DEBUG2("Push %d, length=%lu",cpt, gras_dynar_length(d));
   }
   for (cursor=0; cursor< NB_ELEM; cursor++) {
     gras_dynar_get(d,cursor,&cpt);
     gras_assert2(cursor == cpt,
		  "The retrieved value is not the same than the injected one (%d!=%d)",
		  cursor,cpt);
   }
   gras_dynar_foreach(d,cursor,cpt){
     gras_assert2(cursor == cpt,
     		  "The retrieved value is not the same than the injected one (%d!=%d)",
     		  cursor,cpt);
   }
   for (cpt=0; cpt< NB_ELEM; cpt++)
     gras_dynar_set(d,cpt,&cpt);

   for (cpt=0; cpt< NB_ELEM; cpt++) 
     gras_dynar_set(d,cpt,&cpt);
   
   for (cpt=0; cpt< NB_ELEM; cpt++) 
     gras_dynar_set(d,cpt,&cpt);
   
   cpt=0;
   gras_dynar_foreach(d,cursor,i){
     gras_assert2(i == cpt,
	  "The retrieved value is not the same than the injected one (%d!=%d)",
		  i,cpt);
     cpt++;
   }
   gras_assert2(cpt == NB_ELEM,
		"Cannot retrieve my %d values. Last got one is %d",
		NB_ELEM, cpt);

   for (cpt=0; cpt< NB_ELEM; cpt++) {
     gras_dynar_shift(d,&i);
     gras_assert2(i == cpt,
           "The retrieved value is not the same than the injected one (%d!=%d)",
	       i,cpt);
     DEBUG2("Pop %d, length=%lu",cpt, gras_dynar_length(d));
   }
   gras_dynar_free(d);

   
   INFO1("==== Unshift/pop %d int",NB_ELEM);
   gras_dynar_new(&d,sizeof(int),NULL);
   for (cpt=0; cpt< NB_ELEM; cpt++) {
     gras_dynar_unshift(d,&cpt);
     DEBUG2("Push %d, length=%lu",cpt, gras_dynar_length(d));
   }
   for (cpt=0; cpt< NB_ELEM; cpt++) {
     gras_dynar_pop(d,&i);
     gras_assert2(i == cpt,
           "The retrieved value is not the same than the injected one (%d!=%d)",
		 i,cpt);
     DEBUG2("Pop %d, length=%lu",cpt, gras_dynar_length(d));
   }
   gras_dynar_free(d);


   
   INFO1("==== Push %d int, insert 1000 int in the middle, shift everything",NB_ELEM);
   gras_dynar_new(&d,sizeof(int),NULL);
   for (cpt=0; cpt< NB_ELEM; cpt++) {
     gras_dynar_push(d,&cpt);
     DEBUG2("Push %d, length=%lu",cpt, gras_dynar_length(d));
   }
   for (cpt=0; cpt< 1000; cpt++) {
     gras_dynar_insert_at(d,2500,&cpt);
     DEBUG2("Push %d, length=%lu",cpt, gras_dynar_length(d));
   }

   for (cpt=0; cpt< 2500; cpt++) {
     gras_dynar_shift(d,&i);
     gras_assert2(i == cpt,
           "The retrieved value is not the same than the injected one at the begining (%d!=%d)",
	       i,cpt);
     DEBUG2("Pop %d, length=%lu",cpt, gras_dynar_length(d));
   }
   for (cpt=999; cpt>=0; cpt--) {
     gras_dynar_shift(d,&i);
     gras_assert2(i == cpt,
           "The retrieved value is not the same than the injected one in the middle (%d!=%d)",
	       i,cpt);
   }
   for (cpt=2500; cpt< NB_ELEM; cpt++) {
     gras_dynar_shift(d,&i);
      gras_assert2(i == cpt,
           "The retrieved value is not the same than the injected one at the end (%d!=%d)",
	       i,cpt);
   }
   gras_dynar_free(d);


   INFO1("==== Push %d int, remove 2000-4000. free the rest",NB_ELEM);
   gras_dynar_new(&d,sizeof(int),NULL);
   for (cpt=0; cpt< NB_ELEM; cpt++) 
     gras_dynar_push(d,&cpt);
   
   for (cpt=2000; cpt< 4000; cpt++) {
     gras_dynar_remove_at(d,2000,&i);
     gras_assert2(i == cpt,
		  "Remove a bad value. Got %d, expected %d",
		  i,cpt);
     DEBUG2("remove %d, length=%lu",cpt, gras_dynar_length(d));
   }
   gras_dynar_free(d);

   gras_exit();
   return 0;
}
