/* $Id$ */

/* dynar_string: A test case for the dynar using strings as content         */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003 the OURAGAN project.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <gras.h>

void free_string(void *d);

void free_string(void *d){
  free(*(void**)d);
}

void parse_log_opt(int argc, char **argv,const char *deft);

int main(int argc,char *argv[]) {
   gras_dynar_t *d;
   gras_error_t errcode;
   int cpt,i;
   char buf[1024];
   char *s1,*s2;
   
   parse_log_opt(argc,argv,"dynar.thresh=debug");
   
   fprintf(stderr,"==== Traverse the empty dynar\n");
   TRYFAIL(gras_dynar_new(&d,sizeof(char *),&free_string));
   gras_dynar_foreach(d,cpt,i){
     fprintf(stderr,
	     "Damnit, there is something in the empty dynar\n");
     abort();
   }
   gras_dynar_free(d);

   fprintf(stderr,"==== Push 5000 strings, set them again 3 times, shift them\n");
   TRYFAIL(gras_dynar_new(&d,sizeof(char*),&free_string));
   for (cpt=0; cpt< 5000; cpt++) {
     sprintf(buf,"%d",cpt);
     s1=strdup(buf);
     TRYFAIL(gras_dynar_push(d,&s1));
   }
   for (cpt=0; cpt< 5000; cpt++) {
     sprintf(buf,"%d",cpt);
     s1=strdup(buf);
     TRYFAIL(gras_dynar_remplace(d,cpt,&s1));
   }
   for (cpt=0; cpt< 5000; cpt++) {
     sprintf(buf,"%d",cpt);
     s1=strdup(buf);
     TRYFAIL(gras_dynar_remplace(d,cpt,&s1));
   }
   for (cpt=0; cpt< 5000; cpt++) {
     sprintf(buf,"%d",cpt);
     s1=strdup(buf);
     TRYFAIL(gras_dynar_remplace(d,cpt,&s1));
   }
   for (cpt=0; cpt< 5000; cpt++) {
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


   fprintf(stderr,"==== Unshift/pop 5000 strings\n");
   TRYFAIL(gras_dynar_new(&d,sizeof(char**),&free_string));
   for (cpt=0; cpt< 5000; cpt++) {
     sprintf(buf,"%d",cpt);
     s1=strdup(buf);
     TRYFAIL(gras_dynar_unshift(d,&s1));
   }
   for (cpt=0; cpt< 5000; cpt++) {
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



   fprintf(stderr,"==== Push 5000 strings, insert 1000 strings in the middle, shift everything\n");
   TRYFAIL(gras_dynar_new(&d,sizeof(char*),&free_string));
   for (cpt=0; cpt< 5000; cpt++) {
     sprintf(buf,"%d",cpt);
     s1=strdup(buf);
     TRYFAIL(gras_dynar_push(d,&s1));
   }
   for (cpt=0; cpt< 1000; cpt++) {
     sprintf(buf,"%d",cpt);
     s1=strdup(buf);
     TRYFAIL(gras_dynar_insert_at(d,2500,&s1));
   }

   for (cpt=0; cpt< 2500; cpt++) {
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
   for (cpt=999; cpt>=0; cpt--) {
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
   for (cpt=2500; cpt< 5000; cpt++) {
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


   fprintf(stderr,"==== Push 5000 strings, remove 2000-4000. free the rest\n");
   TRYFAIL(gras_dynar_new(&d,sizeof(char*),&free_string));
   for (cpt=0; cpt< 5000; cpt++) {
     sprintf(buf,"%d",cpt);
     s1=strdup(buf);
     TRYFAIL(gras_dynar_push(d,&s1));
   }
   for (cpt=2000; cpt< 4000; cpt++) {
     sprintf(buf,"%d",cpt);
     gras_dynar_remove_at(d,2000,&s2);
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
