/* $Id$ */

/* ex - Exception Handling (modified to fit into SimGrid from OSSP version) */

/*  Copyright (c) 2005, 2006, 2007 Martin Quinson                           */
/*  Copyright (c) 2002-2004 Ralf S. Engelschall <rse@engelschall.com>       */
/*  Copyright (c) 2002-2004 The OSSP Project <http://www.ossp.org/>         */
/*  Copyright (c) 2002-2004 Cable & Wireless <http://www.cw.com/>           */
/*  All rights reserved.                                                    */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>

#include "portable.h" /* execinfo when available */
#include "xbt/ex.h"
#include "xbt/str.h"
#include "xbt/module.h" /* xbt_binary_name */
#include "xbt/synchro.h" /* xbt_thread_self */

#include "gras/Virtu/virtu_interface.h" /* gras_os_myname */
#include "xbt/ex_interface.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_ex,xbt,"Exception mecanism");

/* default __ex_ctx callback function */
ex_ctx_t *__xbt_ex_ctx_default(void) {
  /* Don't scream: this is a default which is never used (so, yes, 
     there is one setjump container by running entity).

     This default gets overriden in xbt/xbt_os_thread.c so that it works in
     real life and in simulation when using threads to implement the simulation
     processes (ie, with pthreads and on windows).

     It also gets overriden in xbt/context.c when using ucontextes (as well as
     in Java for now, but after the java overhaul, it will get cleaned out)
  */
    static ex_ctx_t ctx = XBT_CTX_INITIALIZER;

    return &ctx;
}

/* Change raw libc symbols to file names and line numbers */
void xbt_ex_setup_backtrace(xbt_ex_t *e);

void xbt_backtrace_current(xbt_ex_t *e) {
#if defined(HAVE_EXECINFO_H) && defined(HAVE_POPEN) && defined(ADDR2LINE)
  e->used     = backtrace((void**)e->bt,XBT_BACKTRACE_SIZE);
  e->bt_strings = NULL;
  xbt_ex_setup_backtrace(e);
#endif
}

void xbt_backtrace_display(xbt_ex_t *e) {
#if defined(HAVE_EXECINFO_H) && defined(HAVE_POPEN) && defined(ADDR2LINE)
  int i;

  if (e->used == 0) {
     fprintf(stderr,"(backtrace not set)\n");
  } else {	
     fprintf(stderr,"Backtrace (displayed in thread %p):\n",
	     (void*)xbt_thread_self());
     for (i=1; i<e->used; i++) /* no need to display "xbt_display_backtrace" */
       fprintf(stderr,"---> %s\n",e->bt_strings[i] +4);
  }
   
  /* don't fool xbt_ex_free with uninitialized msg field */
  e->msg=NULL;
  e->remote=0;
  xbt_ex_free(*e);
#else 
  ERROR0("No backtrace on this arch");
#endif
}

/** \brief show the backtrace of the current point (lovely while debuging) */
void xbt_backtrace_display_current(void) {
  xbt_ex_t e;
  xbt_backtrace_current(&e);
  xbt_backtrace_display(&e);
}

#ifndef WIN32
extern char **environ; /* the environment, as specified by the opengroup */
#endif

