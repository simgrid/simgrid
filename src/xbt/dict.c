/* $Id$ */

/* dict - a generic dictionnary, variation over the B-tree concept          */

/* Copyright (c) 2003,2004 Martin Quinson. All rights reserved.             */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex.h"
#include "dict_private.h"

XBT_LOG_NEW_SUBCATEGORY(dict,xbt,
   "Dictionaries provide the same functionnalities than hash tables");
/*####[ Private prototypes ]#################################################*/

/*####[ Code ]###############################################################*/

/**
 * @brief Constructor
 * @return pointer to the destination
 *
 * Creates and initialize a new dictionnary
 */
xbt_dict_t 
xbt_dict_new(void) {
  xbt_dict_t res= xbt_new(s_xbt_dict_t,1);
  res->head=NULL;
  return res;
}
/**
 * @brief Destructor
 * @param dict the dictionnary to be freed
 *
 * Frees a cache structure with all its childs.
 */
void
xbt_dict_free(xbt_dict_t *dict)  {
  if (dict && *dict) {
    if ((*dict)->head) {
      xbt_dictelm_free( &( (*dict)->head ) );
      (*dict)->head = NULL;
    }
    free(*dict);
    *dict=NULL;
  }
}

/**
 * \brief Add data to the dict (arbitrary key)
 * \param dict the container
 * \param key the key to set the new data
 * \param key_len the size of the \a key
 * \param data the data to add in the dict
 * \param free_ctn function to call with (\a key as argument) when 
 *        \a key is removed from the dictionnary
 *
 * set the \a data in the structure under the \a key, which can be any kind 
 * of data, as long as its length is provided in \a key_len.
 */
void
xbt_dict_set_ext(xbt_dict_t      dict,
		  const char      *key,
		  int              key_len,
		  void            *data,
		  void_f_pvoid_t  *free_ctn) {

  xbt_assert(dict);

  xbt_dictelm_set_ext(&(dict->head),
		       key, key_len, data, free_ctn);
}

/**
 * \brief Add data to the dict (null-terminated key)
 *
 * \param dict the head of the dict
 * \param key the key to set the new data
 * \param data the data to add in the dict
 * \param free_ctn function to call with (\a key as argument) when 
 *        \a key is removed from the dictionnary
 *
 * set the \a data in the structure under the \a key, which is a 
 * null terminated string.
 */
void
xbt_dict_set(xbt_dict_t     dict,
	      const char     *key,
	      void           *data,
	      void_f_pvoid_t *free_ctn) {

  xbt_assert(dict);
  
  xbt_dictelm_set(&(dict->head), key, data, free_ctn);
}

/**
 * \brief Retrieve data from the dict (arbitrary key)
 *
 * \param dict the dealer of data
 * \param key the key to find data
 * \param key_len the size of the \a key
 * \returns the data that we are looking for
 *
 * Search the given \a key. throws not_found_error when not found.
 */
void *
xbt_dict_get_ext(xbt_dict_t      dict,
                 const char     *key,
                 int             key_len) {

  xbt_assert(dict);

  return xbt_dictelm_get_ext(dict->head, key, key_len);
}

/**
 * \brief Retrieve data from the dict (null-terminated key) 
 *
 * \param dict the dealer of data
 * \param key the key to find data
 * \returns the data that we are looking for
 *
 * Search the given \a key. THROWs mismatch_error when not found. 
 * Check xbt_dict_get_or_null() for a version returning NULL without exception when 
 * not found.
 */
void *
xbt_dict_get(xbt_dict_t     dict,
             const char     *key) {
  xbt_assert(dict);

  return xbt_dictelm_get(dict->head, key);
}

/**
 * \brief like xbt_dict_get(), but returning NULL when not found
 */
void *
xbt_dict_get_or_null(xbt_dict_t     dict,
		     const char     *key) {
  xbt_ex_t e;
  void *res=NULL;
  TRY {
    res = xbt_dictelm_get(dict->head, key);
  } CATCH(e) {
    if (e.category != not_found_error) 
      RETHROW;
    xbt_ex_free(&e);
    res=NULL;
  }
  return res;
}


/**
 * \brief Remove data from the dict (arbitrary key)
 *
 * \param dict the trash can
 * \param key the key of the data to be removed
 * \param key_len the size of the \a key
 * 
 *
 * Remove the entry associated with the given \a key (throws not_found)
 */
