/* $Id$ */

/* gras.c - common parts for the Grid Reality And Simulation                */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003 the OURAGAN project.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras_private.h"

/**************************************************************************/
/**************************************************************************/
/* Lock handling                                                          */
/**************************************************************************/
/**************************************************************************/

#ifdef HAVE_LIBPTHREAD
#include <pthread.h>

/* this is the lock for this module. Every other module will use a void *
 * as a variable as mutex. */
pthread_mutex_t gras_thelock = PTHREAD_MUTEX_INITIALIZER;
#endif

int
gras_lock() {
  //fprintf(stderr,"Get Lock... ");
#ifdef HAVE_LIBPTHREAD
  int ret;
  
  ret = pthread_mutex_lock((pthread_mutex_t *)(&gras_thelock));
  if  (ret != 0) {
    fprintf(stderr, "gras_lock: Unable to lock (errno = %d)!\n", ret);
    return 0;
  }
#endif
  //fprintf(stderr,"ok\n");
  return 1;
}

int 
gras_unlock() {

  //  fprintf(stderr,"Release Lock... ");
#ifdef HAVE_LIBPTHREAD
  int ret;

  ret = pthread_mutex_unlock((pthread_mutex_t *)&gras_thelock);
  if (ret != 0) {
    fprintf(stderr, "grasReleaseLock: Unable to release lock (errno = %d)!\n", ret);
    return 0;
  }
#endif
  //fprintf(stderr,"ok\n");
  return 1;
}

/* **************************************************************************
 * Manipulating Callback list
 * **************************************************************************/
gras_cblist_t *gras_cb_get(gras_msgid_t id) {
  grasProcessData_t *pd=grasProcessDataGet();
  int i;

  for (i=0 ; i<pd->grasCblListLen && pd->grasCblList[i].id != id ; i++);
  return i==pd->grasCblListLen ? NULL : &(pd->grasCblList[i]);
}

gras_error_t gras_cb_create(gras_msgid_t message) {
  grasProcessData_t *pd=grasProcessDataGet();

  if (pd->grasCblListLen++) {
    pd->grasCblList = (gras_cblist_t *)realloc(pd->grasCblList,
					     sizeof(gras_cblist_t)*pd->grasCblListLen);
  } else {
    pd->grasCblList = (gras_cblist_t *)malloc(sizeof(gras_cblist_t)*
						      pd->grasCblListLen);
  }
  if (!pd->grasCblList) {
    fprintf(stderr,"PANIC: Malloc error (All callbacks for all hosts are lost)\n");
    pd->grasCblListLen=0;
    return malloc_error;
  }

  pd->grasCblList[pd->grasCblListLen-1].id=message;

  pd->grasCblList[pd->grasCblListLen-1].cbCount=0;
  pd->grasCblList[pd->grasCblListLen-1].cb=NULL;
  pd->grasCblList[pd->grasCblListLen-1].cbTTL=NULL;

  return no_error;
}

/* **************************************************************************
 * Manipulating User Data
 * **************************************************************************/
void *gras_userdata_get(void) { 
  grasProcessData_t *pd=grasProcessDataGet();

  return pd ? pd->userdata : NULL;
}

void *gras_userdata_set(void *ud) {
  grasProcessData_t *pd=grasProcessDataGet();

  ASSERT(pd,"ProcessData==NULL  =>  This process did not run grasInit()\n");
  return pd->userdata=ud;
}