void xbt_ex_setup_backtrace(xbt_ex_t *e)  {
#if defined(HAVE_EXECINFO_H) && defined(HAVE_POPEN) && defined(ADDR2LINE)
  int i;
  /* to get the backtrace from the libc */
  char **backtrace = backtrace_symbols (e->bt, e->used);
  
  /* To build the commandline of addr2line */
  char *cmd, *curr;
  
  /* to extract the addresses from the backtrace */
  char **addrs=xbt_new(char*,e->used);
  char buff[256],*p;
  
  /* To read the output of addr2line */
  FILE *pipe;
  char line_func[1024],line_pos[1024];

  /* size (in char) of pointers on this arch */
  int addr_len=0;

  /* To search for the right executable path when not trivial */
  struct stat stat_buf;
  char *binary_name = NULL;
   
  /* Some arches only have stubs of backtrace, no implementation (hppa comes to mind) */
  if (!e->used)
     return;
   
  /* build the commandline */
  if (stat(xbt_binary_name,&stat_buf)) {
    /* Damn. binary not in current dir. We'll have to dig the PATH to find it */
    int i;
    for (i=0; environ[i]; i++) {
      if (!strncmp("PATH=",environ[i], 5)) {	
	xbt_dynar_t path=xbt_str_split(environ[i] + 5, ":");
	unsigned int cpt;
	char *data;
	xbt_dynar_foreach(path, cpt, data) {
	  if (binary_name)
	    free(binary_name);
	  binary_name = bprintf("%s/%s",data,xbt_binary_name);
	  if (!stat(binary_name,&stat_buf)) {
	    /* Found. */
	    DEBUG1("Looked in the PATH for the binary. Found %s",binary_name);
	    xbt_dynar_free(&path);
	    break;
	  } 
	}
	if (stat(binary_name,&stat_buf)) {
	  /* not found */
	  e->used = 1;
	  e->bt_strings = xbt_new(char*,1);
	  e->bt_strings[0] = bprintf("(binary '%s' not found the path)",xbt_binary_name);
	  return;
	}
	xbt_dynar_free(&path);
	break;
      }	
    }
  } else {
    binary_name = xbt_strdup(xbt_binary_name);
  }      
  cmd = curr = xbt_new(char,strlen(ADDR2LINE)+25+strlen(binary_name)+32*e->used);
   
  curr += sprintf(curr,"%s -f -e %s ",ADDR2LINE,binary_name);
  free(binary_name);
   
  for (i=0; i<e->used;i++) {
    /* retrieve this address */
    DEBUG2("Retrieving address number %d from '%s'", i, backtrace[i]);
    snprintf(buff,256,"%s",strchr(backtrace[i],'[')+1);
    p=strchr(buff,']');
    *p='\0';
    if (strcmp(buff,"(nil)"))
       addrs[i]=bprintf("%s", buff);
    else
       addrs[i]=bprintf("0x0");
    DEBUG3("Set up a new address: %d, '%s'(%p)", i, addrs[i], addrs[i]);
     
    /* Add it to the command line args */
    curr+=sprintf(curr,"%s ",addrs[i]);
  } 
  addr_len = strlen(addrs[0]);

  /* parse the output and build a new backtrace */
  e->bt_strings = xbt_new(char*,e->used);
  
  VERB1("Fire a first command: '%s'", cmd);
  pipe = popen(cmd, "r");
  if (!pipe) {
    CRITICAL0("Cannot fork addr2line to display the backtrace");
    abort();
  }

  for (i=0; i<e->used; i++) {
    DEBUG2("Looking for symbol %d, addr = '%s'", i, addrs[i]); 
    fgets(line_func,1024,pipe);
    line_func[strlen(line_func)-1]='\0';
    fgets(line_pos,1024,pipe);
    line_pos[strlen(line_pos)-1]='\0';

    if (strcmp("??",line_func)) {
      DEBUG2("Found static symbol %s() at %s", line_func, line_pos);
      e->bt_strings[i] = bprintf("**   In %s() at %s", line_func,line_pos);
    } else {
      /* Damn. The symbol is in a dynamic library. Let's get wild */
      char *maps_name;
      FILE *maps;
      char maps_buff[512];

      long int addr,offset=0;
      char *p,*p2;

      char *subcmd;
      FILE *subpipe;
      int found=0;

      /* let's look for the offset of this library in our addressing space */
      maps_name=bprintf("/proc/%d/maps",(int)getpid());
      maps=fopen(maps_name,"r");

      sscanf(addrs[i],"%lx",&addr);
      sprintf(maps_buff,"%#lx",addr);
      
      if (strcmp(addrs[i],maps_buff)) {
	CRITICAL2("Cannot parse backtrace address '%s' (addr=%#lx)",
		  addrs[i], addr);
      }
      DEBUG2("addr=%s (as string) =%#lx (as number)",addrs[i],addr);

      while (!found) {
	long int first, last;
	if (fgets(maps_buff,512,maps) == NULL) 
	  break;
	if (i==0) {
	  maps_buff[strlen(maps_buff) -1]='\0';
	  DEBUG1("map line: %s", maps_buff);
	}
	sscanf(maps_buff,"%lx",&first);
	p=strchr(maps_buff,'-')+1;
	sscanf(p,"%lx",&last);
	if (first < addr && addr < last) {
	  offset = first;
	  found=1;
	}
	if (found) {	      
	   DEBUG3("%#lx in [%#lx-%#lx]", addr, first,last);
	   DEBUG0("Symbol found, map lines not further displayed (even if looking for next ones)");
	}
      }
      fclose(maps);
      free(maps_name);

      if (!found) {
	VERB0("Problem while reading the maps file. Following backtrace will be mangled.");
	DEBUG1("No dynamic. Static symbol: %s", backtrace[i]);
	e->bt_strings[i] = bprintf("**   In ?? (%s)", backtrace[i]);
	continue;
      }

      /* Ok, Found the offset of the maps line containing the searched symbol. 
	 We now need to substract this from the address we got from backtrace.
      */
      
      free(addrs[i]);
      addrs[i] = bprintf("0x%0*lx",addr_len-2,addr-offset);
      DEBUG2("offset=%#lx new addr=%s",offset,addrs[i]);

      /* Got it. We have our new address. Let's get the library path and we 
	 are set */ 
      p  = xbt_strdup(backtrace[i]);
      if (p[0]=='[') {
	 /* library path not displayed in the map file either... */
	 free(p);
	 sprintf(line_func,"??");
      } else {
	 p2 = strrchr(p,'(');
	 if (p2) *p2= '\0';
	 p2 = strrchr(p,' ');
	 if(p2) *p2= '\0';
      
	 /* Here we go, fire an addr2line up */
	 subcmd = bprintf("%s -f -e %s %s",ADDR2LINE,p, addrs[i]);
	 free(p);
	 VERB1("Fire a new command: '%s'",subcmd);
	 subpipe = popen(subcmd,"r");
	 if (!subpipe) {
	    CRITICAL0("Cannot fork addr2line to display the backtrace");
	    abort();
	 }
	 fgets(line_func,1024,subpipe);
	 line_func[strlen(line_func)-1]='\0';
	 fgets(line_pos,1024,subpipe);
	 line_pos[strlen(line_pos)-1]='\0';
	 pclose(subpipe);
	 free(subcmd);
      }

      /* check whether the trick worked */
      if (strcmp("??",line_func)) {
	DEBUG2("Found dynamic symbol %s() at %s", line_func, line_pos);
	e->bt_strings[i] = bprintf("**   In %s() at %s", line_func,line_pos);
      } else {
	/* damn, nothing to do here. Let's print the raw address */
	DEBUG1("Dynamic symbol not found. Raw address = %s", backtrace[i]);
	e->bt_strings[i] = bprintf("**   In ?? at %s", backtrace[i]);
      }
      
    }
    free(addrs[i]);
     
    /* Mask the bottom of the stack */    
    if (!strncmp("main",line_func,strlen("main")) ||
	!strncmp("xbt_thread_context_wrapper",line_func,strlen("xbt_thread_context_wrapper"))) {
       int j;
       for (j=i+1; j<e->used; j++)
	 free(addrs[j]);
       e->used = i;

       if (!strncmp("xbt_thread_context_wrapper",line_func,strlen("xbt_thread_context_wrapper"))) {
	  e->used++;
	  e->bt_strings[i] = bprintf("**   (in a separate thread)");
       }       
    }
     
    
  }
  pclose(pipe);
  free(addrs);
  free(backtrace);
  free(cmd);
#endif
}    