void
xbt_dict_remove_ext(xbt_dict_t  dict,
                     const char  *key,
                     int          key_len) {
  xbt_assert(dict);
  
  xbt_dictelm_remove_ext(dict->head, key, key_len);
}

/**
 * \brief Remove data from the dict (null-terminated key)
 *
 * \param dict the head of the dict
 * \param key the key of the data to be removed
 *
 * Remove the entry associated with the given \a key
 */
void
xbt_dict_remove(xbt_dict_t  dict,
		 const char  *key) {
  if (!dict)
    THROW1(arg_error,0,"Asked to remove key %s from NULL dict",key);

  xbt_dictelm_remove(dict->head, key);
}


/**
 * \brief Outputs the content of the structure (debuging purpose) 
 *
 * \param dict the exibitionist
 * \param output a function to dump each data in the tree
 *
 * Ouputs the content of the structure. (for debuging purpose). \a ouput is a
 * function to output the data. If NULL, data won't be displayed, just the tree structure.
 */

void
xbt_dict_dump(xbt_dict_t     dict,
               void_f_pvoid_t *output) {

  printf("Dict %p:\n", (void*)dict);
  xbt_dictelm_dump(dict ? dict->head: NULL, output);
}

#ifdef SIMGRID_TEST
#include "xbt.h"
#include "xbt/ex.h"
#include "portable.h"

XBT_LOG_EXTERNAL_CATEGORY(dict);
XBT_LOG_DEFAULT_CATEGORY(dict);

XBT_TEST_SUITE("dict","Dict data container");

static void print_str(void *str) {
  printf("%s",(char*)PRINTF_STR(str));
}

static void debuged_add(xbt_dict_t head,const char*key)
{
  char *data=xbt_strdup(key);

  xbt_test_log1("Add %s",PRINTF_STR(key));

  xbt_dict_set(head,key,data,&free);
  if (XBT_LOG_ISENABLED(dict,xbt_log_priority_debug)) {
    xbt_dict_dump(head,(void (*)(void*))&printf);
    fflush(stdout);
  }
}

static void fill(xbt_dict_t *head) {
  xbt_test_add0("Fill in the dictionnary");

  *head = xbt_dict_new();
  debuged_add(*head,"12");
  debuged_add(*head,"12a");
  debuged_add(*head,"12b");
  debuged_add(*head,"123");
  debuged_add(*head,"123456");
  /* Child becomes child of what to add */
  debuged_add(*head,"1234");
  /* Need of common ancestor */
  debuged_add(*head,"123457");

}

static void search(xbt_dict_t head,const char*key) {
  void *data;
  
  xbt_test_add1("Search %s",key);
  data=xbt_dict_get(head,key);
  xbt_test_log1("Found %s",(char *)data);
  if (data)
    xbt_test_assert0(!strcmp((char*)data,key),"Key and data do not match");
}

static void debuged_remove(xbt_dict_t head,const char*key) {

  xbt_test_add1("Remove '%s'",PRINTF_STR(key));
  xbt_dict_remove(head,key);
  /*  xbt_dict_dump(head,(void (*)(void*))&printf); */
}


static void traverse(xbt_dict_t head) {
  xbt_dict_cursor_t cursor=NULL;
  char *key;
  char *data;

  xbt_dict_foreach(head,cursor,key,data) {
    xbt_test_log2("Seen:  %s->%s",PRINTF_STR(key),PRINTF_STR(data));
    xbt_test_assert2(!data || !strcmp(key,data),
		     "Key(%s) != value(%s). Abording\n",key,data);
  }
}

static void search_not_found(xbt_dict_t head, const char *data) {
  xbt_ex_t e;

  xbt_test_add1("Search %s (expected not to be found)",data);

  TRY {    
    data = xbt_dict_get(head,"Can't be found");
    THROW1(unknown_error,0,"Found something which shouldn't be there (%s)",data);
  } CATCH(e) {
    if (e.category != not_found_error) 
      xbt_test_exception(e);
    xbt_ex_free(&e);
  }
}

xbt_ex_t e;
xbt_dict_t head=NULL;
char *data;


