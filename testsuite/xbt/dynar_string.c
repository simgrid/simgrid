/* $Id$ */

/* dynar_string: A test case for the dynar using strings as content         */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003 the OURAGAN project.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <gras.h>

/* NB_ELEM HAS to be a multiple of 5 */
#define NB_ELEM 5000

void free_string(void *d);

void free_string(void *d){
  free(*(void**)d);
}

int main(int argc,char *argv[]) {
   gras_dynar_t *d;
   gras_error_t errcode;
   int cpt,i;
   char buf[1024];
   char *s1,*s2;
   
   gras_init_defaultlog(argc,argv,"dynar.thresh=debug");
   
   fprintf(stderr,"==== Traverse the empty dynar\n");
   TRYFAIL(gras_dynar_new(&d,sizeof(char *),&free_string));
   gras_dynar_foreach(d,cpt,i){
     fprintf(stderr,
	     "Damnit, there is something in the empty dynar\n");
     abort();
   }
   gras_dynar_free(d);

   fprintf(stderr,"==== Push %d strings, set them again 3 times, shift them\n",NB_ELEM);
   TRYFAIL(gras_dynar_new(&d,sizeof(char*),&free_string));
   for (cpt=0; cpt< NB_ELEM; cpt++) {
     sprintf(buf,"%d",cpt);
     s1=strdup(buf);
     TRYFAIL(gras_dynar_push(d,&s1));
   }
   for (cpt=0; cpt< NB_ELEM; cpt++) {
     sprintf(buf,"%d",cpt);
     s1=strdup(buf);
     TRYFAIL(gras_dynar_remplace(d,cpt,&s1));
   }
   for (cpt=0; cpt< NB_ELEM; cpt++) {
     sprintf(buf,"%d",cpt);
     s1=strdup(buf);
     TRYFAIL(gras_dynar_remplace(d,cpt,&s1));
   }
   for (cpt=0; cpt< NB_ELEM; cpt++) {
     sprintf(buf,"%d",cpt);
     s1=strdup(buf);
     TRYFAIL(gras_dynar_remplace(d,cpt,&s1));
   }
   for (cpt=0; cpt< NB_ELEM; cpt++) {
     sprintf(buf,"%d",cpt);
     gras_dynar_shift(d,&s2);
     if (strcmp(buf,s2)) {
       fprintf(stderr,
	    "The retrieved value is not the same than the injected one (%s!=%s)\n",
	       buf,s2);
       abort();
     }
     free(s2);
   }
   gras_dynar_free(d);


   fprintf(stderr,"==== Unshift/pop %d strings\n",NB_ELEM);
   TRYFAIL(gras_dynar_new(&d,sizeof(char**),&free_string));
   for (cpt=0; cpt< NB_ELEM; cpt++) {
     sprintf(buf,"%d",cpt);
     s1=strdup(buf);
     TRYFAIL(gras_dynar_unshift(d,&s1));
   }
   for (cpt=0; cpt< NB_ELEM; cpt++) {
     sprintf(buf,"%d",cpt);
     gras_dynar_pop(d,&s2);
     if (strcmp(buf,s2)) {
       fprintf(stderr,
           "The retrieved value is not the same than the injected one (%s!=%s)\n",
	       buf,s2);
       abort();
     }
     free(s2);
   }
   gras_dynar_free(d);



   fprintf(stderr,"==== Push %d strings, insert %d strings in the middle, shift everything\n",NB_ELEM,NB_ELEM/5);
   TRYFAIL(gras_dynar_new(&d,sizeof(char*),&free_string));
   for (cpt=0; cpt< NB_ELEM; cpt++) {
     sprintf(buf,"%d",cpt);
     s1=strdup(buf);
     TRYFAIL(gras_dynar_push(d,&s1));
   }
   for (cpt=0; cpt< NB_ELEM/5; cpt++) {
     sprintf(buf,"%d",cpt);
     s1=strdup(buf);
     TRYFAIL(gras_dynar_insert_at(d,NB_ELEM/2,&s1));
   }

   for (cpt=0; cpt< NB_ELEM/2; cpt++) {
     sprintf(buf,"%d",cpt);
     gras_dynar_shift(d,&s2);
     if (strcmp(buf,s2)) {
       fprintf(stderr,
           "The retrieved value is not the same than the injected one at the begining (%s!=%s)\n",
	       buf,s2);
       abort();
     }
     free(s2);
   }
   for (cpt=(NB_ELEM/5)-1; cpt>=0; cpt--) {
     sprintf(buf,"%d",cpt);
     gras_dynar_shift(d,&s2);
     if (strcmp(buf,s2)) {
       fprintf(stderr,
           "The retrieved value is not the same than the injected one in the middle (%s!=%s)\n",
	       buf,s2);
       abort();
     }
     free(s2);
   }
   for (cpt=NB_ELEM/2; cpt< NB_ELEM; cpt++) {
     sprintf(buf,"%d",cpt);
     gras_dynar_shift(d,&s2);
     if (strcmp(buf,s2)) {
       fprintf(stderr,
           "The retrieved value is not the same than the injected one at the end (%s!=%s)\n",
	       buf,s2);
       abort();
     }
     free(s2);
   }
   gras_dynar_free(d);


   fprintf(stderr,"==== Push %d strings, remove %d-%d. free the rest\n",NB_ELEM,2*(NB_ELEM/5),4*(NB_ELEM/5));
   TRYFAIL(gras_dynar_new(&d,sizeof(char*),&free_string));
   for (cpt=0; cpt< NB_ELEM; cpt++) {
     sprintf(buf,"%d",cpt);
     s1=strdup(buf);
     TRYFAIL(gras_dynar_push(d,&s1));
   }
   for (cpt=2*(NB_ELEM/5); cpt< 4*(NB_ELEM/5); cpt++) {
     sprintf(buf,"%d",cpt);
     gras_dynar_remove_at(d,2*(NB_ELEM/5),&s2);
     if (strcmp(buf,s2)) {
       fprintf(stderr,
           "Remove a bad value. Got %s, expected %s\n",
	       s2,buf);
       abort();
     }
     free(s2);
   }
   gras_dynar_free(d);

   return 0;
}