/** @brief shows an exception content and the associated stack if available */
void xbt_ex_display(xbt_ex_t *e)  {
  char *thrower=NULL;

  if (e->remote)
    thrower = bprintf(" on host %s(%d)",e->host,e->pid);

  fprintf(stderr,
	  "** SimGrid: UNCAUGHT EXCEPTION received on %s(%d): category: %s; value: %d\n"
	  "** %s\n"
	  "** Thrown by %s()%s\n",
	  gras_os_myname(),(*xbt_getpid)(),
	  xbt_ex_catname(e->category), e->value, e->msg,
	  e->procname,thrower?thrower:" in this process");
  CRITICAL1("%s",e->msg);

  if (thrower)
    free(thrower);

  if (!e->remote && !e->bt_strings)
    xbt_ex_setup_backtrace(e);

#if defined(HAVE_EXECINFO_H) && defined(HAVE_POPEN) && defined(ADDR2LINE)
  /* We have everything to build neat backtraces */
  {
    int i;
    
    fprintf(stderr,"\n");
    for (i=0; i<e->used; i++)
      fprintf(stderr,"%s\n",e->bt_strings[i]);
    
  }
#else
  fprintf(stderr," at %s:%d:%s (no backtrace available on that arch)\n",  
	  e->file,e->line,e->func);
#endif
  xbt_ex_free(*e);
}