XBT_TEST_UNIT("basic",test_dict_basic,"Basic usage: change, retrive, traverse"){

  xbt_test_add0("Traversal the empty dictionnary");
  traverse(head);

  xbt_test_add0("Traverse the full dictionnary");
  fill(&head);

  search(head,"12a");
  traverse(head);

  xbt_test_add0("Free the dictionnary (twice)");
  xbt_dict_free(&head);
  xbt_dict_free(&head);

  /* CHANGING */
  fill(&head);
  xbt_test_add0("Change 123 to 'Changed 123'");
  xbt_dict_set(head,"123",strdup("Changed 123"),&free);

  xbt_test_add0("Change 123 back to '123'");
  xbt_dict_set(head,"123",strdup("123"),&free);

  xbt_test_add0("Change 12a to 'Dummy 12a'");
  xbt_dict_set(head,"12a",strdup("Dummy 12a"),&free);

  xbt_test_add0("Change 12a to '12a'");
  xbt_dict_set(head,"12a",strdup("12a"),&free);

  xbt_test_add0("Traverse the resulting dictionnary");
  traverse(head);
  
  /* RETRIVE */
  xbt_test_add0("Search 123");
  data = xbt_dict_get(head,"123");
  xbt_test_assert(data);
  xbt_test_assert(!strcmp("123",data));

  search_not_found(head,"Can't be found");
  search_not_found(head,"123 Can't be found");
  search_not_found(head,"12345678 NOT");

  search(head,"12a");
  search(head,"12b");
  search(head,"12");
  search(head,"123456");
  search(head,"1234");
  search(head,"123457");

  xbt_test_add0("Traverse the resulting dictionnary");
  traverse(head);

  /*  xbt_dict_dump(head,(void (*)(void*))&printf); */

  xbt_test_add0("Free the dictionnary twice");
  xbt_dict_free(&head);
  xbt_dict_free(&head);

  xbt_test_add0("Traverse the resulting dictionnary");
  traverse(head);
}

XBT_TEST_UNIT("remove",test_dict_remove,"Removing some values"){
  fill(&head);
  xbt_test_add0("Remove non existing data");
  TRY {
    debuged_remove(head,"Does not exist");
  } CATCH(e) {
    if (e.category != not_found_error) 
      xbt_test_exception(e);
    xbt_ex_free(&e);
  }
  traverse(head);

  xbt_dict_free(&head);

  xbt_test_add0("Remove data from the NULL dict");
  TRY {
    debuged_remove(head,"12345");
  } CATCH(e) {
    if (e.category != arg_error) 
      xbt_test_exception(e);
    xbt_ex_free(&e);
  } 

  xbt_test_add0("Remove each data manually (traversing the resulting dictionnary each time)");
  fill(&head);
  debuged_remove(head,"12a");    traverse(head);
  debuged_remove(head,"12b");    traverse(head);
  debuged_remove(head,"12");     traverse(head);
  debuged_remove(head,"123456"); traverse(head);
  TRY {
    debuged_remove(head,"12346");
  } CATCH(e) {
    if (e.category != not_found_error) 
      xbt_test_exception(e);
    xbt_ex_free(&e);         
    traverse(head);
  } 
  debuged_remove(head,"1234");   traverse(head);
  debuged_remove(head,"123457"); traverse(head);
  debuged_remove(head,"123");    traverse(head);
  TRY {
    debuged_remove(head,"12346");
  } CATCH(e) {
    if (e.category != not_found_error) 
      xbt_test_exception(e);
    xbt_ex_free(&e);
  }                              traverse(head);
  
  xbt_test_add0("Free the dictionnary twice");
  xbt_dict_free(&head);
  xbt_dict_free(&head);      
}

XBT_TEST_UNIT("nulldata",test_dict_nulldata,"NULL data management"){
  fill(&head);

  xbt_test_add0("Store NULL under 'null'");
  xbt_dict_set(head,"null",NULL,NULL);
  search(head,"null");

  xbt_test_add0("Check whether I see it while traversing...");
  {
    xbt_dict_cursor_t cursor=NULL;
    char *key;
    int found=0;
    
    xbt_dict_foreach(head,cursor,key,data) {
      xbt_test_log2("Seen:  %s->%s",PRINTF_STR(key),PRINTF_STR(data));
      if (!strcmp(key,"null"))
	found = 1;
    }
    xbt_test_assert0(found,"the key 'null', associated to NULL is not found");
  }
  xbt_dict_free(&head);
}

