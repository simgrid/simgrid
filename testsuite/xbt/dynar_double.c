/* $Id$ */

/* dynar_double: A test case for the dynar using doubles as content         */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003 the OURAGAN project.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <gras.h>

int main(int argc,char *argv[]) {
   gras_dynar_t *d;
   gras_error_t errcode;
   int cpt;
   double d1,d2;
   
   fprintf(stderr,"==== Push/shift 5000 doubles\n");
   TRYFAIL(gras_dynar_new(&d,sizeof(double),NULL));
   for (cpt=0; cpt< 5000; cpt++) {
     d1=(double)cpt;
     TRYFAIL(gras_dynar_push(d,&d1));
   }
   for (cpt=0; cpt< 5000; cpt++) {
     d1=(double)cpt;
     gras_dynar_shift(d,&d2);
     if (d1 != d2) {
       fprintf(stderr,
           "The retrieved value is not the same than the injected one (%f!=%f)\n",
	       d1,d2);
       abort();
     }
   }
   gras_dynar_free(d);


   fprintf(stderr,"==== Unshift/pop 5000 doubles\n");
   TRYFAIL(gras_dynar_new(&d,sizeof(double),NULL));
   for (cpt=0; cpt< 5000; cpt++) {
     d1=(double)cpt;
     TRYFAIL(gras_dynar_unshift(d,&d1));
   }
   for (cpt=0; cpt< 5000; cpt++) {
     d1=(double)cpt;
     gras_dynar_pop(d,&d2);
     if (d1 != d2) {
       fprintf(stderr,
           "The retrieved value is not the same than the injected one (%f!=%f)\n",
	       d1,d2);
       abort();
     }
   }
   gras_dynar_free(d);



   fprintf(stderr,"==== Push 5000 doubles, insert 1000 doubles in the middle, shift everything\n");
   TRYFAIL(gras_dynar_new(&d,sizeof(double),NULL));
   for (cpt=0; cpt< 5000; cpt++) {
     d1=(double)cpt;
     TRYFAIL(gras_dynar_push(d,&d1));
   }
   for (cpt=0; cpt< 1000; cpt++) {
     d1=(double)cpt;
     TRYFAIL(gras_dynar_insert_at(d,2500,&d1));
   }

   for (cpt=0; cpt< 2500; cpt++) {
     d1=(double)cpt;
     gras_dynar_shift(d,&d2);
     if (d1 != d2) {
       fprintf(stderr,
           "The retrieved value is not the same than the injected one at the begining (%f!=%f)\n",
	       d1,d2);
       abort();
     }
     //     fprintf (stderr,"Pop %d, length=%d \n",cpt, gras_dynar_length(d));
   }
   for (cpt=999; cpt>=0; cpt--) {
     d1=(double)cpt;
     gras_dynar_shift(d,&d2);
     if (d1 != d2) {
       fprintf(stderr,
           "The retrieved value is not the same than the injected one in the middle (%f!=%f)\n",
	       d1,d2);
       abort();
     }
   }
   for (cpt=2500; cpt< 5000; cpt++) {
     d1=(double)cpt;
     gras_dynar_shift(d,&d2);
     if (d1 != d2) {
       fprintf(stderr,
           "The retrieved value is not the same than the injected one at the end (%f!=%f)\n",
	       d1,d2);
       abort();
     }
   }
   gras_dynar_free(d);


   fprintf(stderr,"==== Push 5000 double, remove 2000-4000. free the rest\n");
   TRYFAIL(gras_dynar_new(&d,sizeof(double),NULL));
   for (cpt=0; cpt< 5000; cpt++) {
     d1=(double)cpt;
     TRYFAIL(gras_dynar_push(d,&d1));
   }
   for (cpt=2000; cpt< 4000; cpt++) {
     d1=(double)cpt;
     gras_dynar_remove_at(d,2000,&d2);
     if (d1 != d2) {
       fprintf(stderr,
           "Remove a bad value. Got %f, expected %f\n",
	       d2,d1);
       abort();
     }
   }
   gras_dynar_free(d);

   return 0;
}
