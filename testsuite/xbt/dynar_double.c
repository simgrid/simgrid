/* $Id$ */

/* dynar_double: A test case for the dynar using doubles as content         */

/* Copyright (c) 2003,2004 Martin Quinson. All rights reserved.             */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(test,"Logging specific to this test");

int main(int argc,char *argv[]) {
   xbt_dynar_t d;
   xbt_error_t errcode;
   int cpt,cursor;
   double d1,d2;
   
   xbt_init_defaultlog(&argc,argv,"dynar.thresh=debug");

   INFO0("==== Traverse the empty dynar");
   d=xbt_dynar_new(sizeof(int),NULL);
   xbt_dynar_foreach(d,cursor,cpt){
     xbt_assert0(FALSE,
	     "Damnit, there is something in the empty dynar");
   }
   xbt_dynar_free(&d);
   xbt_dynar_free(&d);

   INFO0("==== Push/shift 5000 doubles");
   d=xbt_dynar_new(sizeof(double),NULL);
   for (cpt=0; cpt< 5000; cpt++) {
     d1=(double)cpt;
     xbt_dynar_push(d,&d1);
   }
   xbt_dynar_foreach(d,cursor,d2){
     d1=(double)cursor;
     xbt_assert2(d1 == d2,
           "The retrieved value is not the same than the injected one (%f!=%f)",
		  d1,d2);
   }
   for (cpt=0; cpt< 5000; cpt++) {
     d1=(double)cpt;
     xbt_dynar_shift(d,&d2);
     xbt_assert2(d1 == d2,
           "The retrieved value is not the same than the injected one (%f!=%f)",
		  d1,d2);
   }
   xbt_dynar_free(&d);
   xbt_dynar_free(&d);


   INFO0("==== Unshift/pop 5000 doubles");
   d=xbt_dynar_new(sizeof(double),NULL);
   for (cpt=0; cpt< 5000; cpt++) {
     d1=(double)cpt;
     xbt_dynar_unshift(d,&d1);
   }
   for (cpt=0; cpt< 5000; cpt++) {
     d1=(double)cpt;
     xbt_dynar_pop(d,&d2);
     xbt_assert2 (d1 == d2,
           "The retrieved value is not the same than the injected one (%f!=%f)",
		   d1,d2);
   }
   xbt_dynar_free(&d);
   xbt_dynar_free(&d);



   INFO0("==== Push 5000 doubles, insert 1000 doubles in the middle, shift everything");
   d=xbt_dynar_new(sizeof(double),NULL);
   for (cpt=0; cpt< 5000; cpt++) {
     d1=(double)cpt;
     xbt_dynar_push(d,&d1);
   }
   for (cpt=0; cpt< 1000; cpt++) {
     d1=(double)cpt;
     xbt_dynar_insert_at(d,2500,&d1);
   }

   for (cpt=0; cpt< 2500; cpt++) {
     d1=(double)cpt;
     xbt_dynar_shift(d,&d2);
     xbt_assert2(d1 == d2,
           "The retrieved value is not the same than the injected one at the begining (%f!=%f)",
		  d1,d2);
     DEBUG2("Pop %d, length=%lu",cpt, xbt_dynar_length(d));
   }
   for (cpt=999; cpt>=0; cpt--) {
     d1=(double)cpt;
     xbt_dynar_shift(d,&d2);
     xbt_assert2 (d1 == d2,
           "The retrieved value is not the same than the injected one in the middle (%f!=%f)",
		   d1,d2);
   }
   for (cpt=2500; cpt< 5000; cpt++) {
     d1=(double)cpt;
     xbt_dynar_shift(d,&d2);
     xbt_assert2 (d1 == d2,
           "The retrieved value is not the same than the injected one at the end (%f!=%f)",
		   d1,d2);
   }
   xbt_dynar_free(&d);
   xbt_dynar_free(&d);


   INFO0("==== Push 5000 double, remove 2000-4000. free the rest");
   d=xbt_dynar_new(sizeof(double),NULL);
   for (cpt=0; cpt< 5000; cpt++) {
     d1=(double)cpt;
     xbt_dynar_push(d,&d1);
   }
   for (cpt=2000; cpt< 4000; cpt++) {
     d1=(double)cpt;
     xbt_dynar_remove_at(d,2000,&d2);
     xbt_assert2 (d1 == d2,
           "Remove a bad value. Got %f, expected %f",
	       d2,d1);
   }
   xbt_dynar_free(&d);
   xbt_dynar_free(&d);

   xbt_exit();
   return 0;
}
