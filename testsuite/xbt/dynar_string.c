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
XBT_LOG_NEW_DEFAULT_CATEGORY(test,"Logging specific to this test");

void free_string(void *d);

void free_string(void *d){
  free(*(void**)d);
}

int main(int argc,char *argv[]) {
   xbt_dynar_t d;
   xbt_error_t errcode;
   int cpt;
   char buf[1024];
   char *s1,*s2;
   
   xbt_init_defaultlog(&argc,argv,"dynar.thresh=debug");
   
   INFO0("==== Traverse the empty dynar");
   d=xbt_dynar_new(sizeof(char *),&free_string);
   xbt_dynar_foreach(d,cpt,s1){
     xbt_assert0(FALSE,
		  "Damnit, there is something in the empty dynar");
   }
   xbt_dynar_free(&d);
   xbt_dynar_free(&d);

   INFO1("==== Push %d strings, set them again 3 times, shift them",NB_ELEM);
   d=xbt_dynar_new(sizeof(char*),&free_string);
   for (cpt=0; cpt< NB_ELEM; cpt++) {
     sprintf(buf,"%d",cpt);
     s1=strdup(buf);
     xbt_dynar_push(d,&s1);
   }
   for (cpt=0; cpt< NB_ELEM; cpt++) {
     sprintf(buf,"%d",cpt);
     s1=strdup(buf);
     xbt_dynar_replace(d,cpt,&s1);
   }
   for (cpt=0; cpt< NB_ELEM; cpt++) {
     sprintf(buf,"%d",cpt);
     s1=strdup(buf);
     xbt_dynar_replace(d,cpt,&s1);
   }
   for (cpt=0; cpt< NB_ELEM; cpt++) {
     sprintf(buf,"%d",cpt);
     s1=strdup(buf);
     xbt_dynar_replace(d,cpt,&s1);
   }
   for (cpt=0; cpt< NB_ELEM; cpt++) {
     sprintf(buf,"%d",cpt);
     xbt_dynar_shift(d,&s2);
     xbt_assert2 (!strcmp(buf,s2),
	    "The retrieved value is not the same than the injected one (%s!=%s)",
		   buf,s2);
     free(s2);
   }
   xbt_dynar_free(&d);
   xbt_dynar_free(&d);


   INFO1("==== Unshift, traverse and pop %d strings",NB_ELEM);
   d=xbt_dynar_new(sizeof(char**),&free_string);
   for (cpt=0; cpt< NB_ELEM; cpt++) {
     sprintf(buf,"%d",cpt);
     s1=strdup(buf);
     xbt_dynar_unshift(d,&s1);
   }
   xbt_dynar_foreach(d,cpt,s1) {
     sprintf(buf,"%d",NB_ELEM - cpt -1);
     xbt_assert2 (!strcmp(buf,s1),
           "The retrieved value is not the same than the injected one (%s!=%s)",
	       buf,s1);
   }
   for (cpt=0; cpt< NB_ELEM; cpt++) {
     sprintf(buf,"%d",cpt);
     xbt_dynar_pop(d,&s2);
     xbt_assert2 (!strcmp(buf,s2),
           "The retrieved value is not the same than the injected one (%s!=%s)",
	       buf,s2);
     free(s2);
   }
   xbt_dynar_free(&d);
   xbt_dynar_free(&d);


   INFO2("==== Push %d strings, insert %d strings in the middle, shift everything",NB_ELEM,NB_ELEM/5);
   d=xbt_dynar_new(sizeof(char*),&free_string);
   for (cpt=0; cpt< NB_ELEM; cpt++) {
     sprintf(buf,"%d",cpt);
     s1=strdup(buf);
     xbt_dynar_push(d,&s1);
   }
   for (cpt=0; cpt< NB_ELEM/5; cpt++) {
     sprintf(buf,"%d",cpt);
     s1=strdup(buf);
     xbt_dynar_insert_at(d,NB_ELEM/2,&s1);
   }

   for (cpt=0; cpt< NB_ELEM/2; cpt++) {
     sprintf(buf,"%d",cpt);
     xbt_dynar_shift(d,&s2);
     xbt_assert2(!strcmp(buf,s2),
           "The retrieved value is not the same than the injected one at the begining (%s!=%s)",
	       buf,s2);
      free(s2);
   }
   for (cpt=(NB_ELEM/5)-1; cpt>=0; cpt--) {
     sprintf(buf,"%d",cpt);
     xbt_dynar_shift(d,&s2);
     xbt_assert2 (!strcmp(buf,s2),
           "The retrieved value is not the same than the injected one in the middle (%s!=%s)",
	       buf,s2);
     free(s2);
   }
   for (cpt=NB_ELEM/2; cpt< NB_ELEM; cpt++) {
     sprintf(buf,"%d",cpt);
     xbt_dynar_shift(d,&s2);
     xbt_assert2 (!strcmp(buf,s2),
           "The retrieved value is not the same than the injected one at the end (%s!=%s)",
	       buf,s2);
     free(s2);
   }
   xbt_dynar_free(&d);
   xbt_dynar_free(&d);


   INFO3("==== Push %d strings, remove %d-%d. free the rest",NB_ELEM,2*(NB_ELEM/5),4*(NB_ELEM/5));
   d=xbt_dynar_new(sizeof(char*),&free_string);
   for (cpt=0; cpt< NB_ELEM; cpt++) {
     sprintf(buf,"%d",cpt);
     s1=strdup(buf);
     xbt_dynar_push(d,&s1);
   }
   for (cpt=2*(NB_ELEM/5); cpt< 4*(NB_ELEM/5); cpt++) {
     sprintf(buf,"%d",cpt);
     xbt_dynar_remove_at(d,2*(NB_ELEM/5),&s2);
     xbt_assert2(!strcmp(buf,s2),
		  "Remove a bad value. Got %s, expected %s",
		  s2,buf);
      free(s2);
   }
   xbt_dynar_free(&d);
   xbt_dynar_free(&d);

   xbt_exit();
   return 0;
}