#define NB_ELM 20000
#define SIZEOFKEY 1024
static int countelems(xbt_dict_t head) {
  xbt_dict_cursor_t cursor;
  char *key;
  void *data;
  int res = 0;

  xbt_dict_foreach(head,cursor,key,data) {
    res++;
  }
  return res;
}

XBT_TEST_UNIT("crash",test_dict_crash,"Crash test"){
  xbt_dict_t head=NULL;
  int i,j,k, nb;
  char *key;
  void *data;

  srand((unsigned int)time(NULL));

  xbt_test_add0("CRASH test");
  xbt_test_log0("Fill the struct, count its elems and frees the structure (x10)");
  xbt_test_log1("using 1000 elements with %d chars long randomized keys.",SIZEOFKEY);

  for (i=0;i<10;i++) {
    head=xbt_dict_new();
    //    if (i%10) printf("."); else printf("%d",i/10); fflush(stdout);
    nb=0;
    for (j=0;j<1000;j++) {
      key=xbt_malloc(SIZEOFKEY);

      for (k=0;k<SIZEOFKEY-1;k++)
	key[k]=rand() % ('z' - 'a') + 'a';
      key[k]='\0';
      /*      printf("[%d %s]\n",j,key); */
      xbt_dict_set(head,key,key,&free);
    }
    /*    xbt_dict_dump(head,(void (*)(void*))&printf); */
    nb = countelems(head);
    xbt_test_assert1(nb == 1000,"found %d elements instead of 1000",nb);
    traverse(head);
    xbt_dict_free(&head);
    xbt_dict_free(&head);
  }


  head=xbt_dict_new();
  xbt_test_add1("Fill %d elements, with keys being the number of element",NB_ELM);
  for (j=0;j<NB_ELM;j++) {
    //    if (!(j%1000)) { printf("."); fflush(stdout); }

    key = xbt_malloc(10);
    
    sprintf(key,"%d",j);
    xbt_dict_set(head,key,key,&free);
  }

  xbt_test_add0("Count the elements (retrieving the key and data for each)");
  i = countelems(head);
  xbt_test_log1("There is %d elements",i);

  xbt_test_add1("Search my %d elements 20 times",NB_ELM);
  key=xbt_malloc(10);
  for (i=0;i<20;i++) {
    //    if (i%10) printf("."); else printf("%d",i/10); fflush(stdout);
    for (j=0;j<NB_ELM;j++) {
      
      sprintf(key,"%d",j);
      data = xbt_dict_get(head,key);
      xbt_test_assert2(!strcmp(key,(char*)data),
		       "key=%s != data=%s\n",key,(char*)data);
    }
  }
  free(key);

  xbt_test_add1("Remove my %d elements",NB_ELM);
  key=xbt_malloc(10);
  for (j=0;j<NB_ELM;j++) {
    //if (!(j%10000)) printf("."); fflush(stdout);
    
    sprintf(key,"%d",j);
    xbt_dict_remove(head,key);
  }
  free(key);

  
  xbt_test_add0("Free the structure (twice)");
  xbt_dict_free(&head);
  xbt_dict_free(&head);
}

static void str_free(void *s) {
  char *c=*(char**)s;
  free(c);
}

XBT_TEST_UNIT("multicrash",test_dict_multicrash,"Multi-dict crash test"){

#undef NB_ELM
#define NB_ELM 100 /*00*/
#define DEPTH 5
#define KEY_SIZE 512
#define NB_TEST 20 /*20*/
int verbose=0;

  xbt_dict_t mdict = NULL;
  int i,j,k,l;
  xbt_dynar_t keys = xbt_dynar_new(sizeof(char*),str_free);
  void *data;
  char *key;


  xbt_test_add0("Generic multicache CRASH test");
  xbt_test_log4(" Fill the struct and frees it %d times, using %d elements, "
		"depth of multicache=%d, key size=%d",
		NB_TEST,NB_ELM,DEPTH,KEY_SIZE);

  for (l=0 ; l<DEPTH ; l++) {
    key=xbt_malloc(KEY_SIZE);
    xbt_dynar_push(keys,&key);
  }	

  for (i=0;i<NB_TEST;i++) {
    mdict = xbt_dict_new();
    VERB1("mdict=%p",mdict);
    if (verbose>0)
      printf("Test %d\n",i);
    /* else if (i%10) printf("."); else printf("%d",i/10);*/
    
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

      xbt_test_assert2(data && !strcmp((char*)data,key),
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

}
#endif /* SIMGRID_TEST */