/* default __ex_terminate callback function */
void __xbt_ex_terminate_default(xbt_ex_t *e)  {
  xbt_ex_display(e);

  abort();
}

/* the externally visible API */
XBT_EXPORT_NO_IMPORT(ex_ctx_cb_t)  __xbt_ex_ctx       = &__xbt_ex_ctx_default;
XBT_EXPORT_NO_IMPORT(ex_term_cb_t) __xbt_ex_terminate = &__xbt_ex_terminate_default;


void xbt_ex_free(xbt_ex_t e) {
  int i;

  if (e.msg) free(e.msg);
  if (e.remote) {
    free(e.procname);
    free(e.file);
    free(e.func);
    free(e.host);
  }

  if (e.bt_strings) {	
     for (i=0; i<e.used; i++) 
       free((char*)e.bt_strings[i]);
     free((char **)e.bt_strings);
  }
  /* memset(e,0,sizeof(xbt_ex_t)); */
}

/** \brief returns a short name for the given exception category */
const char * xbt_ex_catname(xbt_errcat_t cat) {
  switch (cat) {
  case unknown_error:   return  "unknown_err";
  case arg_error:       return "invalid_arg";
  case mismatch_error:  return "mismatch";
  case not_found_error: return "not found";
  case system_error:    return "system_err";
  case network_error:   return "network_err";
  case timeout_error:   return "timeout";
  case thread_error:    return "thread_err";
  default:              return "INVALID_ERR";
  }
}

#ifndef HAVE_EXECINFO_H
/* dummy implementation. We won't use the result, but ex.h needs it to be defined */
int backtrace (void **__array, int __size) {
  return 0;
}

#endif

#ifdef SIMGRID_TEST
#include <stdio.h>
#include "xbt/ex.h"

XBT_TEST_SUITE("xbt_ex","Exception Handling");

XBT_TEST_UNIT("controlflow",test_controlflow, "basic nested control flow") {
    xbt_ex_t ex;
    volatile int n=1;

    xbt_test_add0("basic nested control flow");

    TRY {
        if (n != 1)
            xbt_test_fail1("M1: n=%d (!= 1)", n);
        n++;
        TRY {
            if (n != 2)
                xbt_test_fail1("M2: n=%d (!= 2)", n);
            n++;
            THROW0(unknown_error,0,"something");
        } CATCH (ex) {
            if (n != 3)
                xbt_test_fail1("M3: n=%d (!= 3)", n);
            n++;
	    xbt_ex_free(ex);
        }
	n++;
        TRY {
            if (n != 5)
                xbt_test_fail1("M2: n=%d (!= 5)", n);
            n++;
            THROW0(unknown_error,0,"something");
        } CATCH (ex) {
            if (n != 6)
                xbt_test_fail1("M3: n=%d (!= 6)", n);
            n++;
            RETHROW;
            n++;
        }
        xbt_test_fail1("MX: n=%d (shouldn't reach this point)", n);
    }
    CATCH(ex) {
        if (n != 7)
            xbt_test_fail1("M4: n=%d (!= 7)", n);
        n++;
        xbt_ex_free(ex);
    }
    if (n != 8)
        xbt_test_fail1("M5: n=%d (!= 8)", n);
}

XBT_TEST_UNIT("value",test_value,"exception value passing") {
    xbt_ex_t ex;

    TRY {
        THROW0(unknown_error, 2, "toto");
    } CATCH(ex) {
        xbt_test_add0("exception value passing");
        if (ex.category != unknown_error)
            xbt_test_fail1("category=%d (!= 1)", ex.category);
        if (ex.value != 2)
            xbt_test_fail1("value=%d (!= 2)", ex.value);
        if (strcmp(ex.msg,"toto"))
            xbt_test_fail1("message=%s (!= toto)", ex.msg);
        xbt_ex_free(ex);
    }
}

