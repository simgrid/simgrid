/* $Id$ */

/* dynar_int: A test case for the dynar using integers as content           */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras.h"

#define NB_ELEM 5000
XBT_LOG_NEW_DEFAULT_CATEGORY(test,"Logging specific to this test");

int main(int argc,char *argv[]) {
   /* Vars_decl [doxygen cruft] */
   xbt_dynar_t d;
   xbt_error_t errcode;
   int i,cpt,cursor;
   int *iptr;
   
   xbt_init(&argc,argv);

   INFO0("==== Traverse the empty dynar");
   d=xbt_dynar_new(sizeof(int),NULL);
   xbt_dynar_foreach(d,cursor,i){
     xbt_assert0(0,"Damnit, there is something in the empty dynar");
   }
   xbt_dynar_free(&d);
   xbt_dynar_free(&d);

   INFO1("==== Push %d int, set them again 3 times, traverse them, shift them",
	NB_ELEM);
   /* Populate_ints [doxygen cruft] */
   /* 1. Populate the dynar */
   d=xbt_dynar_new(sizeof(int),NULL);
   for (cpt=0; cpt< NB_ELEM; cpt++) {
     xbt_dynar_push_as(d,int,cpt); /* This is faster (and possible only with scalars) */
     /* xbt_dynar_push(d,&cpt);       This would also work */
     DEBUG2("Push %d, length=%lu",cpt, xbt_dynar_length(d));
   }
   
   /* 2. Traverse manually the dynar */
   for (cursor=0; cursor< NB_ELEM; cursor++) {
     iptr=xbt_dynar_get_ptr(d,cursor);
     xbt_assert2(cursor == *iptr,
		 "The retrieved value is not the same than the injected one (%d!=%d)",
		 cursor,cpt);
   }
   
   /* 3. Traverse the dynar using the neat macro to that extend */
   xbt_dynar_foreach(d,cursor,cpt){
     xbt_assert2(cursor == cpt,
		 "The retrieved value is not the same than the injected one (%d!=%d)",
		 cursor,cpt);
   }
   /* end_of_traversal */
   
   for (cpt=0; cpt< NB_ELEM; cpt++)
     *(int*)xbt_dynar_get_ptr(d,cpt) = cpt;

   for (cpt=0; cpt< NB_ELEM; cpt++) 
     *(int*)xbt_dynar_get_ptr(d,cpt) = cpt;
/*     xbt_dynar_set(d,cpt,&cpt);*/
   
   for (cpt=0; cpt< NB_ELEM; cpt++) 
     *(int*)xbt_dynar_get_ptr(d,cpt) = cpt;
   
   cpt=0;
   xbt_dynar_foreach(d,cursor,i){
     xbt_assert2(i == cpt,
	  "The retrieved value is not the same than the injected one (%d!=%d)",
		  i,cpt);
     cpt++;
   }
   xbt_assert2(cpt == NB_ELEM,
		"Cannot retrieve my %d values. Last got one is %d",
		NB_ELEM, cpt);

   /* shifting [doxygen cruft] */
   /* 4. Shift all the values */
   for (cpt=0; cpt< NB_ELEM; cpt++) {
     xbt_dynar_shift(d,&i);
     xbt_assert2(i == cpt,
           "The retrieved value is not the same than the injected one (%d!=%d)",
	       i,cpt);
     DEBUG2("Pop %d, length=%lu",cpt, xbt_dynar_length(d));
   }
   
   /* 5. Free the resources */
   xbt_dynar_free(&d);
   xbt_dynar_free(&d);

   
   INFO1("==== Unshift/pop %d int",NB_ELEM);
   d=xbt_dynar_new(sizeof(int),NULL);
   for (cpt=0; cpt< NB_ELEM; cpt++) {
     xbt_dynar_unshift(d,&cpt);
     DEBUG2("Push %d, length=%lu",cpt, xbt_dynar_length(d));
   }
   for (cpt=0; cpt< NB_ELEM; cpt++) {
     i=xbt_dynar_pop_as(d,int);
     xbt_assert2(i == cpt,
           "The retrieved value is not the same than the injected one (%d!=%d)",
		 i,cpt);
     DEBUG2("Pop %d, length=%lu",cpt, xbt_dynar_length(d));
   }
   xbt_dynar_free(&d);
   xbt_dynar_free(&d);

   
   INFO1("==== Push %d int, insert 1000 int in the middle, shift everything",NB_ELEM);
   d=xbt_dynar_new(sizeof(int),NULL);
   for (cpt=0; cpt< NB_ELEM; cpt++) {
     xbt_dynar_push_as(d,int,cpt);
     DEBUG2("Push %d, length=%lu",cpt, xbt_dynar_length(d));
   }
   for (cpt=0; cpt< 1000; cpt++) {
     xbt_dynar_insert_at_as(d,2500,int,cpt);
     DEBUG2("Push %d, length=%lu",cpt, xbt_dynar_length(d));
   }

   for (cpt=0; cpt< 2500; cpt++) {
     xbt_dynar_shift(d,&i);
     xbt_assert2(i == cpt,
           "The retrieved value is not the same than the injected one at the begining (%d!=%d)",
	       i,cpt);
     DEBUG2("Pop %d, length=%lu",cpt, xbt_dynar_length(d));
   }
   for (cpt=999; cpt>=0; cpt--) {
     xbt_dynar_shift(d,&i);
     xbt_assert2(i == cpt,
           "The retrieved value is not the same than the injected one in the middle (%d!=%d)",
	       i,cpt);
   }
   for (cpt=2500; cpt< NB_ELEM; cpt++) {
     xbt_dynar_shift(d,&i);
      xbt_assert2(i == cpt,
           "The retrieved value is not the same than the injected one at the end (%d!=%d)",
	       i,cpt);
   }
   xbt_dynar_free(&d);
   xbt_dynar_free(&d);


   INFO1("==== Push %d int, remove 2000-4000. free the rest",NB_ELEM);
   d=xbt_dynar_new(sizeof(int),NULL);
   for (cpt=0; cpt< NB_ELEM; cpt++) 
     xbt_dynar_push_as(d,int,cpt);
   
   for (cpt=2000; cpt< 4000; cpt++) {
     xbt_dynar_remove_at(d,2000,&i);
     xbt_assert2(i == cpt,
		  "Remove a bad value. Got %d, expected %d",
		  i,cpt);
     DEBUG2("remove %d, length=%lu",cpt, xbt_dynar_length(d));
   }
   xbt_dynar_free(&d);
   xbt_dynar_free(&d);

   xbt_exit();
   return 0;
}