XBT_TEST_UNIT("variables",test_variables,"variable value preservation") {
    xbt_ex_t ex;
    int r1, r2;
    volatile int v1, v2;

    r1 = r2 = v1 = v2 = 1234;
    TRY {
        r2 = 5678;
        v2 = 5678;
        THROW0(unknown_error, 0, "toto");
    } CATCH(ex) {
        xbt_test_add0("variable preservation");
        if (r1 != 1234)
            xbt_test_fail1("r1=%d (!= 1234)", r1);
        if (v1 != 1234)
            xbt_test_fail1("v1=%d (!= 1234)", v1);
        /* r2 is allowed to be destroyed because not volatile */
        if (v2 != 5678)
            xbt_test_fail1("v2=%d (!= 5678)", v2);
        xbt_ex_free(ex);
    }
}

XBT_TEST_UNIT("cleanup",test_cleanup,"cleanup handling") {
    xbt_ex_t ex;
    volatile int v1;
    int c;

    xbt_test_add0("cleanup handling");

    v1 = 1234;
    c = 0;
    TRY {
        v1 = 5678;
        THROW0(1, 2, "blah");
    } CLEANUP {
        if (v1 != 5678)
            xbt_test_fail1("v1 = %d (!= 5678)", v1);
        c = 1;
    } CATCH(ex) {
        if (v1 != 5678)
            xbt_test_fail1("v1 = %d (!= 5678)", v1);
        if (!(ex.category == 1 && ex.value == 2 && !strcmp(ex.msg,"blah")))
            xbt_test_fail0("unexpected exception contents");
        xbt_ex_free(ex);
    }
    if (!c)
        xbt_test_fail0("xbt_ex_free not executed");
}


/*
 * The following is the example included in the documentation. It's a good 
 * idea to check its syntax even if we don't try to run it.
 * And actually, it allows to put comments in the code despite doxygen.
 */ 
static char *mallocex(int size) {
  return NULL;
}
#define SMALLAMOUNT 10
#define TOOBIG 100000000

#if 0 /* this contains syntax errors, actually */
static void bad_example(void) {
  struct {char*first;} *globalcontext;
  ex_t ex;

  /* BAD_EXAMPLE */
  TRY {
    char *cp1, *cp2, *cp3;
    
    cp1 = mallocex(SMALLAMOUNT);
    globalcontext->first = cp1;
    cp2 = mallocex(TOOBIG);
    cp3 = mallocex(SMALLAMOUNT);
    strcpy(cp1, "foo");
    strcpy(cp2, "bar");
  } CLEANUP {
    if (cp3 != NULL) free(cp3);
    if (cp2 != NULL) free(cp2);
    if (cp1 != NULL) free(cp1);
  } CATCH(ex) {
    printf("cp3=%s", cp3);
    RETHROW;
  }
  /* end_of_bad_example */
}
#endif
typedef struct {char *first;} global_context_t;
   
static void good_example(void) {
  global_context_t *global_context=malloc(sizeof(global_context_t));
  xbt_ex_t ex;

  /* GOOD_EXAMPLE */
  { /*01*/
    char * volatile /*03*/ cp1 = NULL /*02*/;
    char * volatile /*03*/ cp2 = NULL /*02*/;
    char * volatile /*03*/ cp3 = NULL /*02*/;
    TRY {
      cp1 = mallocex(SMALLAMOUNT);
      global_context->first = cp1;
      cp1 = NULL /*05 give away*/;
      cp2 = mallocex(TOOBIG);
      cp3 = mallocex(SMALLAMOUNT);
      strcpy(cp1, "foo");
      strcpy(cp2, "bar");
    } CLEANUP { /*04*/
      printf("cp3=%s", cp3 == NULL /*02*/ ? "" : cp3);
      if (cp3 != NULL)
	free(cp3);
      if (cp2 != NULL)
	free(cp2);
      /*05 cp1 was given away */
    } CATCH(ex) {
      /*05 global context untouched */
      RETHROW;
    }
  }
  /* end_of_good_example */
}
#endif /* SIMGRID_TEST */
